#include "horizon/CairoGraphicsContext.hpp"
#include "horizon/Widget.hpp"
#include <cstdio>
#include <cstring>
#include <horizon/Application.hpp>
#include <horizon/ClientMenu.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/LabwcCompositorContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/WayfireCompositorContext.hpp>
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

    Application::Application(const std::string &app_id, int w, int h, bool defer_init)
        : m_app_id(app_id)
    {
        // Global safeguard: ignore SIGPIPE to prevent crash when writing to broken sockets
        signal(SIGPIPE, SIG_IGN);
        m_name = "Horizon Application";
        create_window(w, h); // Create initial main window
    }

    Application::~Application()
    {
        m_is_running = false;
        
        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            for (auto &mw : m_managed_windows)
            {
                mw.window->quit();
            }
        }

        for (auto &t : m_window_threads)
        {
            if (t.joinable())
                t.join();
        }
        
        std::lock_guard<std::mutex> lock(m_windows_mutex);
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

    WaylandWindow *Application::create_window(int w, int h)
    {
        // We use defer_init=true so that initialization happens in the target thread
        auto window = std::make_unique<WaylandWindow>(m_app_id, w, h, true);
        window->set_name(m_name);
        window->set_icon_name(m_icon_name);
        WaylandWindow *ptr = window.get();
        
        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            m_managed_windows.push_back({std::move(window), nullptr});
        }

        if (m_is_running)
        {
            m_window_threads.emplace_back([ptr]() {
                ptr->w_surface()->init_display();
                ptr->w_surface()->setup_xdg_toplevel(ptr->name(), ptr->app_id());
                ptr->run();
            });
        }
        
        return ptr;
    }

    WaylandWindow *Application::create_dialog(WaylandWindow *parent, int w, int h)
    {
        auto window = std::make_unique<WaylandWindow>(m_app_id, w, h, true);
        window->set_name(m_name);
        window->set_icon_name(m_icon_name);
        WaylandWindow *ptr = window.get();
        
        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            m_managed_windows.push_back({std::move(window), parent});
        }

        if (m_is_running)
        {
            m_window_threads.emplace_back([ptr]() {
                ptr->w_surface()->init_display();
                ptr->w_surface()->setup_xdg_toplevel(ptr->name(), ptr->app_id());
                ptr->run();
            });
        }
        
        return ptr;
    }

    void Application::run()
    {
        if (m_managed_windows.empty())
            return;

        m_is_running = true;

        // Start threads for auxiliary windows already created
        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            for (size_t i = 1; i < m_managed_windows.size(); ++i)
            {
                WaylandWindow *ptr = m_managed_windows[i].window.get();
                m_window_threads.emplace_back([ptr]() {
                    ptr->w_surface()->init_display();
                    ptr->w_surface()->setup_xdg_toplevel(ptr->name(), ptr->app_id());
                    ptr->run();
                });
            }
        }

        // Run primary window in main thread
        WaylandWindow *mainWin = m_managed_windows[0].window.get();
        mainWin->w_surface()->init_display();
        mainWin->w_surface()->setup_xdg_toplevel(mainWin->name(), mainWin->app_id());
        mainWin->run();

        // Shutdown sequence
        m_is_running = false;
        
        {
            std::lock_guard<std::mutex> lock(m_windows_mutex);
            for (auto &mw : m_managed_windows) {
                mw.window->quit();
            }
        }

        for (auto &t : m_window_threads)
        {
            if (t.joinable())
                t.join();
        }
    }

} // namespace horizon
