#include "horizon/CairoGraphicsContext.hpp"
#include "horizon/Widget.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>
#include <horizon/Application.hpp>
#include <horizon/ClientMenu.hpp>
#include <horizon/I18n.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/JsonBackend.hpp>
#include <horizon/LabwcCompositorContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/WayfireCompositorContext.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/Window.hpp>
#include <horizon/xdg-shell-client-protocol.h>
#include <librsvg/rsvg.h>
#include <linux/input-event-codes.h>
#include <nlohmann/json.hpp>
#include <poll.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

namespace horizon
{

    Application::Application(const std::string &app_id, int w, int h)
        : Application(app_id, w, h, false)
    {
    }

    Application::Application(const std::string &app_id, int w, int h, bool defer_init,
                             bool skip_window)
        : m_app_id(app_id)
    {
        // Global safeguard: ignore SIGPIPE to prevent crash when writing to broken sockets
        signal(SIGPIPE, SIG_IGN);

        // Initialize i18n
        i18n(); // Ensure singleton is initialized, it will load core locales automatically

        m_name = "Horizon Application";
        if (!skip_window)
        {
            create_window(w, h); // Create initial main window
        }
    }

    Application::~Application()
    {
        m_is_running = false;

        std::lock_guard<std::mutex> lock(m_windows_mutex);
        for (auto &mw : m_managed_windows)
        {
            mw.window->quit();
        }

        for (auto &mw : m_managed_windows)
        {
            if (mw.thread.joinable())
                mw.thread.join();
        }

        m_managed_windows.clear();
    }

    void Application::set_name(const std::string &name)
    {
        m_name = name;
        std::lock_guard<std::mutex> lock(m_windows_mutex);
        for (auto &mw : m_managed_windows)
        {
            mw.window->set_name(name);
        }
    }

    void Application::set_icon_name(const std::string &icon_name)
    {
        m_icon_name = icon_name;
        std::lock_guard<std::mutex> lock(m_windows_mutex);
        for (auto &mw : m_managed_windows)
        {
            mw.window->set_icon_name(icon_name);
        }
    }

    void Application::set_root(std::unique_ptr<Widget> root)
    {
        std::lock_guard<std::mutex> lock(m_windows_mutex);
        if (!m_managed_windows.empty())
        {
            m_managed_windows[0].window->set_root(std::move(root));
        }
    }

    void Application::add_menu(std::unique_ptr<Menu> menu)
    {
        std::lock_guard<std::mutex> lock(m_windows_mutex);
        if (!m_managed_windows.empty())
        {
            m_managed_windows[0].window->add_menu(std::move(menu));
        }
    }

    void Application::set_app_menu(std::unique_ptr<Menu> menu)
    {
        std::lock_guard<std::mutex> lock(m_windows_mutex);
        if (!m_managed_windows.empty())
        {
            m_managed_windows[0].window->set_app_menu(std::move(menu));
        }
    }

    void Application::remove_window(WaylandWindow *ptr)
    {
        // Must be called from the main thread to avoid deadlocks and ensure safety
        // if we are destroying windows that might be in use by other main-thread logic.

        std::lock_guard<std::mutex> lock(m_windows_mutex);
        if (m_managed_windows.empty())
            return;

        // Never remove the main window via this method
        if (ptr == m_managed_windows[0].window.get())
            return;

        auto it = std::find_if(m_managed_windows.begin() + 1, m_managed_windows.end(),
                               [ptr](const ManagedWindow &mw) { return mw.window.get() == ptr; });

        if (it != m_managed_windows.end())
        {
            LOG_INFO << "[APP] Removing window: " << ptr->name();
            // We can't join the thread if we are currently IN it,
            // but this method is intended to be called via post_task on main thread.
            if (it->thread.joinable())
                it->thread.detach(); // Detach since it's already finished or finishing
            m_managed_windows.erase(it);
        }
    }

