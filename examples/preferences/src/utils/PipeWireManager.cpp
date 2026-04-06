#include <utils/PipeWireManager.hpp>
#include <pipewire/pipewire.h>
#include <pipewire/extensions/metadata.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>
#include <spa/utils/result.h>
#include <iostream>
#include <algorithm>

namespace horizon::preferences
{
    static const struct pw_registry_events registry_events = {
        .version = PW_VERSION_REGISTRY_EVENTS,
        .global = PipeWireManager::registry_event_global,
        .global_remove = PipeWireManager::registry_event_global_remove,
    };

    static const struct pw_node_events node_events = {
        .version = PW_VERSION_NODE_EVENTS,
        .info = PipeWireManager::node_event_info,
        .param = PipeWireManager::node_event_param,
    };

    static const struct pw_metadata_events metadata_events = {
        .version = PW_VERSION_METADATA_EVENTS,
        .property = PipeWireManager::metadata_event_property,
    };

    PipeWireManager &PipeWireManager::instance()
    {
        static PipeWireManager inst;
        return inst;
    }

    PipeWireManager::PipeWireManager()
    {
        pw_init(nullptr, nullptr);
    }

    PipeWireManager::~PipeWireManager()
    {
        stop();
        pw_deinit();
    }

    void PipeWireManager::start()
    {
        if (m_started) return;

        m_loop = pw_thread_loop_new("PipeWireManager", nullptr);
        m_context = pw_context_new(pw_thread_loop_get_loop(m_loop), nullptr, 0);
        m_core = pw_context_connect(m_context, nullptr, 0);
        
        if (!m_core) {
            std::cerr << "Failed to connect to PipeWire core" << std::endl;
            return;
        }

        m_registry = pw_core_get_registry(m_core, PW_VERSION_REGISTRY, 0);
        pw_registry_add_listener(m_registry, &m_registry_listener, &registry_events, this);

        pw_thread_loop_start(m_loop);
        m_started = true;
    }

    void PipeWireManager::stop()
    {
        if (!m_started) return;

        pw_thread_loop_stop(m_loop);
        
        for (auto &pair : m_proxies) {
            pw_proxy_destroy(pair.second);
        }
        m_proxies.clear();

        pw_proxy_destroy((struct pw_proxy *)m_registry);
        pw_core_disconnect(m_core);
        pw_context_destroy(m_context);
        pw_thread_loop_destroy(m_loop);

        m_started = false;
    }

