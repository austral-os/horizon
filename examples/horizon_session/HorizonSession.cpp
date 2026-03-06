#include "HorizonSession.hpp"
#include "DesktopParser.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

HorizonSession::HorizonSession() : m_server_socket_path("/tmp/horizon_session.sock")
{
    // Initialize IPC Server
    m_server = std::make_unique<horizon::IpcServer>(
        m_server_socket_path,
        [this](const std::string &msg) -> std::string
        {
            // Placeholder: Parse and handle incoming messages
            // Depending on the message, it could be a pub/sub request or a state change
            LOG_INFO << "[HorizonSession] Received IPC msg: " << msg << std::endl;
            return "{\"status\": \"ok\"}";
        });
}

HorizonSession::~HorizonSession()
{
    stop();
}

void HorizonSession::init(bool with_compositor)
{
    // Make sure we initialize the logger file output
    Logger::instance().init("/tmp/horizon_session.log");
    LOG_INFO << "[HorizonSession] Initializing..." << std::endl;

    if (with_compositor)
    {
        m_startup_services.push_back("labwc");
    }

    // Example of default core services
    m_startup_services.push_back("/home/horacio/Desarrollo/austral-os/horizon/build/horizon_wall");
    m_startup_services.push_back(
        "/home/horacio/Desarrollo/austral-os/horizon/build/horizon_menu_manager_d");
    m_startup_services.push_back("/home/horacio/Desarrollo/austral-os/horizon/build/top_panel");
    m_startup_services.push_back("/home/horacio/Desarrollo/austral-os/horizon/build/dock");
}

void HorizonSession::start()
{
    LOG_INFO << "[HorizonSession] Starting IPC Server..." << std::endl;
    m_server->start();
    run_startup_services();
}

void HorizonSession::stop()
{
    m_server->stop();
}

void HorizonSession::run_app(const std::string &app_name)
{
    LOG_INFO << "[HorizonSession] Executing app: " << app_name << std::endl;

    auto entry = DesktopParser::parse(app_name);
    if (!entry)
    {
        LOG_ERROR << "[HorizonSession] Failed to parse desktop entry for: " << app_name
                  << std::endl;
        return;
    }

    run_service(entry->exec);
}

void HorizonSession::run_service(const std::string &service_path)
{
    LOG_INFO << "[HorizonSession] Executing service: " << service_path << std::endl;

    pid_t pid = fork();

    if (pid == 0)
    {
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        char *argv[] = {(char *)service_path.c_str(), nullptr};
        execvp(service_path.c_str(), argv);

        LOG_ERROR << "[HorizonSession] Failed to exec " << service_path << ": " << strerror(errno)
                  << std::endl;
        exit(1);
    }
    else if (pid > 0)
    {
        LOG_INFO << "[HorizonSession] Successfully spawned " << service_path << " with PID " << pid
                 << std::endl;
    }
    else
    {
        LOG_ERROR << "[HorizonSession] Failed to fork for " << service_path << ": "
                  << strerror(errno) << std::endl;
    }
}

void HorizonSession::run_startup_services()
{
    for (const auto &svc_path : m_startup_services)
    {
        run_service(svc_path);
    }
}

void HorizonSession::add_app(int pid, const AppInfo &app)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_apps[pid] = app;
}

void HorizonSession::remove_app(int pid)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_apps.erase(pid);

    // Remove from subscriptions
    for (auto &[event, pids] : m_event_subscribers)
    {
        auto it = std::find(pids.begin(), pids.end(), pid);
        if (it != pids.end())
        {
            pids.erase(it);
        }
    }
}

std::optional<AppInfo> HorizonSession::get_app(int pid)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    auto it = m_apps.find(pid);
    if (it != m_apps.end())
    {
        return it->second;
    }
    return std::nullopt;
}

void HorizonSession::send_message(const HznSessionMessage &message)
{
    nlohmann::json j;
    j["sender_pid"] = message.sender_pid;
    j["receiver_pid"] = message.receiver_pid;
    j["sender_id"] = message.sender_id;
    j["receiver_id"] = message.receiver_id;
    j["message"] = message.message;

    m_server->broadcast(j.dump()); // Broadcast or target via specific mechanisms
}

bool HorizonSession::is_subscribed(int pid, HznSessionEvent event)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    auto it = m_event_subscribers.find(event);
    if (it != m_event_subscribers.end())
    {
        const auto &pids = it->second;
        return std::find(pids.begin(), pids.end(), pid) != pids.end();
    }
    return false;
}

void HorizonSession::subscribe(int pid, HznSessionEvent event)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    auto &pids = m_event_subscribers[event];
    if (std::find(pids.begin(), pids.end(), pid) == pids.end())
    {
        pids.push_back(pid);
        // Also add to app's personal list if needed, but not strictly necessary now
    }
}

void HorizonSession::unsubscribe(int pid, HznSessionEvent event)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    auto it = m_event_subscribers.find(event);
    if (it != m_event_subscribers.end())
    {
        auto &pids = it->second;
        auto pid_it = std::find(pids.begin(), pids.end(), pid);
        if (pid_it != pids.end())
        {
            pids.erase(pid_it);
        }
    }
}

std::vector<AppInfo> HorizonSession::get_subscribers(HznSessionEvent event)
{
    std::lock_guard<std::mutex> lock(m_state_mutex);
    std::vector<AppInfo> result;
    auto it = m_event_subscribers.find(event);
    if (it != m_event_subscribers.end())
    {
        for (int pid : it->second)
        {
            auto app_it = m_apps.find(pid);
            if (app_it != m_apps.end())
            {
                result.push_back(app_it->second);
            }
        }
    }
    return result;
}

void HorizonSession::send_to_subscribers(HznSessionEvent event, const HznSessionMessage &message)
{
    std::vector<int> target_pids;
    {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        auto it = m_event_subscribers.find(event);
        if (it != m_event_subscribers.end())
        {
            target_pids = it->second;
        }
    }

    for (int pid : target_pids)
    {
        HznSessionMessage target_msg = message;
        target_msg.receiver_pid = pid;
        send_message(target_msg);
    }
}
