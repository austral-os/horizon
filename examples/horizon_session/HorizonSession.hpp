#include "IpcServer.hpp"
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct HznSessionMessage
{
    int sender_pid;
    int receiver_pid;
    std::string sender_id; // por si se necesita comunicarse con un servicio especifico como dock,
                           // top_panel, etc.
    std::string receiver_id;
    std::string message; // JSON message
};

enum class HznSessionEvent
{
    APP_STARTED,
    APP_STOPPED,
    APP_MINIMIZED,
    APP_MAXIMIZED,
    APP_RESTORED,
    APP_MOVED,
    APP_RESIZED,
    APP_FOCUS,
    APP_BLUR,
    APP_CLOSE,
    MENU_SHOW,
    MENU_CLICKED,
};

struct AppInfo
{
    std::string id;
    std::string name;
    int pid;
    std::string icon;
    bool show_in_dock;
    bool show_in_system_tray;
    bool is_minimized;
    std::vector<HznSessionEvent> subscriptions;
    int x, y;
    int width, height;
    int monitor;
};

class HorizonSession
{
public:
    HorizonSession();
    ~HorizonSession();

    void init(bool with_compositor = false);
    void start();
    void stop();
    bool is_running() const
    {
        return m_running;
    }
    void terminate_all_apps();

    void run_app(const std::string &app_path);
    void run_service(const std::string &service_path);
    void run_startup_services();

    std::string handle_ipc_message(const std::string &msg);

    void add_app(int pid, const AppInfo &app);
    void remove_app(int pid);
    std::optional<AppInfo> get_app(int pid);

    void send_message(const HznSessionMessage &message); // Send message to a specific app

    bool is_subscribed(int pid, HznSessionEvent event);
    void subscribe(int pid, HznSessionEvent event);
    void unsubscribe(int pid, HznSessionEvent event);
    std::vector<AppInfo> get_subscribers(HznSessionEvent event);

    void send_to_subscribers(HznSessionEvent event, const HznSessionMessage &message);

private:
    std::vector<std::string> get_wayland_displays();
    std::string wait_for_new_wayland_display(const std::vector<std::string> &existing);
    /**
     * @brief Path del socket del servidor
     */
    std::string m_server_socket_path;
    /**
     * @brief Listado de servicios que se inician al iniciar la sesión
     */
    std::vector<std::string> m_startup_services;
    /**
     * @brief Mapa de aplicaciones que se estan ejecutando
     */
    std::map<int, AppInfo> m_apps;
    /**
     * @brief Mapa para optimizar la busqueda de aplicaciones por evento
     */
    std::map<HznSessionEvent, std::vector<int>> m_event_subscribers;
    /**
     * @brief Mutex para proteger el estado concurrente (m_apps, suscripciones, etc)
     */
    std::mutex m_state_mutex;
    /**
     * @brief Servidor de IPC
     */
    std::unique_ptr<horizon::IpcServer> m_server;
    /**
     * @brief Flag para controlar la ejecución de la sesión
     */
    bool m_running = true;
    /**
     * @brief Lista de PIDs de procesos hijos para terminarlos al cerrar la sesión
     */
    std::vector<int> m_spawned_pids;
};