    std::vector<AudioDevice> PipeWireManager::get_sinks()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<AudioDevice> sinks;
        for (auto &pair : m_devices) {
            if (pair.second.media_class == "Audio/Sink") {
                AudioDevice dev = pair.second;
                dev.is_default = (dev.name == m_default_sink_name);
                sinks.push_back(dev);
            }
        }
        return sinks;
    }

    std::vector<AudioDevice> PipeWireManager::get_sources()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<AudioDevice> sources;
        for (auto &pair : m_devices) {
            if (pair.second.media_class == "Audio/Source") {
                AudioDevice dev = pair.second;
                dev.is_default = (dev.name == m_default_source_name);
                sources.push_back(dev);
            }
        }
        return sources;
    }

    void PipeWireManager::set_volume(uint32_t id, float volume)
    {
        pw_thread_loop_lock(m_loop);
        auto it = m_proxies.find(id);
        if (it != m_proxies.end()) {
            struct pw_node *node = (struct pw_node *)it->second;
            char buf[1024];
            struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
            struct spa_pod *param = (struct spa_pod *)spa_pod_builder_add_object(&b,
                SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
                SPA_PROP_volume, SPA_TYPE_Float, volume);
            pw_node_set_param(node, SPA_PARAM_Props, 0, param);
        }
        pw_thread_loop_unlock(m_loop);
    }

    void PipeWireManager::set_mute(uint32_t id, bool mute)
    {
        pw_thread_loop_lock(m_loop);
        auto it = m_proxies.find(id);
        if (it != m_proxies.end()) {
            struct pw_node *node = (struct pw_node *)it->second;
            char buf[1024];
            struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
            struct spa_pod *param = (struct spa_pod *)spa_pod_builder_add_object(&b,
                SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
                SPA_PROP_mute, SPA_TYPE_Bool, mute);
            pw_node_set_param(node, SPA_PARAM_Props, 0, param);
        }
        pw_thread_loop_unlock(m_loop);
    }

    void PipeWireManager::set_balance(uint32_t id, float balance)
    {
        // For simplicity, we assume stereo.
        // Balance -1.0 = L only, 1.0 = R only, 0.0 = Equal.
        float volumes[2];
        if (balance < 0) {
            volumes[0] = 1.0f;
            volumes[1] = 1.0f + balance;
        } else {
            volumes[0] = 1.0f - balance;
            volumes[1] = 1.0f;
        }

        pw_thread_loop_lock(m_loop);
        auto it = m_proxies.find(id);
        if (it != m_proxies.end()) {
            struct pw_node *node = (struct pw_node *)it->second;
            char buf[1024];
            struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
            struct spa_pod *param = (struct spa_pod *)spa_pod_builder_add_object(&b,
                SPA_TYPE_OBJECT_Props, SPA_PARAM_Props,
                SPA_PROP_channelVolumes, SPA_TYPE_Array, volumes); // This is a simplification
            pw_node_set_param(node, SPA_PARAM_Props, 0, param);
        }
        pw_thread_loop_unlock(m_loop);
    }

    void PipeWireManager::set_default(uint32_t id)
    {
        // Logic to set default via metadata or WirePlumber tool
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_devices.count(id)) {
            const auto& dev = m_devices[id];
            std::string command;
            if (dev.media_class == "Audio/Sink") {
                command = "wpctl set-default " + std::to_string(id); // Using wpctl is easier
                m_default_sink_name = dev.name;
            } else {
                command = "wpctl set-default " + std::to_string(id);
                m_default_source_name = dev.name;
            }
            std::system(command.c_str());
            if (on_devices_changed) on_devices_changed();
        }
    }

    void PipeWireManager::registry_event_global(void *data, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props)
    {
        reinterpret_cast<PipeWireManager *>(data)->handle_global(id, type, props);
    }

    void PipeWireManager::registry_event_global_remove(void *data, uint32_t id)
    {
        reinterpret_cast<PipeWireManager *>(data)->handle_global_remove(id);
    }

    void PipeWireManager::node_event_info(void *data, const struct pw_node_info *info)
    {
        // Implementation for property updates
    }

    void PipeWireManager::node_event_param(void *data, int seq, uint32_t id, uint32_t index, uint32_t next, const struct spa_pod *param)
    {
        // Implementation for param updates
    }

    int PipeWireManager::metadata_event_property(void *data, uint32_t id, const char *key, const char *type, const char *value)
    {
        reinterpret_cast<PipeWireManager *>(data)->handle_metadata_property(key, value);
        return 0;
    }

    void PipeWireManager::handle_global(uint32_t id, const char *type, const spa_dict *props)
    {
        std::string interface(type);
        if (interface == PW_TYPE_INTERFACE_Node) {
            const char *media_class = pw_properties_get((struct pw_properties *)props, "media.class");
            if (media_class && (std::string(media_class) == "Audio/Sink" || std::string(media_class) == "Audio/Source")) {
                std::lock_guard<std::mutex> lock(m_mutex);
                AudioDevice dev;
                dev.id = id;
                dev.media_class = media_class;
                dev.name = pw_properties_get((struct pw_properties *)props, "node.name") ?: "Unknown";
                dev.description = pw_properties_get((struct pw_properties *)props, "node.description") ?: dev.name;
                
                m_devices[id] = dev;

                struct pw_node *node = (struct pw_node *)pw_registry_bind(m_registry, id, type, PW_VERSION_NODE, 0);
                m_proxies[id] = (struct pw_proxy *)node;
                
                if (on_devices_changed) on_devices_changed();
            }
        } else if (interface == PW_TYPE_INTERFACE_Metadata) {
            struct pw_metadata *metadata = (struct pw_metadata *)pw_registry_bind(m_registry, id, type, PW_VERSION_METADATA, 0);
            struct spa_hook *listener = new struct spa_hook(); // Leak for simplicity, should manage better
            pw_metadata_add_listener(metadata, listener, &metadata_events, this);
            m_proxies[id] = (struct pw_proxy *)metadata;
        }
    }

    void PipeWireManager::handle_global_remove(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_devices.erase(id) > 0) {
            auto it = m_proxies.find(id);
            if (it != m_proxies.end()) {
                pw_proxy_destroy(it->second);
                m_proxies.erase(it);
            }
            if (on_devices_changed) on_devices_changed();
        }
    }

    void PipeWireManager::handle_metadata_property(const char *key, const char *value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (value) {
            std::string val(value);
            // value is usually a JSON-like object: {"name":"..."}
            size_t start = val.find("\"name\":\"");
            if (start != std::string::npos) {
                start += 8;
                size_t end = val.find("\"", start);
                if (end != std::string::npos) {
                    val = val.substr(start, end - start);
                }
            }

            if (std::string(key) == "default.audio.sink") {
                m_default_sink_name = val;
            } else if (std::string(key) == "default.audio.source") {
                m_default_source_name = val;
            }
            if (on_devices_changed) on_devices_changed();
        }
    }
}
