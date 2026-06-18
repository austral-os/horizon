#include "ImageViewerWindow.hpp"
#include "ImageViewerToolbar.hpp"
#include "CroppableImageWidget.hpp"
#include <horizon/Statusbar.hpp>
#include <horizon/Label.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/I18n.hpp>
#include <horizon/dialogs/FileDialog.hpp>
#include <horizon/Application.hpp>
#include <horizon/Logger.hpp>
#include <filesystem>
#include <algorithm>
#include <sstream>

namespace fs = std::filesystem;

namespace horizon {
namespace image {

// --- ImageViewerTabContent Implementation ---

ImageViewerTabContent::ImageViewerTabContent(const std::string& path) {
    set_layout_type(WIDGET_LAYOUT_VERTICAL);
    set_position_type(FILL);
    
    auto scroll = std::make_unique<ScrollArea>();
    m_scroll_area = scroll.get();
    m_scroll_area->set_position_type(FILL);
    
    auto widget = std::make_unique<CroppableImageWidget>();
    m_image_widget = widget.get();
    
    m_scroll_area->set_content(std::move(widget));
    add_child(std::move(scroll));
    
    open_file(path);
}

void ImageViewerTabContent::open_file(const std::string& path) {
    if (path.empty()) return;
    
    m_current_path = path;
    if (m_image_widget) {
        m_image_widget->set_path(path);
    }
    scan_directory();
    
    // Invalida para redibujar con la nueva imagen
    invalidate();
}

void ImageViewerTabContent::scan_directory() {
    m_directory_files.clear();
    m_current_index = -1;
    
    try {
        fs::path p(m_current_path);
        fs::path dir = p.parent_path();
        
        if (!fs::exists(dir)) return;

        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp" || ext == ".svg") {
                    m_directory_files.push_back(entry.path().string());
                }
            }
        }
        
        std::sort(m_directory_files.begin(), m_directory_files.end());
        
        auto it = std::find(m_directory_files.begin(), m_directory_files.end(), m_current_path);
        if (it != m_directory_files.end()) {
            m_current_index = std::distance(m_directory_files.begin(), it);
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to scan directory: " << e.what();
    }
}

void ImageViewerTabContent::navigate(int direction) {
    if (m_directory_files.empty() || m_current_index == -1) return;
    
    int next_index = m_current_index + direction;
    if (next_index < 0) next_index = m_directory_files.size() - 1;
    if (next_index >= (int)m_directory_files.size()) next_index = 0;
    
    open_file(m_directory_files[next_index]);
}

// --- ImageViewerWindow Implementation ---

ImageViewerWindow::ImageViewerWindow() : ApplicationWindow(i18n().tr("app.title")) {
    set_size(1024, 768);
    setup_ui();
    setup_toolbar();

    show_status_bar();
    auto *sb = statusbar();
    
    auto lbl = std::make_unique<horizon::Label>("");
    m_status_label = lbl.get();

    auto pbc = std::make_unique<horizon::Widget>();
    auto pb = std::make_unique<horizon::ProgressBar>();

    m_progress_bar = pb.get();
    m_progress_bar->set_visible(false);
    m_progress_bar->set_fixed_size(10);
    m_progress_bar->set_indeterminate(true);

    pbc->set_layout_type(WidgetLayoutTypes::WIDGET_LAYOUT_VERTICAL);
    pbc->set_fixed_size(200);

    pbc->add_child(horizon::Spacer());
    pbc->add_child(std::move(pb));
    pbc->add_child(horizon::Spacer());

    sb->add_child(horizon::Spacer(10));
    sb->add_child(std::move(lbl));
    sb->add_child(std::move(pbc));
    sb->add_child(horizon::Spacer(10));

    // Conectar eventos estándar de archivos
    when_file_opened.connect([this](Window::FileOpenedContext& ctx) {
        this->open_file(ctx.path);
    });

    when_file_close.connect([this](EventContext&) {
        if (m_tabs && m_tabs->tab_count() > 0) {
            m_tabs->remove_tab(m_tabs->current_tab_index());
        }
    });

    when_save.connect([this](Window::FileSaveContext& ctx) {
        auto* content = current_content();
        if (content && content->image_widget()) {
            content->image_widget()->save_to_path(ctx.path);
        }
    });

    when_save_as.connect([this](Window::FileSaveContext& ctx) {
        auto* content = current_content();
        if (content && content->image_widget()) {
            content->image_widget()->save_to_path(ctx.path);
            content->open_file(ctx.path);
        }
    });

    when_undo.connect([this](EventContext&) {
        auto* content = current_content();
        if (content && content->image_widget()) {
            content->image_widget()->undo_crop();
        }
    });

    when_redo.connect([this](EventContext&) {
        auto* content = current_content();
        if (content && content->image_widget()) {
            content->image_widget()->redo_crop();
        }
    });

    set_accept_drops(true);
    when_drop.connect([this](DropEventContext &ctx) {
        auto data = ctx.get_data("text/uri-list");
        if (!data.empty())
        {
            std::string uris(data.begin(), data.end());
            std::stringstream ss(uris);
            std::string line;
            if (std::getline(ss, line))
            {
                if (line.substr(0, 7) == "file://")
                    line = line.substr(7);
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();

                open_file(line);
            }
        }
    });
}

