#include <horizon/Application.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class TopPanelWidget;

class TopPanelApplication : public horizon::Application
{
public:
    TopPanelApplication();
    virtual ~TopPanelApplication() override;

    void run_panel();

    // Delegation methods for window operations
    size_t add_timer(int ms, std::function<void()> cb, bool repeat = false) { return m_window->add_timer(ms, cb, repeat); }
    void stop_timer(size_t id) { m_window->stop_timer(id); }
    void post_task(std::function<void()> task) { m_window->post_task(task); }
    void send_remote_signal(int pid, const std::string &signal, const std::string &arg = "") { m_window->send_remote_signal(pid, signal, arg); }
    void invalidate() { m_window->invalidate(); }

    horizon::WaylandLayerWindow* window() const { return m_window; }

private:
    void setup_window();
    void setup_ipc();
    void process_messages();

    TopPanelWidget *m_root_widget;
    std::unique_ptr<horizon::IpcClient> m_ipc_client;

    std::mutex m_queue_mutex;
    std::vector<std::string> m_pending_messages;
    horizon::WaylandLayerWindow *m_window{nullptr};
};
