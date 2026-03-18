#include "ArkfmIconView.hpp"
#include "ArkfmFileProvider.hpp"
#include "ArkfmWindow.hpp"
#include "dialogs/PropertiesDialog.hpp"
#include "horizon/Application.hpp"
#include "horizon/Icon.hpp"
#include "horizon/Label.hpp"
#include "horizon/Logger.hpp"
#include "horizon/Menu.hpp"
#include "horizon/ThemeManager.hpp"
#include "horizon/WaylandWindow.hpp"
#include <filesystem>

namespace horizon::arkfm
{
    class ArkfmIconItem : public Widget
    {
    public:
        ArkfmIconItem() : Widget()
        {
            auto icon = std::make_unique<Icon>();
            icon->set_position_type(FREE);
            icon->set_enabled(false); // Propagate events to parent
            m_icon_ptr = icon.get();
            add_child(std::move(icon));

            auto label = std::make_unique<Label>();
            label->set_position_type(FREE);
            label->set_alignment(TextAlignment::Center);
            label->set_vertical_alignment(VerticalAlignment::Middle);
            label->set_enabled(false); // Propagate events to parent
            m_label_ptr = label.get();
            add_child(std::move(label));

            m_position_type = FREE;
        }

        void set_data(const arkutils::FileInfo &f, float zoom, bool selected)
        {
            m_zoom = zoom;
            m_selected = selected;
            m_label_ptr->set_text(ArkfmFileProvider::get_display_name(f));

            std::string icon_name = ArkfmFileProvider::get_icon_name(f);

            m_icon_size = static_cast<int>(48 * m_zoom);
            m_icon_ptr->set_icon_name(icon_name);
            m_icon_ptr->set_icon_size(m_icon_size);

            m_label_ptr->set_font_size(10 * m_zoom);

            invalidate();
        }

        int preferred_height(int width) const override
        {
            int padding = static_cast<int>(4 * m_zoom);
            int gap = 4;
            int label_h = m_label_ptr->preferred_height(width - 4);
            return padding + m_icon_size + gap + label_h + padding;
        }

        void calculate_layout() override
        {
            int padding = static_cast<int>(4 * m_zoom);
            int icon_y = padding;

            m_icon_ptr->set_position(m_x + (m_width - m_icon_size) / 2, m_y + icon_y);
            m_icon_ptr->set_size(m_icon_size, m_icon_size);

            int label_y = icon_y + m_icon_size + 4;
            m_label_ptr->set_position(m_x + 2, m_y + label_y);
            m_label_ptr->set_size(m_width - 4, m_height - label_y - 2);
        }

        void draw(GraphicsContext &gc) override
        {
            if (m_selected)
            {
                auto *tm = application()->theme_manager.get();
                Color bg = tm->get_color("table_row_selected");
                Color fg = tm->get_color("table_row_selected_fg");

                int padding = static_cast<int>(4 * m_zoom);
                int label_y = padding + m_icon_size + 4;

                int h_x = m_x + 2;
                int h_y = m_y + label_y;
                int h_w = m_width - 4;
                int h_h = m_height - label_y - 2;

                gc.setColor(bg);
                gc.fillRect(h_x, h_y, h_w, h_h, CornerRadius(6));

                m_label_ptr->set_text_color(fg);
            }
            else
            {
                m_label_ptr->set_text_color(Color(0.0f, 0.0f, 0.0f, 1.0f));
            }
        }

    private:
        Icon *m_icon_ptr{nullptr};
        Label *m_label_ptr{nullptr};
        float m_zoom{1.5f};
        int m_icon_size{48};
        bool m_selected{false};
    };

