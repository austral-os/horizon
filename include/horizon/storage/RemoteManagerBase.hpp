#pragma once
#include <string>
#include <vector>
#include <functional>
#include <horizon/EventsManager.hpp>

namespace horizon::storage
{
    struct RemoteStorageEventContext : public EventContext {};

    struct RemoteMountInfo
    {
        std::string name;
        std::string uri;
        std::string mount_path;
        std::string icon_name;
    };

    struct RemoteCredentials
    {
        bool is_guest = false;
        std::string username;
        std::string password;
        bool remember = false;
    };

    struct RemoteMountResult
    {
        bool success;
        std::string message;
        std::string mount_path;
    };

    class RemoteManagerBase
    {
    public:
        virtual ~RemoteManagerBase() = default;

        virtual void when_mount(const std::string& uri, 
                           const RemoteCredentials& credentials,
                           std::function<void(RemoteMountResult)> callback) = 0;

        virtual void when_unmount(const std::string& mount_path, std::function<void(bool, std::string)> callback) = 0;
        virtual void when_unmount_by_uri(const std::string& uri, std::function<void(bool, std::string)> callback) = 0;

        virtual std::vector<RemoteMountInfo> get_active_mounts() = 0;
        
        virtual void save_credentials(const std::string& uri, const RemoteCredentials& creds) = 0;
        virtual bool get_credentials(const std::string& uri, RemoteCredentials& out_creds) = 0;

        EventsManager<RemoteStorageEventContext> when_changed;
    };
}
