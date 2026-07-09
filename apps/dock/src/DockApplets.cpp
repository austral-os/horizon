#include "DockApplets.hpp"
#include "DockApplication.hpp"
#include "DockShelf.hpp"
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/Logger.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/Menu.hpp>
#include <horizon/Vault.hpp>
#include <horizon/CairoGraphicsContext.hpp>
#include <horizon/IconThemeLookup.hpp>
#include <horizon/files/FileIconProvider.hpp>
#include <horizon/arkutils/FileInfo.hpp>
#include <horizon/Matrix.hpp>
#include <filesystem>
#include <cmath>
#include <cstring>

namespace horizon {

DockApplet::DockApplet(DockApplication* app, const std::string& name, const std::string& icon_name)
    : m_app(app), m_name(name)
{
    set_icon_name(icon_name);
}

void DockApplet::draw(GraphicsContext& ctx)
{
    ctx.pushGroup();
    
    Icon::draw(ctx);
    
    uint32_t tex_id = 0;
    ctx.popGroupToTexture(tex_id, m_start_draw_x, m_start_draw_y, m_available_draw_width, m_available_draw_height);

    float mvp[16];
    Matrix::identity(mvp);
    
    // Ortho projection: maps [0, W]x[H, 0] to NDC [-1, 1]
    Matrix::ortho(mvp, 0, (float)m_app->window()->width(), (float)m_app->window()->height(), 0, -1, 1);
    
    float window_x = (float)m_start_draw_x;
    float window_y = (float)m_start_draw_y;

    float main_mvp[16];
    std::memcpy(main_mvp, mvp, 16 * sizeof(float));
    Matrix::translate(main_mvp, window_x + m_available_draw_width / 2.0f, 
                      window_y + m_available_draw_height / 2.0f, 0);
    Matrix::scale(main_mvp, m_available_draw_width / 2.0f, -m_available_draw_height / 2.0f, 1);
    
    float opacity = 1.0f;
    ctx.drawTexture3D(tex_id, m_available_draw_width, m_available_draw_height, main_mvp, opacity, false);

    std::string position = "bottom";
    DockShelf* shelf = dynamic_cast<DockShelf*>(parent());
    if (shelf) {
        position = shelf->dock_position();
    }

    // Draw Reflection
    float refl_mvp[16];
    std::memcpy(refl_mvp, mvp, 16 * sizeof(float));
    
    WaylandWindow::GLDrawCall refl_call;
    refl_call.texture_id = tex_id;
    refl_call.opacity = 0.5f;
    refl_call.delete_texture = true;
    refl_call.use_scissor = false;
    
    if (position == "left") {
        float refl_size = m_available_draw_width * 0.4f;
        Matrix::translate(refl_mvp, window_x - refl_size / 2.0f, 
                          window_y + m_available_draw_height / 2.0f, 0);
        Matrix::scale(refl_mvp, -refl_size / 2.0f, -m_available_draw_height / 2.0f, 1);
        
        refl_call.gradient_horizontal = true;
        refl_call.gradient_start = 1.0f; // Near icon
        refl_call.gradient_end = 0.0f;   // Far edge
    } else if (position == "right") {
        float refl_size = m_available_draw_width * 0.4f;
        Matrix::translate(refl_mvp, window_x + m_available_draw_width + refl_size / 2.0f, 
                          window_y + m_available_draw_height / 2.0f, 0);
        Matrix::scale(refl_mvp, -refl_size / 2.0f, -m_available_draw_height / 2.0f, 1);
        
        refl_call.gradient_horizontal = true;
        refl_call.gradient_start = 0.0f; // Far edge
        refl_call.gradient_end = 1.0f;   // Near icon
    } else {
        float refl_height = m_available_draw_height * 0.4f;
        Matrix::translate(refl_mvp, window_x + m_available_draw_width / 2.0f, 
                          window_y + m_available_draw_height + refl_height / 2.0f, 0);
        Matrix::scale(refl_mvp, m_available_draw_width / 2.0f, refl_height / 2.0f, 1);
        
        refl_call.gradient_horizontal = false;
        refl_call.gradient_start = 0.0f;
        refl_call.gradient_end = 1.0f;
    }
    
    std::memcpy(refl_call.mvp, refl_mvp, 16 * sizeof(float));
    m_app->window()->queue_gl_draw(refl_call);
}

// --- TrashApplet ---
TrashApplet::TrashApplet(DockApplication* app) : DockApplet(app, "Trash", "user-trash")
{
    update_icon();
    
    // Check trash status periodically
    m_timer_id = m_app->window()->add_timer(5000, [this]() {
        update_icon();
    }, true);

    set_accept_drops(true);

    when_click.connect([this](auto&) {
        std::string trash_path = "trash:///";
        ApplicationLauncher::launch_binary("arkfm", {trash_path});
    });

    when_drop.connect([this](auto& ctx) {
        std::string trash_path = std::string(getenv("HOME")) + "/.local/share/Trash/files";
        std::filesystem::create_directories(trash_path);
        
        auto uri_list = ctx.get_data_as_string("text/uri-list");
        if (uri_list.empty()) return;
        
        auto uri_decode = [](const std::string& encoded) {
            std::string decoded;
            decoded.reserve(encoded.size());
            for (size_t i = 0; i < encoded.size(); ++i) {
                if (encoded[i] == '%' && i + 2 < encoded.size()) {
                    int value;
                    std::istringstream is(encoded.substr(i + 1, 2));
                    if (is >> std::hex >> value) {
                        decoded += static_cast<char>(value);
                        i += 2;
                    } else {
                        decoded += encoded[i];
                    }
                } else {
                    decoded += encoded[i];
                }
            }
            return decoded;
        };

        std::stringstream ss(uri_list);
        std::string line;
        while(std::getline(ss, line)) {
            if (line.empty()) continue;
            if (line.back() == '\r') line.pop_back();
            
            std::string path;
            if (line.starts_with("file://")) {
                path = uri_decode(line.substr(7));
            } else {
                path = line;
            }
            
            try {
                if (std::filesystem::exists(path)) {
                    std::filesystem::rename(path, trash_path + "/" + std::filesystem::path(path).filename().string());
                }
            } catch(const std::exception& e) {
                LOG_ERROR << "[TrashApplet] Error moving " << path << " to trash: " << e.what();
            }
        }
        update_icon();
    });

    when_right_click.connect([this](auto& ctx) {
        // Show empty trash menu
        auto menu = std::make_unique<Menu>();
        menu->set_title("trash_context");
        auto* item = menu->add_item("Empty Trash");
        item->when_click.connect([this](auto&) {
            std::string trash_path = std::string(getenv("HOME")) + "/.local/share/Trash/files";
            std::string info_path = std::string(getenv("HOME")) + "/.local/share/Trash/info";
            try {
                std::filesystem::remove_all(trash_path);
                std::filesystem::create_directories(trash_path);
                std::filesystem::remove_all(info_path);
                std::filesystem::create_directories(info_path);
            } catch(...) {}
            update_icon();
        });
        m_app->window()->show_context_menu(menu.get(), ctx.x, ctx.y, ctx.serial, this);
        m_app->window()->add_menu(std::move(menu));
    });
}

TrashApplet::~TrashApplet()
{
    if (m_timer_id) m_app->window()->stop_timer(m_timer_id);
}

void TrashApplet::update_icon()
{
    std::string trash_path = std::string(getenv("HOME")) + "/.local/share/Trash/files";
    bool empty = true;
    if (std::filesystem::exists(trash_path)) {
        for (const auto& entry : std::filesystem::directory_iterator(trash_path)) {
            empty = false;
            break;
        }
    }
    set_icon_name(empty ? "user-trash" : "user-trash-full");
}



} // namespace horizon
