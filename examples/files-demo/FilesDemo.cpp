#include "FilesDemo.hpp"
#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Slider.hpp>
#include <horizon/SolidObject.hpp>
#include <horizon/VPanel.hpp>
#include <iostream>

namespace horizon::demo
{
    FilesDemo::FilesDemo()
    {
        m_fs_model = std::make_unique<arkutils::FileSystemModel>();
        m_current_path = "/home/horacio"; // Default path

        m_app = std::make_unique<Application>(800, 600);
        m_app->set_app_id("org.horizon.files_demo");
        m_app->set_name("Files Demo");

        auto window = std::make_unique<ApplicationWindow>("FileSystem Model Demo");
        window->set_size(800, 600);
        m_window = window.get();

        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto top_bar = std::make_unique<Widget>();
        top_bar->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        top_bar->set_fixed_size(40);

        auto toggle_btn = std::make_unique<Button<AquaObject>>();
        toggle_btn->set_text("Toggle View");
        toggle_btn->set_size(100, 30);
        toggle_btn->when_mouse_press.connect(
            [this](MouseButtonEventContext &ev)
            {
                if (ev.button == 0x110)
                {
                    m_is_coverflow_view = !m_is_coverflow_view;
                    m_table->set_visible(!m_is_coverflow_view);
                    if (m_coverflow_container)
                        m_coverflow_container->set_visible(m_is_coverflow_view);

                    if (m_view_container)
                    {
                        m_view_container->set_background_color(m_is_coverflow_view
                                                                   ? Color(0.0f, 0.0f, 0.0f)
                                                                   : Color(0.0f, 0.0f, 0.0f, 0.0f));
                        m_view_container->invalidate();
                    }
                }
            });

        top_bar->add_child(std::move(toggle_btn));
        root->add_child(std::move(top_bar));

        auto view_container = std::make_unique<Widget>();
        m_view_container = view_container.get();

        auto table = std::make_unique<TableView<arkutils::FileInfo>>();
        m_table = table.get();
        m_table->set_width_mode(horizon::TableViewWidthMode::Fill);

        TableColumn<arkutils::FileInfo> col_icon;
        col_icon.id = "icon";
        col_icon.title = "";
        col_icon.width = 40;
        col_icon.cell_factory = [](const arkutils::FileInfo &f)
        {
            auto icon = std::make_unique<Icon>();
            icon->set_icon_size(24);

            std::string icon_name = "text-x-generic";
            if (f.type == arkutils::FileType::Directory)
            {
                icon_name = "folder";
            }
            else if (f.extension == "png" || f.extension == "jpg" || f.extension == "jpeg" ||
                     f.extension == "svg")
            {
                icon_name = "image-x-generic";
            }
            else if (f.extension == "cpp" || f.extension == "hpp" || f.extension == "c" ||
                     f.extension == "h")
            {
                icon_name = "text-x-csrc";
            }
            else if (f.extension == "txt" || f.extension == "md")
            {
                icon_name = "text-x-generic";
            }
            else if (f.extension == "pdf")
            {
                icon_name = "document-pdf";
            }
            else if (f.extension == "zip" || f.extension == "tar" || f.extension == "gz")
            {
                icon_name = "package-x-generic";
            }

            icon->set_icon_name(icon_name);
            return icon;
        };

        TableColumn<arkutils::FileInfo> col_name;
        col_name.id = "name";
        col_name.title = "Name";
        col_name.width = 300;
        col_name.cell_factory = [](const arkutils::FileInfo &f)
        {
            auto lbl = std::make_unique<Label>(f.name);
            if (f.type == arkutils::FileType::Directory)
            {
                lbl->set_font_weight(FONT_WEIGHT_BOLD);
            }
            return lbl;
        };

        TableColumn<arkutils::FileInfo> col_type;
        col_type.id = "type";
        col_type.title = "Type";
        col_type.width = 100;
        col_type.cell_factory = [](const arkutils::FileInfo &f)
        {
            return std::make_unique<Label>(f.type == arkutils::FileType::Directory ? "Folder"
                                                                                   : "File");
        };

        TableColumn<arkutils::FileInfo> col_size;
        col_size.id = "size";
        col_size.title = "Size";
        col_size.width = 100;
        col_size.cell_factory = [](const arkutils::FileInfo &f)
        { return std::make_unique<Label>(std::to_string(f.size / 1024) + " KB"); };

        TableColumn<arkutils::FileInfo> col_mod;
        col_mod.id = "modified";
        col_mod.title = "Modified";
        col_mod.width = 200;
        col_mod.cell_factory = [](const arkutils::FileInfo &f)
        {
            auto t = std::chrono::system_clock::to_time_t(f.last_modified);
            char time_str[26] = {0};
            ctime_r(&t, time_str);
            if (time_str[0] != '\0')
            {
                time_str[24] = '\0'; // Remove newline
            }
            return std::make_unique<Label>(std::string(time_str));
        };

        m_table->add_column(col_icon);
        m_table->add_column(col_name);
        m_table->add_column(col_type);
        m_table->add_column(col_size);
        m_table->add_column(col_mod);

        // Handle row selection
        m_table->when_row_dbl_click.connect(
            [this](horizon::TableViewRowMouseClickContext<arkutils::FileInfo> &ctx)
            {
                if (ctx.row_data.type == arkutils::FileType::Directory)
                {
                    m_current_path = ctx.row_data.path;
                    m_fs_model->unwatch_all(); // Important: we should stop watching old folders if
                                               // we only care about the new one, but for now we
                                               // just change path.
                    this->refresh_ui(m_current_path);

                    // Update window title or add a label if desired
                    std::cout << "[Demo] Navigating to: " << m_current_path << std::endl;
                }
            });

        auto coverflow_container = std::make_unique<Widget>();
        m_coverflow_container = coverflow_container.get();
        coverflow_container->set_background_color(Color(0.0f, 0.0f, 0.0f)); // Black background
        coverflow_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        coverflow_container->set_visible(false);
        m_is_coverflow_view = false;
        m_table->set_visible(true);

        auto coverflow = std::make_unique<CoverFlow<arkutils::FileInfo>>();
        m_coverflow = coverflow.get();
        m_coverflow->set_item_factory(
            [](const arkutils::FileInfo &f, bool selected)
            {
                auto item = std::make_unique<Widget>();
                item->set_layout_type(WIDGET_LAYOUT_VERTICAL);

                auto icon_name_str =
                    f.type == arkutils::FileType::Directory ? "folder" : "text-x-generic";
                if (f.extension == "png" || f.extension == "jpg" || f.extension == "jpeg" ||
                    f.extension == "svg")
                    icon_name_str = "image-x-generic";

                auto icon = std::make_unique<Icon>();
                icon->set_icon_name(icon_name_str);
                icon->set_icon_size(180);

                // Note: The CoverFlow image typically just shows the art, we don't put labels on
                // the items themselves.
                item->add_child(std::move(icon));

                return item;
            });

        auto cf_label = std::make_unique<Label>("No selection");
        cf_label->set_text_color(Color(1.0f, 1.0f, 1.0f));
        cf_label->set_background_color(Color(0.0f, 0.0f, 0.0f)); // Black background
        cf_label->set_font_weight(FONT_WEIGHT_BOLD);
        cf_label->set_alignment(TextAlignment::Center);
        cf_label->set_fixed_size(30);
        auto cf_label_ptr = cf_label.get();

        auto cf_slider = std::make_unique<Slider>();
        cf_slider->set_size(400, 30);
        cf_slider->set_show_ticks(false);
        auto cf_slider_ptr = cf_slider.get();

        auto cf_slider_container = std::make_unique<Widget>();
        cf_slider_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        cf_slider_container->set_background_color(Color(0.0f, 0.0f, 0.0f)); // Black background
        cf_slider_container->set_fixed_size(40);

        auto cf_spacer_l = std::make_unique<Widget>();
        auto cf_spacer_r = std::make_unique<Widget>();

        cf_slider_container->add_child(std::move(cf_spacer_l));
        cf_slider_container->add_child(std::move(cf_slider));
        cf_slider_container->add_child(std::move(cf_spacer_r));

        m_coverflow->when_selection_changed.connect(
            [this, cf_label_ptr, cf_slider_ptr](EventContext &)
            {
                int idx = m_coverflow->selected_index();
                if (idx >= 0 && idx < (int)m_coverflow->data().size())
                {
                    const auto &f = m_coverflow->data()[idx];
                    cf_label_ptr->set_text(f.name);

                    if (m_coverflow->data().size() > 1)
                    {
                        float frac = (float)idx / (m_coverflow->data().size() - 1);
                        cf_slider_ptr->set_value(frac);
                    }
                }
            });

        cf_slider_ptr->when_value_changed.connect(
            [this](EventContext &ev)
            {
                auto *slider = static_cast<Slider *>(ev.sender);
                float frac = slider ? slider->value() : 0.0f;
                int count = (int)m_coverflow->data().size();
                if (count > 0)
                {
                    int target_idx = std::round(frac * (count - 1));
                    target_idx = std::max(0, std::min(count - 1, target_idx));
                    if (m_coverflow->selected_index() != target_idx)
                    {
                        m_coverflow->set_selected_index(target_idx);
                    }
                }
            });

        coverflow_container->add_child(std::move(coverflow));
        coverflow_container->add_child(std::move(cf_label));
        coverflow_container->add_child(std::move(cf_slider_container));

        m_view_container->add_child(std::move(table));
        m_view_container->add_child(std::move(coverflow_container));
        root->add_child(std::move(view_container));

        // Listen for changes
        m_fs_model->signal_manager().connect(arkutils::FileSystemModel::SIGNAL_DIRECTORY_CHANGED,
                                             [this](SignalContext &ctx)
                                             {
                                                 std::string *path = (std::string *)ctx.data;
                                                 if (path && *path == m_current_path)
                                                 {
                                                     this->refresh_ui(*path);
                                                 }
                                             });

        refresh_ui(m_current_path);

        m_window->add_child(std::move(root));
        m_app->set_root(std::move(window));
    }

    int FilesDemo::run()
    {
        m_app->run();
        return 0;
    }

    void FilesDemo::refresh_ui(const std::string &path)
    {
        auto files = m_fs_model->list_directory(path);
        std::vector<arkutils::FileInfo> visible_files;
        for (const auto &f : files)
        {
            if (!f.is_hidden)
            {
                visible_files.push_back(f);
            }
        }
        update_table(visible_files);
    }

    void FilesDemo::update_table(const std::vector<arkutils::FileInfo> &files)
    {
        std::cout << "[Demo] Updating table with " << files.size() << " files." << std::endl;
        // TableView stores data by value, copy it.
        std::vector<arkutils::FileInfo> files_copy = files;
        m_table->set_data(std::move(files_copy));
        m_coverflow->set_data(std::move(files));
    }
} // namespace horizon::demo

int main(int argc, char *argv[])
{
    horizon::demo::FilesDemo app;
    return app.run();
}
