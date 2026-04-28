#pragma once

#include <horizon/WaylandWindow.hpp>
#include <functional>
#include <string>

namespace horizon {
namespace downloader {

class NewDownloadDialog : public horizon::WaylandWindow {
public:
    NewDownloadDialog(std::function<void(std::string)> on_accept);
    virtual ~NewDownloadDialog() = default;

private:
    std::function<void(std::string)> m_on_accept;
};

} // namespace downloader
} // namespace horizon
