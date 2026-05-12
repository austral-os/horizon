#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <horizon/EventsManager.hpp>

namespace horizon::storage
{
    struct RemoteStorageEventContext : public EventContext
    {
    };

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

    class RemoteManager
    {
    public:
        RemoteManager();
        ~RemoteManager();

        void when_mount(const std::string& uri, 
                   const RemoteCredentials& credentials,
                   std::function<void(RemoteMountResult)> callback);

        void when_unmount(const std::string& mount_path, std::function<void(bool, std::string)> callback);
        void when_unmount_by_uri(const std::string& uri, std::function<void(bool, std::string)> callback);

        std::vector<RemoteMountInfo> get_active_mounts();

        // Signal triggered when mounts change
        EventsManager<RemoteStorageEventContext> when_changed;

    private:
        struct Private;
        std::unique_ptr<Private> d;
    };
}
