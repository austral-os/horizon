#include "DockApplets.hpp"
#include "DockApplication.hpp"
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/Logger.hpp>
#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/Menu.hpp>
#include <horizon/Vault.hpp>
#include <horizon/CairoGraphicsContext.hpp>
#include <horizon/IconThemeLookup.hpp>
#include <horizon/files/FileIconProvider.hpp>
#include <horizon/arkutils/FileInfo.hpp>
#include <filesystem>
#include <cmath>

namespace horizon {

DockApplet::DockApplet(DockApplication* app, const std::string& name, const std::string& icon_name)
    : m_app(app), m_name(name)
{
    set_icon_name(icon_name);
}

// --- TrashApplet ---
TrashApplet::TrashApplet(DockApplication* app) : DockApplet(app, "Trash", "user-trash")
{
    update_icon();
    
    // Check trash status periodically
    m_timer_id = m_app->window()->add_timer(5000, [this]() {
        update_icon();
    }, true);

    when_click.connect([this](auto&) {
        std::string trash_path = "trash:///";
        ApplicationLauncher::launch_binary("arkfm", {trash_path});
    });

    when_drop.connect([this](auto& ctx) {
        std::string trash_path = std::string(getenv("HOME")) + "/.local/share/Trash/files";
        std::filesystem::create_directories(trash_path);
        auto uri_list = ctx.get_data_as_string("text/uri-list");
        LOG_INFO << "[TrashApplet] Drop received: " << uri_list;
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

// --- DownloadsApplet & ParabolaVault ---

class ParabolaVault : public Vault {
public:
    struct Item {
        std::string path;       // file path or "OPEN_IN_ARKFM"
        std::string icon_path;  // resolved icon path
        std::string label;
    };

    ParabolaVault(int max_items, WaylandWindow* window) : m_window(window) {
        set_size(m_vault_w, m_vault_h);

        // Collect recent downloads
        std::string dl_path = std::string(getenv("HOME")) + "/Downloads";
        std::vector<std::filesystem::directory_entry> entries;
        if (std::filesystem::exists(dl_path)) {
            for (const auto& entry : std::filesystem::directory_iterator(dl_path)) {
                if (entry.is_regular_file() || entry.is_directory()) {
                    entries.push_back(entry);
                }
            }
            std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
                return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
            });
            int count = 0;
            for (const auto& entry : entries) {
                if (count >= max_items) break;
                Item item;
                item.path = entry.path().string();
                auto info = arkutils::FileInfo::from_path(item.path);
                std::string icon_name = files::FileIconProvider::get_icon_name(info);
                item.icon_path = IconThemeLookup::find_icon(icon_name, 48);
                std::string fname = entry.path().filename().string();
                if (fname.length() > 12) fname = fname.substr(0, 10) + "..";
                item.label = fname;
                m_items.push_back(std::move(item));
                count++;
            }
        }

        // "Open Downloads folder" item always first (leftmost)
        Item open_item;
        open_item.path = "OPEN_IN_ARKFM";
        open_item.icon_path = IconThemeLookup::find_icon("folder-downloads", 48);
        if (open_item.icon_path.empty())
            open_item.icon_path = IconThemeLookup::find_icon("folder", 48);
        open_item.label = "Descargas";
        m_items.insert(m_items.begin(), std::move(open_item));

        // Start animation
        m_animation_timer = m_window->add_timer(16, [this]() {
            m_progress += 0.06f;
            if (m_progress >= 1.0f) {
                m_progress = 1.0f;
                m_window->stop_timer(m_animation_timer);
                m_animation_timer = 0;
            }
            invalidate();
        }, true);

        // Handle clicks
        when_mouse_press.connect([this](auto& ctx) {
            handle_click(ctx.x, ctx.y);
        });
    }

    ~ParabolaVault() {
        if (m_animation_timer) m_window->stop_timer(m_animation_timer);
    }

    int preferred_width() const override { return m_vault_w; }
    int preferred_height() const override { return m_vault_h; }

    void calculate_layout() override {
        set_size(m_vault_w, m_vault_h);
        m_start_draw_x = m_x;
        m_start_draw_y = m_y;
    }

protected:
    void draw(GraphicsContext& gc) override {
        int n = (int)m_items.size();
        if (n == 0) return;

        double ease = double(m_progress) * (2.0 - double(m_progress)); // easeOutQuad

        // Bottom-center anchor (where items fan out from)
        double bx = m_x + m_vault_w / 2.0;
        double by = m_y + m_vault_h - 20.0;

        // macOS-style parabolic fan:
        // X = t^1.5 (starts moving right sooner to avoid vertical overlap)
        // Y = linear (uniform vertical rise)
        // Fan starts directly above Downloads icon (t=0) and sweeps upper-right (t=1)
        for (int i = 0; i < n; ++i) {
            double t = (n > 1) ? (double)(n - 1 - i) / (n - 1) : 0.0;
            double tx = bx + std::pow(t, 1.5) * 230.0;  // moves right faster initially
            double ty = by - 20.0 - t * 330.0;           // taller linear rise to prevent vertical overlap

            // Animate from anchor toward target
            double cx = bx + (tx - bx) * ease;
            double cy = by + (ty - by) * ease;

            float alpha = (float)ease;
            int icon_x = (int)(cx - m_icon_size / 2.0);
            int icon_y = (int)(cy - m_icon_size / 2.0);

            // Shadow/bubble background
            gc.setColor({0.1f, 0.1f, 0.1f, alpha * 0.35f});
            gc.fillCircle((int)cx, (int)cy, m_icon_size/2 + 4);

            // Draw icon
            if (!m_items[i].icon_path.empty()) {
                gc.drawImage(m_items[i].icon_path, icon_x, icon_y, m_icon_size, m_icon_size, alpha);
            } else {
                gc.setColor({0.5f, 0.7f, 1.0f, alpha});
                gc.fillCircle((int)cx, (int)cy, m_icon_size/2);
            }

            // Label below icon
            if (alpha > 0.3f) {
                gc.setDrawFont("Inter", 10, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
                auto tm = gc.getTextMetrics(m_items[i].label.c_str(), "Inter", 10, FONT_SLANT_NORMAL, FONT_WEIGHT_NORMAL);
                int lx = (int)(cx - tm.width / 2.0);
                int ly = (int)(cy + m_icon_size/2 + 14);
                // Label background
                gc.setColor({0.0f, 0.0f, 0.0f, alpha * 0.55f});
                gc.fillRect(lx - 3, ly - 11, tm.width + 6, 15, 4);
                gc.setColor({1.0f, 1.0f, 1.0f, alpha});
                gc.drawText(lx, ly, m_items[i].label.c_str());
            }
        }
    }

    Widget* hit_test(int x, int y) override {
        // Only respond to clicks, no children to delegate to
        if (x >= m_x && x < m_x + m_vault_w && y >= m_y && y < m_y + m_vault_h)
            return this;
        return nullptr;
    }

private:
    void handle_click(int mx, int my) {
        int n = (int)m_items.size();
        if (n == 0 || m_progress < 0.8f) return;

        double bx = m_x + m_vault_w / 2.0;
        double by = m_y + m_vault_h - 20.0;

        for (int i = 0; i < n; ++i) {
            double t = (n > 1) ? (double)(n - 1 - i) / (n - 1) : 0.0;
            double tx = bx + std::pow(t, 1.5) * 230.0;
            double ty = by - 20.0 - t * 330.0;

            double dx = mx - tx;
            double dy = my - ty;
            if (dx*dx + dy*dy <= double((m_icon_size/2 + 8) * (m_icon_size/2 + 8))) {
                if (m_items[i].path == "OPEN_IN_ARKFM") {
                    std::string dl = std::string(getenv("HOME")) + "/Downloads";
                    ApplicationLauncher::launch_binary("arkfm", {dl});
                } else {
                    ApplicationLauncher::launch_binary("xdg-open", {m_items[i].path});
                }
                m_window->close_vault();
                return;
            }
        }
    }

    static constexpr int m_vault_w = 520;
    static constexpr int m_vault_h = 400;
    static constexpr int m_icon_size = 48;

    WaylandWindow* m_window;
    std::vector<Item> m_items;
    uint32_t m_animation_timer = 0;
    float m_progress = 0.0f;
};


static std::unique_ptr<ParabolaVault> g_parabola_vault;


DownloadsApplet::DownloadsApplet(DockApplication* app) : DockApplet(app, "Downloads", "folder-downloads")
{
    when_click.connect([this](auto& ctx) {
        g_parabola_vault = std::make_unique<ParabolaVault>(m_max_items, m_app->window());
        // Center vault horizontally over the Downloads icon (520px wide)
        int vault_x = ctx.x - 260;
        m_app->window()->show_vault(g_parabola_vault.get(), vault_x, ctx.y, ctx.serial, this);
    });

    when_drop.connect([this](auto& ctx) {
        std::string dl_path = std::string(getenv("HOME")) + "/Downloads";
        // Logic to move dropped files
        LOG_INFO << "[DownloadsApplet] Drop received";
    });
}

DownloadsApplet::~DownloadsApplet()
{
}

void DownloadsApplet::load_config(const nlohmann::json& config)
{
    if (config.contains("downloads_items_count")) {
        m_max_items = config["downloads_items_count"].get<int>();
    }
}

} // namespace horizon
