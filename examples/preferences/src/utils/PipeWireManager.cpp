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
    
    static const struct pw_device_events device_events = {
        .version = PW_VERSION_DEVICE_EVENTS,
        .info = PipeWireManager::device_event_info,
        .param = PipeWireManager::device_event_param,
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
        
        // Add active nodes
        for (auto &pair : m_devices) {
            if (pair.second.media_class == "Audio/Sink") {
                AudioDevice dev = pair.second;
                dev.is_default = (dev.name == m_default_sink_name);
                sinks.push_back(dev);
            }
        }

        // Add available profiles from cards that are NOT the active node's card, or are different profiles
        for (auto &card_pair : m_card_profiles) {
            uint32_t card_id = card_pair.first;
            std::string active = m_active_profile[card_id];
            
            for (auto &prof : card_pair.second) {
                if (!prof.is_output) continue;
                if (prof.name == active) continue; // Already shown as a node

                AudioDevice dev;
                dev.id = card_id + 0x20000; // Offset for output profile IDs
                dev.name = prof.name;
                dev.description = prof.description;
                dev.media_class = "Card/Profile";
                dev.is_profile = true;
                dev.profile_name = prof.name;
                dev.card_id = card_id;
                dev.is_default = (prof.name == active);
                
                m_devices[dev.id] = dev;
                sinks.push_back(dev);
            }
        }

        return sinks;
    }

    std::vector<AudioDevice> PipeWireManager::get_sources()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<AudioDevice> sources;
        
        // Add active nodes
        for (auto &pair : m_devices) {
            if (pair.second.media_class == "Audio/Source") {
                AudioDevice dev = pair.second;
                dev.is_default = (dev.name == m_default_source_name);
                sources.push_back(dev);
            }
        }

        // Add available profiles
        for (auto &card_pair : m_card_profiles) {
            uint32_t card_id = card_pair.first;
            std::string active = m_active_profile[card_id];
            
            for (auto &prof : card_pair.second) {
                if (!prof.is_input) continue;
                if (prof.name == active) continue;

                AudioDevice dev;
                dev.id = card_id + 0x10000; // Offset for profile IDs as a simple workaround
                dev.name = prof.name;
                dev.description = prof.description;
                dev.media_class = "Card/Profile";
                dev.is_profile = true;
                dev.profile_name = prof.name;
                dev.card_id = card_id;
                dev.is_default = (prof.name == active);
                
                // Store in m_devices temporarily so set_default can find it
                m_devices[dev.id] = dev;
                
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
                SPA_PROP_volume, "f", volume);
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
                SPA_PROP_mute, "b", mute);
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
                SPA_PROP_channelVolumes, "a", sizeof(float), SPA_TYPE_Float, 2, volumes);
            pw_node_set_param(node, SPA_PARAM_Props, 0, param);
        }
        pw_thread_loop_unlock(m_loop);
    }

    void PipeWireManager::set_default(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_devices.count(id)) {
            const auto& dev = m_devices[id];
            if (dev.is_profile) {
                // Switch profile
                std::string command = "wpctl set-profile " + std::to_string(dev.card_id) + " " + dev.profile_name;
                std::system(command.c_str());
            } else {
                // Set default node
                std::string command = "wpctl set-default " + std::to_string(id);
                std::system(command.c_str());
                if (dev.media_class == "Audio/Sink") m_default_sink_name = dev.name;
                else m_default_source_name = dev.name;
            }
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

    void PipeWireManager::device_event_info(void *data, const struct pw_device_info *info)
    {
        DeviceProxy *proxy = reinterpret_cast<DeviceProxy *>(data);
        proxy->manager->handle_device_info(proxy->id, info);
    }

    void PipeWireManager::device_event_param(void *data, int seq, uint32_t id, uint32_t index, uint32_t next, const struct spa_pod *param)
    {
        DeviceProxy *proxy = reinterpret_cast<DeviceProxy *>(data);
        // id here is param_id (e.g. SPA_PARAM_EnumProfile)
        proxy->manager->handle_device_param(proxy->id, id, param);
    }

    void PipeWireManager::handle_global(uint32_t id, const char *type, const spa_dict *props)
    {
        std::string interface(type);
        if (interface == PW_TYPE_INTERFACE_Node) {
            const char *media_class = spa_dict_lookup(props, "media.class");
            if (media_class && (std::string(media_class) == "Audio/Sink" || std::string(media_class) == "Audio/Source")) {
                std::lock_guard<std::mutex> lock(m_mutex);
                AudioDevice dev;
                dev.id = id;
                dev.media_class = media_class;
                dev.name = spa_dict_lookup(props, "node.name") ?: "Unknown";
                dev.description = spa_dict_lookup(props, "node.description") ?: dev.name;
                
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
        } else if (interface == PW_TYPE_INTERFACE_Device) {
            handle_device(id, props);
        }
    }

    void PipeWireManager::handle_device(uint32_t id, const spa_dict *props)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const char *media_class = spa_dict_lookup(props, "media.class");
        if (media_class && std::string(media_class) == "Audio/Device") {
            struct pw_device *device = (struct pw_device *)pw_registry_bind(m_registry, id, PW_TYPE_INTERFACE_Device, PW_VERSION_DEVICE, 0);
            if (!device) return;
            
            auto proxy = std::make_shared<DeviceProxy>();
            proxy->manager = this;
            proxy->id = id;
            proxy->proxy = (struct pw_proxy *)device;
            
            m_proxies[id] = (struct pw_proxy *)device;
            m_device_proxies[id] = proxy;
            
            pw_device_add_listener(device, &proxy->listener, &device_events, proxy.get());
        }
    }

    void PipeWireManager::handle_device_info(uint32_t id, const pw_device_info *info)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (info->change_mask & PW_DEVICE_CHANGE_MASK_PARAMS) {
            for (uint32_t i = 0; i < info->n_params; i++) {
                if (info->params[i].id == SPA_PARAM_EnumProfile || info->params[i].id == SPA_PARAM_Profile) {
                    if (m_proxies.count(id)) {
                        pw_device_enum_params((struct pw_device *)m_proxies[id], 0, info->params[i].id, 0, 0, NULL);
                    }
                }
            }
        }
    }

    void PipeWireManager::handle_device_param(uint32_t device_id, uint32_t param_id, const spa_pod *param)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (param_id == SPA_PARAM_EnumProfile) {
            uint32_t profile_id = 0, priority = 0;
            const char *name = nullptr, *desc = nullptr;

            if (spa_pod_parse_object(param,
                    SPA_TYPE_OBJECT_ParamProfile, NULL,
                    SPA_PARAM_PROFILE_index, "i", &profile_id,
                    SPA_PARAM_PROFILE_name, "s", &name,
                    SPA_PARAM_PROFILE_description, "s", &desc,
                    SPA_PARAM_PROFILE_priority, "i", &priority) < 0) {
                return;
            }

            if (!name) return;

            AudioProfile prof;
            prof.index = profile_id;
            prof.name = name;
            prof.description = desc ? desc : name;
            prof.priority = priority;
            
            // Simple heuristic for role if classes parsing is too complex
            std::string n(name);
            prof.is_output = (n.find("output") != std::string::npos || n.find("sink") != std::string::npos || n.find("stereo") != std::string::npos || n.find("surround") != std::string::npos || n.find("hdmi") != std::string::npos || n.find("pro-audio") != std::string::npos);
            prof.is_input = (n.find("input") != std::string::npos || n.find("source") != std::string::npos || n.find("analog-stereo") != std::string::npos || n.find("pro-audio") != std::string::npos);

            // Avoid duplicates
            auto &profiles = m_card_profiles[device_id];
            bool found = false;
            for (const auto& p : profiles) if (p.name == prof.name) { found = true; break; }
            if (!found) profiles.push_back(prof);

        } else if (param_id == SPA_PARAM_Profile) {
            uint32_t profile_id;
            if (spa_pod_parse_object(param,
                    SPA_TYPE_OBJECT_ParamProfile, NULL,
                    SPA_PARAM_PROFILE_index, "i", &profile_id) >= 0) {
                for (const auto& p : m_card_profiles[device_id]) {
                    if (p.index == profile_id) {
                        m_active_profile[device_id] = p.name;
                        break;
                    }
                }
            }
        }
        if (on_devices_changed) on_devices_changed();
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