ImageViewerWindow::~ImageViewerWindow() {}

void ImageViewerWindow::setup_ui() {
    auto tabs = std::make_unique<TabCollection>();
    m_tabs = tabs.get();
    m_tabs->set_smart_header(true);
    m_tabs->set_closable_tabs(true);
    
    m_tabs->when_add_tab_clicked.connect([this](EventContext&) {
        this->on_open_clicked();
    });
    
    m_tabs->when_tab_close_requested.connect([this](int index) {
        if (application()) {
            application()->post_task([this, index]() {
                m_tabs->remove_tab(index);
            });
        }
    });

    m_tabs->when_tab_selected.connect([this](int index) {
        auto* content = current_content();
        if (content && content->image_widget()) {
            content->image_widget()->set_focus(true);
        }
    });

    set_content(std::move(tabs));
}

void ImageViewerWindow::setup_toolbar() {
    auto tb = toolbar();
    if (!tb) return;

    auto viewer_tb = std::make_unique<ImageViewerToolbar>();
    m_toolbar_widget = viewer_tb.get();

    m_toolbar_widget->when_open_clicked.connect([this](EventContext&) {
        this->on_open_clicked();
    });

    m_toolbar_widget->when_save_clicked.connect([this](EventContext&) {
        this->on_save_clicked();
    });

    m_toolbar_widget->when_undo_clicked.connect([this](EventContext&) {
        this->on_undo_clicked();
    });

    m_toolbar_widget->when_redo_clicked.connect([this](EventContext&) {
        this->on_redo_clicked();
    });

    m_toolbar_widget->when_crop_clicked.connect([this](EventContext&) {
        this->on_crop_clicked();
    });

    m_toolbar_widget->when_navigation_clicked.connect([this](GroupButtonClickEvent& ev) {
        this->on_navigation_clicked(ev.button_index);
    });

    m_toolbar_widget->when_zoom_clicked.connect([this](GroupButtonClickEvent& ev) {
        this->on_zoom_clicked(ev.button_index);
    });

    m_toolbar_widget->when_transform_clicked.connect([this](GroupButtonClickEvent& ev) {
        this->on_transform_clicked(ev.button_index);
    });

    m_toolbar_widget->when_extra_clicked.connect([this](GroupButtonClickEvent& ev) {
        this->on_extra_clicked(ev.button_index);
    });

    tb->add_toolbar_widget(std::move(viewer_tb));
}

ImageViewerTabContent* ImageViewerWindow::current_content() const {
    if (!m_tabs) return nullptr;
    return dynamic_cast<ImageViewerTabContent*>(m_tabs->current_tab_body());
}

std::string ImageViewerWindow::current_file_path() const {
    auto* content = current_content();
    if (content) {
        return content->current_path();
    }
    return "";
}

