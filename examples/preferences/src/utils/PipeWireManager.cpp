#include <algorithm>
#include <iostream>
#include <pipewire/extensions/metadata.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>
#include <spa/pod/builder.h>
#include <spa/pod/parser.h>
#include <spa/utils/result.h>
#include <utils/PipeWireManager.hpp>

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
        if (m_started)
            return;

        m_loop = pw_thread_loop_new("PipeWireManager", nullptr);
        m_context = pw_context_new(pw_thread_loop_get_loop(m_loop), nullptr, 0);
        m_core = pw_context_connect(m_context, nullptr, 0);

        if (!m_core)
        {
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
        if (!m_started)
            return;

        pw_thread_loop_stop(m_loop);

        for (auto &pair : m_proxies)
        {
            pw_proxy_destroy(pair.second);
        }
        m_proxies.clear();

        pw_proxy_destroy((struct pw_proxy *)m_registry);
        pw_core_disconnect(m_core);
        pw_context_destroy(m_context);
        pw_thread_loop_destroy(m_loop);

        m_started = false;
    }

    std::vector<AudioItem> PipeWireManager::get_sinks()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<AudioItem> sinks;
        std::map<uint32_t, bool> card_has_sink_node;

        // Add active nodes
        for (auto &pair : m_nodes)
        {
            if (pair.second.media_class.find("Audio/Sink") != std::string::npos)
            {
                // Sinks are outputs
                AudioNode &node = pair.second;
                if ((node.name.find("dummy") != std::string::npos ||
                     node.name.find("auto_null") != std::string::npos) &&
                    m_nodes.size() > 2)
                    continue; // Hide dummy if possible

                AudioItem item;
                item.is_profile = false;
                item.node_id = node.id;
                item.description = node.description;
                item.is_default = (node.name == m_default_sink_name);
                sinks.push_back(item);

                if (node.card_id != 0)
                    card_has_sink_node[node.card_id] = true;
            }
        }

        // Add available profiles from cards that are NOT the active node's card, or are different
        // profiles
        for (auto &card_pair : m_card_profiles)
        {
            uint32_t card_id = card_pair.first;
            std::string active = m_active_profile[card_id];

            for (auto &prof : card_pair.second)
            {
                if (!prof.is_output)
                    continue;
                if (prof.name == active && card_has_sink_node[card_id])
                    continue; // Already shown as a node

                AudioItem item;
                item.is_profile = true;
                item.device_id = card_id;
                item.profile_index = prof.index;
                item.description = prof.description;
                item.is_default = (prof.name == active);
                sinks.push_back(item);
            }
        }

        return sinks;
    }

    std::vector<AudioItem> PipeWireManager::get_sources()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<AudioItem> sources;
        std::map<uint32_t, bool> card_has_source_node;

        // Add active nodes
        for (auto &pair : m_nodes)
        {
            if (pair.second.media_class.find("Audio/Source") != std::string::npos)
            {
                // Sources are inputs, but skip monitors
                AudioNode &node = pair.second;
                if (node.name.find("monitor") != std::string::npos)
                    continue;

                AudioItem item;
                item.is_profile = false;
                item.node_id = node.id;
                item.description = node.description;
                item.is_default = (node.name == m_default_source_name);
                sources.push_back(item);

                if (node.card_id != 0)
                    card_has_source_node[node.card_id] = true;
            }
        }

        // Add available profiles
        for (auto &card_pair : m_card_profiles)
        {
            uint32_t card_id = card_pair.first;
            std::string active = m_active_profile[card_id];

            for (auto &prof : card_pair.second)
            {
                if (!prof.is_input)
                    continue;
                if (prof.name == active && card_has_source_node[card_id])
                    continue;

                AudioItem item;
                item.is_profile = true;
                item.device_id = card_id;
                item.profile_index = prof.index;
                item.description = prof.description;
                item.is_default = (prof.name == active);
                sources.push_back(item);
            }
        }
        return sources;
    }

    void PipeWireManager::set_volume(uint32_t node_id, float volume)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_nodes.count(node_id))
        {
            m_nodes[node_id].volume = volume;
        }

        pw_thread_loop_lock(m_loop);
        auto it = m_proxies.find(node_id);
        if (it != m_proxies.end())
        {
            struct pw_node *node = (struct pw_node *)it->second;
            char buf[1024];
            struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
            struct spa_pod *param = (struct spa_pod *)spa_pod_builder_add_object(
                &b, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props, SPA_PROP_volume, "f", volume);
            pw_node_set_param(node, SPA_PARAM_Props, 0, param);
        }
        pw_thread_loop_unlock(m_loop);
    }

    void PipeWireManager::set_mute(uint32_t node_id, bool mute)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_nodes.count(node_id))
        {
            m_nodes[node_id].mute = mute;
        }

        pw_thread_loop_lock(m_loop);
        auto it = m_proxies.find(node_id);
        if (it != m_proxies.end())
        {
            struct pw_node *node = (struct pw_node *)it->second;
            char buf[1024];
            struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
            struct spa_pod *param = (struct spa_pod *)spa_pod_builder_add_object(
                &b, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props, SPA_PROP_mute, "b", mute);
            pw_node_set_param(node, SPA_PARAM_Props, 0, param);
        }
        pw_thread_loop_unlock(m_loop);
    }

    void PipeWireManager::set_balance(uint32_t node_id, float balance)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_nodes.count(node_id))
            return;

        m_nodes[node_id].balance = balance;
        float base_vol = m_nodes[node_id].volume;

        // Balance -1.0 = L only, 1.0 = R only, 0.0 = Equal.
        float volumes[2];
        if (balance < 0)
        {
            volumes[0] = base_vol;
            volumes[1] = base_vol * (1.0f + balance);
        }
        else
        {
            volumes[0] = base_vol * (1.0f - balance);
            volumes[1] = base_vol;
        }

        pw_thread_loop_lock(m_loop);
        auto it = m_proxies.find(node_id);
        if (it != m_proxies.end())
        {
            struct pw_node *node = (struct pw_node *)it->second;
            char buf[1024];
            struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
            struct spa_pod *param = (struct spa_pod *)spa_pod_builder_add_object(
                &b, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props, SPA_PROP_channelVolumes, "a",
                sizeof(float), SPA_TYPE_Float, 2, volumes);
            pw_node_set_param(node, SPA_PARAM_Props, 0, param);
        }
        pw_thread_loop_unlock(m_loop);
    }

    void PipeWireManager::set_default_node(uint32_t node_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_nodes.count(node_id) && m_default_metadata)
        {
            const auto &node = m_nodes[node_id];

            pw_thread_loop_lock(m_loop);
            char value[512];
            snprintf(value, sizeof(value), "{\"name\": \"%s\"}", node.name.c_str());
            const char *key = (node.media_class.find("Audio/Sink") != std::string::npos)
                                  ? "default.audio.sink"
                                  : "default.audio.source";
            pw_metadata_set_property(m_default_metadata, PW_ID_CORE, key, "Spa:String:JSON", value);
            pw_thread_loop_unlock(m_loop);

            if (node.media_class.find("Audio/Sink") != std::string::npos)
                m_default_sink_name = node.name;
            else
                m_default_source_name = node.name;

            if (on_devices_changed)
                on_devices_changed();
        }
    }

    void PipeWireManager::set_device_profile(uint32_t device_id, uint32_t profile_index)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_proxies.find(device_id);
        if (it != m_proxies.end())
        {
            pw_thread_loop_lock(m_loop);
            struct pw_device *device = (struct pw_device *)it->second;

            char buf[1024];
            struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
            struct spa_pod *param = (struct spa_pod *)spa_pod_builder_add_object(
                &b, SPA_TYPE_OBJECT_ParamProfile, SPA_PARAM_Profile, SPA_PARAM_PROFILE_index, "i",
                profile_index, SPA_PARAM_PROFILE_save, "b", true);

            pw_device_set_param(device, SPA_PARAM_Profile, 0, param);
            pw_thread_loop_unlock(m_loop);
        }
    }

    void PipeWireManager::registry_event_global(void *data, uint32_t id, uint32_t permissions,
                                                const char *type, uint32_t version,
                                                const struct spa_dict *props)
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

    void PipeWireManager::node_event_param(void *data, int seq, uint32_t id, uint32_t index,
                                           uint32_t next, const struct spa_pod *param)
    {
        // Implementation for param updates
    }

    int PipeWireManager::metadata_event_property(void *data, uint32_t id, const char *key,
                                                 const char *type, const char *value)
    {
        reinterpret_cast<PipeWireManager *>(data)->handle_metadata_property(key, value);
        return 0;
    }

    void PipeWireManager::device_event_info(void *data, const struct pw_device_info *info)
    {
        DeviceProxy *proxy = reinterpret_cast<DeviceProxy *>(data);
        proxy->manager->handle_device_info(proxy->id, info);
    }

    void PipeWireManager::device_event_param(void *data, int seq, uint32_t id, uint32_t index,
                                             uint32_t next, const struct spa_pod *param)
    {
        DeviceProxy *proxy = reinterpret_cast<DeviceProxy *>(data);
        // id here is param_id (e.g. SPA_PARAM_EnumProfile)
        proxy->manager->handle_device_param(proxy->id, id, param);
    }

    void PipeWireManager::handle_global(uint32_t id, const char *type, const spa_dict *props)
    {
        std::string interface(type);
        if (interface == PW_TYPE_INTERFACE_Node)
        {
            const char *media_class = spa_dict_lookup(props, "media.class");
            if (media_class && (std::string(media_class) == "Audio/Sink" ||
                                std::string(media_class) == "Audio/Source"))
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                AudioNode node;
                node.id = id;
                node.media_class = media_class;
                node.name = spa_dict_lookup(props, "node.name") ?: "Unknown";
                node.description = spa_dict_lookup(props, "node.description") ?: node.name;

                const char *device_id_str = spa_dict_lookup(props, "device.id");
                if (device_id_str)
                {
                    node.card_id = std::stoul(device_id_str);
                }
                else
                {
                    node.card_id = 0;
                }

                m_nodes[id] = node;

                struct pw_node *pwNode =
                    (struct pw_node *)pw_registry_bind(m_registry, id, type, PW_VERSION_NODE, 0);
                m_proxies[id] = (struct pw_proxy *)pwNode;

                if (on_devices_changed)
                    on_devices_changed();
            }
        }
        else if (interface == PW_TYPE_INTERFACE_Metadata)
        {
            struct pw_metadata *metadata = (struct pw_metadata *)pw_registry_bind(
                m_registry, id, type, PW_VERSION_METADATA, 0);
            const char *name = spa_dict_lookup(props, "metadata.name");
            if (name && std::string(name) == "default") {
                m_default_metadata = metadata;
            }
            static const struct pw_metadata_events metadata_events = {
                PW_VERSION_METADATA_EVENTS,
                metadata_event_property
            };
            pw_metadata_add_listener(metadata, &m_node_listeners[id], &metadata_events, this);
            m_proxies[id] = (struct pw_proxy *)metadata;
        }
        else if (interface == PW_TYPE_INTERFACE_Device)
        {
            handle_device(id, props);
        }
    }

    void PipeWireManager::handle_device(uint32_t id, const spa_dict *props)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const char *media_class = spa_dict_lookup(props, "media.class");
        if (media_class && std::string(media_class) == "Audio/Device")
        {
            struct pw_device *device = (struct pw_device *)pw_registry_bind(
                m_registry, id, PW_TYPE_INTERFACE_Device, PW_VERSION_DEVICE, 0);
            if (!device)
                return;

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
        if (info->change_mask & PW_DEVICE_CHANGE_MASK_PARAMS)
        {
            for (uint32_t i = 0; i < info->n_params; i++)
            {
                if (info->params[i].id == SPA_PARAM_EnumProfile ||
                    info->params[i].id == SPA_PARAM_Profile)
                {
                    if (m_proxies.count(id))
                    {
                        pw_device_enum_params((struct pw_device *)m_proxies[id], 0,
                                              info->params[i].id, 0, 0, NULL);
                    }
                }
            }
        }
    }

    void PipeWireManager::handle_device_param(uint32_t device_id, uint32_t param_id,
                                              const spa_pod *param)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (param_id == SPA_PARAM_EnumProfile)
        {
            uint32_t profile_id = 0, priority = 0;
            const char *name = nullptr, *desc = nullptr;

            if (spa_pod_parse_object(
                    param, SPA_TYPE_OBJECT_ParamProfile, NULL, SPA_PARAM_PROFILE_index, "i",
                    &profile_id, SPA_PARAM_PROFILE_name, "s", &name, SPA_PARAM_PROFILE_description,
                    "s", &desc, SPA_PARAM_PROFILE_priority, "i", &priority) < 0)
            {
                return;
            }

            if (!name)
                return;

            AudioProfile prof;
            prof.index = profile_id;
            prof.name = name;
            prof.description = desc ? desc : name;
            prof.priority = priority;

            // Improved heuristic for role based on common PipeWire profile names
            std::string n(name);
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);

            // Hide raw pro-audio profiles as they don't produce typical sink/source nodes
            if (n.find("pro-audio") != std::string::npos)
                return;

            bool is_input_keyword =
                (n.find("input") != std::string::npos || n.find("source") != std::string::npos ||
                 n.find("mic") != std::string::npos || n.find("capture") != std::string::npos);
            bool is_output_keyword =
                (n.find("output") != std::string::npos || n.find("sink") != std::string::npos ||
                 n.find("playback") != std::string::npos || n.find("hdmi") != std::string::npos);
            bool is_duplex =
                (n.find("duplex") != std::string::npos || n.find("pro-audio") != std::string::npos);
            bool is_stereo_surround =
                (n.find("stereo") != std::string::npos || n.find("surround") != std::string::npos);

            if (is_duplex)
            {
                prof.is_input = true;
                prof.is_output = true;
            }
            else if (is_input_keyword && !is_output_keyword && !is_stereo_surround)
            {
                prof.is_input = true;
                prof.is_output = false;
            }
            else if (is_output_keyword && !is_input_keyword)
            {
                prof.is_input = false;
                prof.is_output = true;
            }
            else
            {
                // Mixed or generic names
                prof.is_output = is_stereo_surround || is_output_keyword;
                prof.is_input = is_input_keyword;

                // If it's just "analog-stereo", it's an output
                if (n == "analog-stereo" || n == "analog-surround-40" || n == "analog-surround-51")
                {
                    prof.is_output = true;
                    prof.is_input = false;
                }
            }

            // Avoid duplicates
            auto &profiles = m_card_profiles[device_id];
            bool found = false;
            for (const auto &p : profiles)
                if (p.name == prof.name)
                {
                    found = true;
                    break;
                }
            if (!found)
                profiles.push_back(prof);
        }
        else if (param_id == SPA_PARAM_Profile)
        {
            uint32_t profile_id;
            if (spa_pod_parse_object(param, SPA_TYPE_OBJECT_ParamProfile, NULL,
                                     SPA_PARAM_PROFILE_index, "i", &profile_id) >= 0)
            {
                for (const auto &p : m_card_profiles[device_id])
                {
                    if (p.index == profile_id)
                    {
                        m_active_profile[device_id] = p.name;
                        break;
                    }
                }
            }
        }
        if (on_devices_changed)
            on_devices_changed();
    }

    void PipeWireManager::handle_global_remove(uint32_t id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_nodes.erase(id);
        if (m_proxies.count(id)) {
            pw_proxy_destroy(m_proxies[id]);
            m_proxies.erase(id);
        }
        if (on_devices_changed)
            on_devices_changed();
    }

    void PipeWireManager::handle_metadata_property(const char *key, const char *value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (value)
        {
            std::string val(value);
            // value is usually a JSON-like object: {"name":"..."}
            size_t start = val.find("\"name\":\"");
            if (start != std::string::npos)
            {
                start += 8;
                size_t end = val.find("\"", start);
                if (end != std::string::npos)
                {
                    val = val.substr(start, end - start);
                }
            }

            if (std::string(key) == "default.audio.sink")
            {
                m_default_sink_name = val;
            }
            else if (std::string(key) == "default.audio.source")
            {
                m_default_source_name = val;
            }
            if (on_devices_changed)
                on_devices_changed();
        }
    }
} // namespace horizon::preferences
