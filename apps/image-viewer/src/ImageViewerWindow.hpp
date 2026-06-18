#pragma once

#include <horizon/ApplicationWindow.hpp>
#include <horizon/TabCollection.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/image/ImageWidget.hpp>
#include <string>
#include <vector>
#include <memory>

namespace horizon {
    class Label;
    class ProgressBar;
}

namespace horizon {
namespace image {

class ImageViewerToolbar;

class CroppableImageWidget;

/**
 * @class ImageViewerTabContent
 * @brief Represents the content of a single tab in the image viewer.
 */
class ImageViewerTabContent : public Widget {
public:
    ImageViewerTabContent(const std::string& path);
    
    void open_file(const std::string& path);
    void navigate(int direction);
    
    CroppableImageWidget* image_widget() const { return m_image_widget; }
    const std::string& current_path() const { return m_current_path; }

private:
    void scan_directory();
    
    std::string m_current_path;
    std::vector<std::string> m_directory_files;
    int m_current_index{-1};
    
    CroppableImageWidget* m_image_widget{nullptr};
    ScrollArea* m_scroll_area{nullptr};
};

/**
 * @class ImageViewerWindow
 * @brief The main window for the image viewer application.
 */
class ImageViewerWindow : public ApplicationWindow {
public:
    ImageViewerWindow();
    virtual ~ImageViewerWindow();

    void open_file(const std::string& path);
    uint32_t file_capabilities() const override { return FileOpen | FileClose | FileSave | FileSaveAs; }
    std::string current_file_path() const override;
    
    bool supports_undo() const override { return true; }

private:
    void setup_ui();
    void setup_toolbar();
    
    ImageViewerTabContent* current_content() const;

    // Slots for toolbar events
    void on_open_clicked();
    void on_navigation_clicked(int button_index);
    void on_zoom_clicked(int button_index);
    void on_transform_clicked(int button_index);
    void on_extra_clicked(int button_index);
    void on_crop_clicked();
    void on_save_clicked();
    void on_undo_clicked();
    void on_redo_clicked();

    TabCollection* m_tabs{nullptr};
    ImageViewerToolbar* m_toolbar_widget{nullptr};
    horizon::Label* m_status_label{nullptr};
    horizon::ProgressBar* m_progress_bar{nullptr};
};

} // namespace image
} // namespace horizon