void ImageViewerWindow::open_file(const std::string& path) {
    auto content = std::make_unique<ImageViewerTabContent>(path);
    auto* img = content->image_widget();
    
    img->when_load_started.connect([this](EventContext&) {
        if (m_progress_bar) m_progress_bar->set_visible(true);
        if (m_status_label) m_status_label->set_text(i18n().tr("dialog.open_image"));
    });

    img->when_load_finished.connect([this](EventContext&) {
        if (m_progress_bar) m_progress_bar->set_visible(false);
        if (m_status_label) m_status_label->set_text("");
    });

    auto* croppable = dynamic_cast<CroppableImageWidget*>(img);
    if (croppable) {
        croppable->when_operation_started.connect([this](EventContext&) {
            if (m_progress_bar) m_progress_bar->set_visible(true);
            if (m_status_label) m_status_label->set_text("Procesando imagen...");
        });
        croppable->when_operation_finished.connect([this](EventContext&) {
            if (m_progress_bar) m_progress_bar->set_visible(false);
            if (m_status_label) m_status_label->set_text("");
        });
        croppable->when_crop_mode_changed.connect([this, croppable](EventContext&) {
            if (m_status_label) {
                if (croppable->is_cropping()) {
                    m_status_label->set_text(i18n().tr("statusbar.crop_mode"));
                } else {
                    m_status_label->set_text("");
                }
            }
        });
    }
    
    fs::path p(path);
    std::string title = p.filename().string();
    
    int index = m_tabs->add_tab(title, std::move(content));
    m_tabs->set_current_tab(index);
    
    // Asignar foco al widget de imagen para soporte de fullscreen
    auto* cur = current_content();
    if (cur && cur->image_widget()) {
        cur->image_widget()->set_focus(true);
    }
    
    set_title(title + " - " + i18n().tr("app.title"));
}

void ImageViewerWindow::on_open_clicked() {
    // Usamos el sistema automatizado emitiendo la señal al manejador global
    if (application()) {
        application()->signal_manager.emit("file.open");
    }
}

void ImageViewerWindow::on_navigation_clicked(int button_index) {
    auto* content = current_content();
    if (!content) return;
    
    if (button_index == 0) content->navigate(-1); // Prev
    else if (button_index == 1) content->navigate(1); // Next
    
    // Update window title after navigation
    fs::path p(content->current_path());
    set_title(p.filename().string() + " - " + i18n().tr("app.title"));
    m_tabs->set_tab_title(m_tabs->current_tab_index(), p.filename().string());
}

void ImageViewerWindow::on_zoom_clicked(int button_index) {
    auto* content = current_content();
    if (!content) return;
    auto* img = content->image_widget();
    if (!img) return;

    if (button_index == 0) img->zoom_out();
    else if (button_index == 1) img->zoom_in();
    else if (button_index == 2) img->zoom_fit(content->width(), content->height());
    else if (button_index == 3) img->original_size();
}

void ImageViewerWindow::on_transform_clicked(int button_index) {
    auto* content = current_content();
    if (!content) return;
    auto* img = content->image_widget();
    if (!img) return;

    if (button_index == 0) img->rotate_ccw();
    else if (button_index == 1) img->rotate_cw();
}

void ImageViewerWindow::on_extra_clicked(int button_index) {
    if (button_index == 0) { // Fullscreen
        if (application()) application()->signal_manager.emit("fullscreen");
    } else if (button_index == 1) { // Settings
        if (application()) application()->show_preferences();
    }
}

void ImageViewerWindow::on_crop_clicked() {
    auto* content = current_content();
    if (!content) return;
    auto* img = content->image_widget();
    if (img) img->toggle_crop_mode();
}

void ImageViewerWindow::on_save_clicked() {
    if (application()) {
        application()->signal_manager.emit("file.save");
    }
}

void ImageViewerWindow::on_undo_clicked() {
    if (application()) {
        application()->signal_manager.emit("undo");
    }
}

void ImageViewerWindow::on_redo_clicked() {
    if (application()) {
        application()->signal_manager.emit("redo");
    }
}

} // namespace image
} // namespace horizon
