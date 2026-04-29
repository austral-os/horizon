#pragma once

#include "horizon/ApplicationWindow.hpp"
#include "horizon/download/DownloadManager.hpp"

namespace horizon {
namespace downloader {

class DownloaderWindow : public horizon::ApplicationWindow {
public:
    DownloaderWindow();
    virtual ~DownloaderWindow() = default;

private:
    void setup_ui();
    void refresh_list();
};

} // namespace downloader
} // namespace horizon
