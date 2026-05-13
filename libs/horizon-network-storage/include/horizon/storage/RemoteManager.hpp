#pragma once
#include <horizon/storage/RemoteManagerBase.hpp>
#include <memory>

namespace horizon::storage
{
    class RemoteManager : public RemoteManagerBase
    {
    public:
        RemoteManager();
        ~RemoteManager() override;

        void when_mount(const std::string& uri, 
                   const RemoteCredentials& credentials,
                   std::function<void(RemoteMountResult)> callback) override;

        void when_unmount(const std::string& mount_path, std::function<void(bool, std::string)> callback) override;
        void when_unmount_by_uri(const std::string& uri, std::function<void(bool, std::string)> callback) override;

        std::vector<RemoteMountInfo> get_active_mounts() override;

    private:
        struct Private;
        std::unique_ptr<Private> d;
    };
}
