#pragma once

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <map>

namespace horizon::preferences
{
    struct AudioDevice
    {
        uint32_t id;
        std::string name;
        std::string description;
        std::string media_class; // "Audio/Sink" or "Audio/Source"
        bool is_default{false};
        float volume{1.0f};
        bool mute{false};
        float balance{0.0f}; // -1.0 to 1.0 (Left to Right)
    };

    class PipeWireManager
    {
    public:
        static PipeWireManager &instance();

        PipeWireManager();
        ~PipeWireManager();

        void start();
        void stop();

        std::vector<AudioDevice> get_sinks();
        std::vector<AudioDevice> get_sources();

        void set_volume(uint32_t id, float volume);
        void set_mute(uint32_t id, bool mute);
        void set_balance(uint32_t id, float balance);
        void set_default(uint32_t id);

        // Signals/Callbacks
        std::function<void()> on_devices_changed;
        std::function<void(uint32_t id, float volume, bool mute)> on_volume_changed;

    public:
        // Calibration/Callback functions for PipeWire
        static void registry_event_global(void *data, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props);
        static void registry_event_global_remove(void *data, uint32_t id);
        static void proxy_event_removed(void *data);
        static void node_event_info(void *data, const struct pw_node_info *info);
        static void node_event_param(void *data, int seq, uint32_t id, uint32_t index, uint32_t next, const struct spa_pod *param);
        static int metadata_event_property(void *data, uint32_t id, const char *key, const char *type, const char *value);

    private:
        void handle_global(uint32_t id, const char *type, const spa_dict *props);
        void handle_global_remove(uint32_t id);
        void handle_node_info(uint32_t id, const pw_node_info *info);
        void handle_node_param(uint32_t id, uint32_t param_id, const spa_pod *param);
        void handle_metadata_property(const char *key, const char *value);

        struct pw_thread_loop *m_loop{nullptr};
        struct pw_context *m_context{nullptr};
        struct pw_core *m_core{nullptr};
        struct pw_registry *m_registry{nullptr};
        struct spa_hook m_registry_listener;

        std::mutex m_mutex;
        std::map<uint32_t, AudioDevice> m_devices;
        std::map<uint32_t, struct pw_proxy *> m_proxies;
        std::map<uint32_t, struct spa_hook> m_node_listeners;

        std::string m_default_sink_name;
        std::string m_default_source_name;

        bool m_started{false};
    };
}
