#include "LaunchpadWindow.hpp"
#include <algorithm>
#include <filesystem>
#include <horizon/Application.hpp>
#include <horizon/ApplicationLauncher.hpp>
#include <horizon/DesktopEntry.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Spacer.hpp>

namespace fs = std::filesystem;

namespace horizon
{

    LaunchpadWindow::LaunchpadWindow(Application* app) : Window(app, "Launchpad")
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_spacing(40);

        // Top spacer for some padding from the screen edge
        auto top_spacer = std::make_unique<Widget>();
        top_spacer->set_fixed_size(40);
        add_child(std::move(top_spacer));

        // Search Box Container (Horizontal to center the search box)
        auto search_container = std::make_unique<Widget>();
        search_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        search_container->set_fixed_size(48);

        // Left Spacer
        search_container->add_child(Spacer());

        // Search Box
        auto search_box = std::make_unique<SearchBox>();
        search_box->set_fixed_size(300);
        search_box->set_placeholder("Search Applications...");
        m_search_box = search_box.get();
        search_container->add_child(std::move(search_box));

        // Right Spacer
        search_container->add_child(Spacer());

        add_child(std::move(search_container));

        // Icon View
        auto icon_view = std::make_unique<IconView<AppData>>();
        icon_view->set_zoom(1.2f);
        icon_view->set_item_size(120, 140);
        icon_view->set_transparent(true);
        m_icon_view = icon_view.get();

        icon_view->set_item_factory(
            [this](const AppData &data, float zoom, bool selected)
            {
                auto item = std::make_unique<AppItem>();
                item->set_data(data, zoom, selected);
                item->set_font_size(14);
                return item;
            });

        icon_view->when_item_click.connect(
            [this](IconViewItemMouseClickContext<AppData> &ctx)
            {
                LOG_INFO << "Launching: " << ctx.item_data.name;
                ApplicationLauncher launcher;
                launcher.launch(ctx.item_data.exec);

                if (application())
                {
                    application()->quit();
                }
            });

        add_child(std::move(icon_view));

        // Filtering logic
        m_search_box->when_text_changed.connect([this](KeyEventContext &)
                                                { filter_apps(m_search_box->text()); });

        when_key_press.connect(
            [this](KeyEventContext &ctx)
            {
                if (ctx.keysym == 0xff1b) // XKB_KEY_Escape
                {
                    if (application())
                        application()->quit();
                }
            });

        load_apps();
    }

    void LaunchpadWindow::load_apps()
    {
        m_all_apps.clear();
        auto dirs = DesktopEntry::get_desktop_search_dirs();

        for (const auto &dir : dirs)
        {
            if (!fs::exists(dir) || !fs::is_directory(dir))
                continue;

            for (const auto &entry : fs::directory_iterator(dir))
            {
                if (entry.path().extension() == ".desktop")
                {
                    std::string path = entry.path().string();
                    std::string name = DesktopEntry::get_value_from_desktop_file(path, "Name");
                    std::string icon = DesktopEntry::get_value_from_desktop_file(path, "Icon");
                    std::string exec = DesktopEntry::get_value_from_desktop_file(path, "Exec");
                    std::string nodisplay =
                        DesktopEntry::get_value_from_desktop_file(path, "NoDisplay");

                    if (name.empty() || nodisplay == "true")
                        continue;

                    m_all_apps.push_back({name, icon, path});
                }
            }
        }

        // Sort alphabetically
        std::sort(m_all_apps.begin(), m_all_apps.end(),
                  [](const AppData &a, const AppData &b) { return a.name < b.name; });

        // Remove duplicates (based on name)
        auto last =
            std::unique(m_all_apps.begin(), m_all_apps.end(),
                        [](const AppData &a, const AppData &b) { return a.name == b.name; });
        m_all_apps.erase(last, m_all_apps.end());

        m_filtered_apps = m_all_apps;
        m_icon_view->set_data(m_filtered_apps);
    }

    void LaunchpadWindow::filter_apps(const std::string &query)
    {
        if (query.empty())
        {
            m_filtered_apps = m_all_apps;
        }
        else
        {
            m_filtered_apps.clear();
            std::string lower_query = query;
            std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

            for (const auto &app : m_all_apps)
            {
                std::string lower_name = app.name;
                std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
                if (lower_name.find(lower_query) != std::string::npos)
                {
                    m_filtered_apps.push_back(app);
                }
            }
        }
        m_icon_view->set_data(m_filtered_apps);
    }

    void LaunchpadWindow::draw(GraphicsContext &gc)
    {
        // 70% transparent dark background
        gc.setColor(Color(0.0f, 0.0f, 0.0f, 0.7f));
        gc.fillRect(m_x, m_y, m_width, m_height, CornerRadius(0));
    }

} // namespace horizon