    WaylandWindow *Application::create_window(int w, int h)
    {
        // We use defer_init=true so that initialization happens in the target thread
        auto window = std::make_unique<WaylandWindow>(m_app_id, w, h, true);
        window->set_name(m_name);
        window->set_icon_name(m_icon_name);
        window->set_about_manager(&m_about_manager);
        WaylandWindow *ptr = window.get();

        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            m_managed_windows.push_back({std::move(window), nullptr, {}});
        }

        if (m_is_running)
        {
            // Find the newly added ManagedWindow to set its thread
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            auto &mw = m_managed_windows.back();
            mw.thread = std::thread(
                [this, ptr]()
                {
                    ptr->initialize();
                    ptr->run();

                    // Once run returns, clean up
                    std::lock_guard<std::mutex> lock(m_windows_mutex);
                    if (m_managed_windows.empty())
                        return;
                    WaylandWindow *mainWinPtr = m_managed_windows[0].window.get();
                    mainWinPtr->post_task([this, ptr]() { this->remove_window(ptr); });
                });
        }

        return ptr;
    }

    WaylandWindow *Application::create_dialog(WaylandWindow *parent, int w, int h)
    {
        auto window = std::make_unique<WaylandWindow>(m_app_id, w, h, true);
        window->set_name(m_name);
        window->set_icon_name(m_icon_name);
        window->set_about_manager(&m_about_manager);
        WaylandWindow *ptr = window.get();

        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            m_managed_windows.push_back({std::move(window), parent, {}});
        }

        if (m_is_running)
        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            auto &mw = m_managed_windows.back();
            mw.thread = std::thread(
                [this, ptr]()
                {
                    ptr->initialize();
                    ptr->run();

                    std::lock_guard<std::mutex> lock(m_windows_mutex);
                    if (m_managed_windows.empty())
                        return;
                    WaylandWindow *mainWinPtr = m_managed_windows[0].window.get();
                    mainWinPtr->post_task([this, ptr]() { this->remove_window(ptr); });
                });
        }

        return ptr;
    }

    WaylandLayerWindow *Application::create_layer_window(const std::string &namespace_id,
                                                         uint32_t layer, int monitor_index)
    {
        auto window =
            std::make_unique<WaylandLayerWindow>(namespace_id, layer, true, monitor_index);
        window->set_about_manager(&m_about_manager);
        WaylandLayerWindow *ptr = window.get();

        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            m_managed_windows.push_back({std::move(window), nullptr, {}});
        }

        if (m_is_running)
        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            auto &mw = m_managed_windows.back();
            mw.thread = std::thread(
                [this, ptr]()
                {
                    ptr->initialize();
                    ptr->run();

                    std::lock_guard<std::mutex> lock(m_windows_mutex);
                    if (m_managed_windows.empty())
                        return;
                    WaylandWindow *mainWinPtr = m_managed_windows[0].window.get();
                    mainWinPtr->post_task([this, ptr]() { this->remove_window(ptr); });
                });
        }

        return ptr;
    }

    void Application::run()
    {
        // Validation: Every application must have name, description, version and icon.
        const auto &app_info = m_about_manager.app_data();
        if (app_info.title.empty() || app_info.description.empty() ||
            app_info.version.empty() || app_info.icon.empty())
        {
            LOG_ERROR << "[APP] Application failed to start: Missing mandatory about information.";
            LOG_ERROR << "[APP] Required: Title, Description, Version, and Icon.";
            return;
        }

        if (m_managed_windows.empty())
            return;

        m_is_running = true;

        // Start threads for auxiliary windows already created
        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            for (size_t i = 1; i < m_managed_windows.size(); ++i)
            {
                WaylandWindow *ptr = m_managed_windows[i].window.get();
                m_managed_windows[i].thread = std::thread(
                    [this, ptr]()
                    {
                        ptr->initialize();
                        ptr->run();

                        std::lock_guard<std::mutex> lock(m_windows_mutex);
                        if (m_managed_windows.empty())
                            return;
                        WaylandWindow *mainWinPtr = m_managed_windows[0].window.get();
                        mainWinPtr->post_task([this, ptr]() { this->remove_window(ptr); });
                    });
            }
        }

        // Run primary window in main thread
        WaylandWindow *mainWin = m_managed_windows[0].window.get();
        mainWin->initialize();
        mainWin->run();

        // Shutdown sequence: main window closed, stop everything else
        m_is_running = false;

        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            for (auto &mw : m_managed_windows)
            {
                mw.window->quit();
            }
        }

        // Join threads safely
        std::vector<std::thread> threads_to_join;
        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            for (auto &mw : m_managed_windows)
            {
                if (mw.thread.joinable())
                {
                    threads_to_join.push_back(std::move(mw.thread));
                }
            }
        }

        for (auto &t : threads_to_join)
        {
            t.join();
        }
    }

    void Application::alert(const std::string &message, const std::string &title, MessageType type)
    {
        auto dialog = std::make_unique<MessageDialog>(title, message, type, false);
        WaylandWindow *ptr = dialog.get();

        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            m_managed_windows.push_back({std::move(dialog), nullptr, {}});
        }

        if (m_is_running)
        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            auto &mw = m_managed_windows.back();
            mw.thread = std::thread(
                [this, ptr]()
                {
                    ptr->initialize();
                    ptr->run();

                    std::lock_guard<std::mutex> lock(m_windows_mutex);
                    if (m_managed_windows.empty())
                        return;
                    WaylandWindow *mainWinPtr = m_managed_windows[0].window.get();
                    mainWinPtr->post_task([this, ptr]() { this->remove_window(ptr); });
                });
        }
    }

    bool Application::confirm(const std::string &message, const std::string &title,
                              MessageType type)
    {
        auto dialog = std::make_unique<MessageDialog>(title, message, type, true);
        WaylandWindow *ptr = dialog.get();

        std::promise<bool> promise;
        std::future<bool> future = promise.get_future();

        dialog->when_responded.connect(
            [&promise](MessageResponseEvent res)
            { promise.set_value(res.response == MessageResponse::Accept); });

        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            m_managed_windows.push_back({std::move(dialog), nullptr, {}});
        }

        if (m_is_running)
        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            auto &mw = m_managed_windows.back();
            mw.thread = std::thread(
                [this, ptr]()
                {
                    ptr->initialize();
                    ptr->run();

                    std::lock_guard<std::mutex> lock(m_windows_mutex);
                    if (m_managed_windows.empty())
                        return;
                    WaylandWindow *mainWinPtr = m_managed_windows[0].window.get();
                    mainWinPtr->post_task([this, ptr]() { this->remove_window(ptr); });
                });
        }

        return future.get();
    }

    void Application::set_preferences_content(WaylandWindow::PreferencesFactory factory, int width,
                                              int height)
    {
        std::lock_guard<std::mutex> lock(m_windows_mutex);
        if (!m_managed_windows.empty())
        {
            m_managed_windows[0].window->set_preferences_content(std::move(factory), width, height);
        }
    }

    void Application::show_preferences()
    {
        std::lock_guard<std::mutex> lock(m_windows_mutex);
        if (!m_managed_windows.empty())
        {
            m_managed_windows[0].window->show_preferences();
        }
    }


    void Application::show_aboutus()
    {
        std::lock_guard<std::mutex> lock(m_windows_mutex);
        if (!m_managed_windows.empty())
        {
            m_managed_windows[0].window->show_about_dialog(m_about_manager);
        }
    }

    AboutManager &Application::about_manager()
    {
        return m_about_manager;
    }

    void Application::post_task(std::function<void()> task)
    {
        std::lock_guard<std::mutex> lock(m_windows_mutex);
        if (!m_managed_windows.empty())
        {
            m_managed_windows[0].window->post_task(std::move(task));
        }
    }

} // namespace horizon
