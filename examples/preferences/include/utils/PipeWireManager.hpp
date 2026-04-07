#pragma once

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <map>
#include <memory>

namespace horizon::preferences
{
    struct AudioNode
    {
        uint32_t id;
        std::string name;
        std::string description;
        std::string media_class;
        uint32_t card_id = 0;

        float volume = 1.0f;
        bool mute = false;
        float balance = 0.0f;

        bool is_stream = false;
        std::string application_name;
        std::string application_icon_name;
    };

    struct AudioProfile
    {
        uint32_t index;       // Profile index as per SPA_PARAM_PROFILE_index
        std::string name;     // e.g. "analog-stereo"
        std::string description;
        uint32_t priority;
        bool is_output{false};
        bool is_input{false};
        bool is_active{false};
        uint32_t card_id;     // The PW_TYPE_INTERFACE_Device it belongs to
    };

    struct AudioItem
    {
        bool is_profile = false;
        uint32_t node_id = 0;
        uint32_t device_id = 0;
        uint32_t profile_index = 0;
        std::string description;
        bool is_default = false;

        // Stream-specific
        bool is_stream = false;
        std::string application_name;
        std::string application_icon_name;
        std::string stream_type; // "Salida" or "Entrada"
        float volume = 1.0f;
        bool mute = false;
    };

    class PipeWireManager;

    struct DeviceProxy {
        PipeWireManager *manager;
        uint32_t id;
        struct pw_proxy *proxy;
        struct spa_hook listener;
    };

    class PipeWireManager
    {
    public:
        static PipeWireManager &instance();

        PipeWireManager();
        ~PipeWireManager();

        void start();
        void stop();

        std::vector<AudioItem> get_sinks();
        std::vector<AudioItem> get_sources();
        std::vector<AudioItem> get_app_streams();

        // Node Configuration
        void set_volume(uint32_t node_id, float volume);
        void set_mute(uint32_t node_id, bool mute);
        void set_balance(uint32_t node_id, float balance);

        // Routing Configuration
        void set_default_node(uint32_t node_id);
        void set_device_profile(uint32_t device_id, uint32_t profile_index);

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
        static void device_event_param(void *data, int seq, uint32_t id, uint32_t index, uint32_t next, const struct spa_pod *param);
        static int metadata_event_property(void *data, uint32_t id, const char *key, const char *type, const char *value);
        static void device_event_info(void *data, const struct pw_device_info *info);

    private:
        void handle_global(uint32_t id, const char *type, const spa_dict *props);
        void handle_global_remove(uint32_t id);
        void handle_node_info(uint32_t id, const pw_node_info *info);
        void handle_node_param(uint32_t id, uint32_t param_id, const spa_pod *param);
        void handle_device_param(uint32_t id, uint32_t param_id, const spa_pod *param);
        void handle_metadata_property(const char *key, const char *value);
        void handle_device(uint32_t id, const spa_dict *props);
        void handle_device_info(uint32_t id, const pw_device_info *info);

        struct pw_thread_loop *m_loop{nullptr};
        struct pw_context *m_context{nullptr};
        struct pw_core *m_core{nullptr};
        struct pw_registry *m_registry{nullptr};
        struct spa_hook m_registry_listener;

        std::mutex m_mutex;
        std::map<uint32_t, AudioNode> m_nodes;
        std::map<uint32_t, struct pw_proxy *> m_proxies;
        std::map<uint32_t, std::shared_ptr<DeviceProxy>> m_device_proxies;
        std::map<uint32_t, struct spa_hook> m_node_listeners;
        std::map<uint32_t, std::vector<AudioProfile>> m_card_profiles;
        std::map<uint32_t, std::string> m_active_profile;

        std::string m_default_sink_name;
        std::string m_default_source_name;
        struct pw_metadata *m_default_metadata{nullptr};

        bool m_started{false};
    };
}
