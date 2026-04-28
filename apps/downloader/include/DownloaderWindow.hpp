#pragma once

#include "horizon/ApplicationWindow.hpp"
#include "horizon/download/DownloadManager.hpp"
#include "horizon/TableView.hpp"

namespace horizon {
namespace downloader {

class DownloaderWindow : public horizon::ApplicationWindow {
public:
    DownloaderWindow();
    virtual ~DownloaderWindow() = default;

private:
    void setup_ui();
    void refresh_list();

    horizon::TableView<std::shared_ptr<download::DownloadTask>>* m_table = nullptr;
};

} // namespace downloader
} // namespace horizon