    ArkfmIconView::ArkfmIconView(std::string path) : IconView<arkutils::FileInfo>()
    {
        set_position_type(FILL);
        set_zoom(1.5f);
        m_current_path = std::move(path);
        m_fs_model = std::make_unique<arkutils::FileSystemModel>();

        set_item_size(120, 130);
        set_side_margin(20);

        set_item_factory(
            [this](const arkutils::FileInfo &f, float zoom, bool selected)
            {
                auto item = std::make_unique<ArkfmIconItem>();
                item->set_data(f, zoom, selected);

                auto menu = std::make_unique<horizon::Menu>();
                menu->set_title(ArkfmFileProvider::get_display_name(f));

                
                ArkfmWindow* win = nullptr;
                if (auto *app = application())
                {
                    win = dynamic_cast<ArkfmWindow *>(app->root());
                }


                auto copy_item = menu->add_item("Copiar");
                copy_item->when_click.connect(
                    [win, f](horizon::MouseButtonEventContext &)
                    {
                        if (win) win->handle_copy(f.path);
                    });

                auto cut_item = menu->add_item("Cortar");
                cut_item->when_click.connect(
                    [win, f](horizon::MouseButtonEventContext &)
                    {
                        if (win) win->handle_cut(f.path);
                    });

                auto paste_item = menu->add_item("Pegar");
                if (win && win->has_clipboard_content())
                {
                    paste_item->when_click.connect(
                        [this, win, f](horizon::MouseButtonEventContext &)
                        {
                            if (win)
                            {
                                if (f.type == arkutils::FileType::Directory)
                                    win->handle_paste(f.path);
                                else
                                    win->handle_paste(m_current_path);
                            }
                        });
                }
                else
                {
                    paste_item->set_enabled(false);
                }

                auto rename_item = menu->add_item("Cambiar nombre");
                rename_item->when_click.connect(
                    [win, f](horizon::MouseButtonEventContext &)
                    {
                        if (win) win->handle_rename(f.path);
                    });

                menu->add_separator();

                auto delete_item = menu->add_item("Eliminar");
                delete_item->when_click.connect(
                    [win, f](horizon::MouseButtonEventContext &)
                    {
                        if (win) win->handle_delete(f.path);
                    });

                menu->add_separator();

                auto prop_item = menu->add_item("Propiedades");
                prop_item->when_click.connect(
                    [f](horizon::MouseButtonEventContext &)
                    {
                        auto dialog = std::make_unique<PropertiesDialog>(f);
                        dialog->run();
                    });

                item->set_context_menu(std::move(menu));
                item->when_right_click.connect([](horizon::MouseButtonEventContext &ctx) { ctx.stop_propagation = true; });

                return item;
            });

        m_fs_model->signal_manager().connect(arkutils::FileSystemModel::SIGNAL_DIRECTORY_CHANGED,
                                             [this](SignalContext &ctx)
                                             {
                                                 std::string *path = (std::string *)ctx.data;
                                                 if (path && *path == m_current_path)
                                                 {
                                                     this->refresh(*path);
                                                 }
                                             });

        when_right_click.connect(
            [this](horizon::MouseButtonEventContext &ctx)
            {
                auto bg_menu = std::make_unique<horizon::Menu>();
                bg_menu->set_title("Carpeta");
                
                ArkfmWindow* win = nullptr;
                if (auto *app = application())
                {
                    win = dynamic_cast<ArkfmWindow *>(app->root());
                }

                auto copy_item = bg_menu->add_item("Copiar");
                copy_item->set_enabled(false);

                auto cut_item = bg_menu->add_item("Cortar");
                cut_item->set_enabled(false);

                auto paste_item = bg_menu->add_item("Pegar");
                if (win && win->has_clipboard_content())
                {
                    paste_item->when_click.connect(
                        [this, win](horizon::MouseButtonEventContext &)
                        {
                            win->handle_paste(m_current_path);
                        });
                }
                else
                {
                    paste_item->set_enabled(false);
                }

                auto rename_item = bg_menu->add_item("Cambiar nombre");
                rename_item->set_enabled(false);

                bg_menu->add_separator();

                auto delete_item = bg_menu->add_item("Eliminar");
                delete_item->set_enabled(false);

                bg_menu->add_separator();

                auto bg_prop_item = bg_menu->add_item("Propiedades");
                bg_prop_item->when_click.connect(
                    [this](horizon::MouseButtonEventContext &)
                    {
                        arkutils::FileInfo f;
                        f.name = std::filesystem::path(m_current_path).filename().string();
                        if (f.name.empty()) f.name = "/";
                        f.path = m_current_path;
                        f.type = arkutils::FileType::Directory;
                        auto dialog = std::make_unique<PropertiesDialog>(f);
                        dialog->run();
                    });

                set_context_menu(std::move(bg_menu));
                ctx.stop_propagation = true;
            });

        when_application_load.connect(
            [this](EventContext &)
            {
                this->refresh(m_current_path);
            });
    }

    void ArkfmIconView::refresh(const std::string &path)
    {
        LOG_INFO << "Refreshing icon view for path: " << path;
        try
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
            update_icons(visible_files);
        }
        catch (std::exception &e)
        {
            LOG_ERROR << "Failed to refresh icon view: " << e.what();
        }
    }

    void ArkfmIconView::update_icons(const std::vector<arkutils::FileInfo> &files)
    {
        set_data(files);
    }
} // namespace horizon::arkfm
