#include "DownloaderWindow.hpp"
#include "NewDownloadDialog.hpp"
#include "horizon/download/DownloadView.hpp"
#include "horizon/download/DownloadManager.hpp"
#include "horizon/Toolbar.hpp"
#include "horizon/ToolbarButton.hpp"
#include "horizon/Application.hpp"
#include <thread>

namespace horizon {
namespace downloader {

DownloaderWindow::DownloaderWindow()
    : ApplicationWindow("Gestor de Descargas") {
    
    set_size(1000, 600);
    setup_ui();
}

void DownloaderWindow::setup_ui() {
    auto tb = toolbar();
    tb->set_bottom_height(58);

    auto add_btn = std::make_unique<horizon::ToolbarButton>("Nueva", "list-add-symbolic");
    add_btn->when_click.connect([this](horizon::MouseButtonEventContext&) {
        auto dialog = std::make_shared<NewDownloadDialog>([this](std::string url) {
            this->application()->post_task([this, url]() {
                download::DownloadManager::instance().add_download(url);
            });
        });
        std::thread([dialog]() {
            dialog->initialize();
            dialog->run();
        }).detach();
    });
    tb->add_toolbar_widget(std::move(add_btn));

    auto pause_all = std::make_unique<horizon::ToolbarButton>("Pausar todo", "media-playback-pause-symbolic");
    pause_all->when_click.connect([this](horizon::MouseButtonEventContext&) {
        download::DownloadManager::instance().pause_all();
    });
    tb->add_toolbar_widget(std::move(pause_all));

    auto resume_all = std::make_unique<horizon::ToolbarButton>("Reanudar todo", "media-playback-start-symbolic");
    resume_all->when_click.connect([this](horizon::MouseButtonEventContext&) {
        download::DownloadManager::instance().resume_all();
    });
    tb->add_toolbar_widget(std::move(resume_all));

    auto clear_btn = std::make_unique<horizon::ToolbarButton>("Limpiar", "edit-clear-symbolic");
    clear_btn->when_click.connect([this](horizon::MouseButtonEventContext&) {
        // Clear logic could be implemented in DownloadManager if needed
    });
    tb->add_toolbar_widget(std::move(clear_btn));

    set_content(std::make_unique<download::DownloadView>());
}

void DownloaderWindow::refresh_list() {
    // List refreshes automatically in DownloadView
}

} // namespace downloader
} // namespace horizon
