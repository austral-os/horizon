#include "FileDialog.hpp"
#include <horizon/files/FileView.hpp>
#include <horizon/files/FileSidebar.hpp>
#include <horizon/files/FileToolbar.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Label.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Button.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/Logger.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/AquaObject.hpp>
#include <filesystem>

namespace horizon
{
    FileDialog::FileDialog(FileDialogMode mode, const std::string &title)
        : WaylandWindow("horizon.dialog.file", 900, 600, false, false), m_mode(mode)
    {
        set_name(title.empty() ? "Select File" : title);
        setup_ui();
    }

    FileDialog::~FileDialog() = default;

    void FileDialog::setup_ui()
    {
        auto window_widget = std::make_unique<ApplicationWindow>(name());
        auto *app_window = window_widget.get();

        // 1. Toolbar setup
        auto toolbar_widget = std::make_unique<files::FileToolbar>();
        m_toolbar = toolbar_widget.get();
        app_window->toolbar()->add_toolbar_widget(std::move(toolbar_widget));

        // 2. Main Content (Sidebar + View)
        auto main_panel = std::make_unique<VPanel>();
        main_panel->set_spacing(0);

        auto sidebar = std::make_unique<files::FileSidebar>();
        m_sidebar = sidebar.get();
        m_sidebar->set_width(200);

        auto view = std::make_unique<files::FileView>(getenv("HOME") ? getenv("HOME") : "/");
        m_view = view.get();

        // Connect Sidebar to View
        m_sidebar->when_item_selected.connect([this](SidebarItemSelectedContext &ctx) {
            if (!ctx.item->path().empty()) {
                m_view->navigate_to(ctx.item->path());
            }
        });

        // Connect Toolbar to View
        m_toolbar->when_navigation_clicked.connect([this](files::NavigationButtonClickEvent &ctx) {
            if (ctx.index == 0) m_view->navigate_back();
            else m_view->navigate_forward();
        });

        m_toolbar->when_view_mode_changed.connect([this](files::ViewModeChangeEvent &ctx) {
            if (ctx.view_mode_index == 0) m_view->set_view_mode(files::ViewMode::Grid);
            else if (ctx.view_mode_index == 1) m_view->set_view_mode(files::ViewMode::List);
            else if (ctx.view_mode_index == 3) m_view->set_view_mode(files::ViewMode::CoverFlow);
        });

        m_toolbar->when_search_changed.connect([this](files::SearchChangedEvent &ctx) {
            m_view->set_search_query(ctx.query);
        });

        main_panel->add_child(std::move(sidebar));
        main_panel->add_child(std::move(view));

        // 3. Footer Setup
        auto footer_wrapper = std::make_unique<Widget>();
        footer_wrapper->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        footer_wrapper->set_height(100);
        footer_wrapper->set_margin(10);

        // Row 1: Filename
        auto row1 = std::make_unique<Widget>();
        row1->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        row1->set_height(40);
        
        auto lbl_name = std::make_unique<Label>(m_mode == FileDialogMode::Save ? "Save as:" : "File name:");
        lbl_name->set_width(100);
        
        auto input_name = std::make_unique<TextBox<>>();
        m_filename_input = input_name.get();
        m_filename_input->set_position_type(FILL);

        row1->add_child(std::move(lbl_name));
        row1->add_child(std::move(input_name));

        // Row 2: Filter and Buttons
        auto row2 = std::make_unique<Widget>();
        row2->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        row2->set_height(40);

        auto lbl_filter = std::make_unique<Label>("Filter:");
        lbl_filter->set_width(100);

        auto combo_filter = std::make_unique<Combo>();
        m_filter_combo = combo_filter.get();
        m_filter_combo->set_width(200);
        m_filter_combo->add_item("all", "All Files (*.*)");

        auto btn_cancel = std::make_unique<Button<AquaObject>>();
        btn_cancel->set_text("Cancel");
        btn_cancel->set_width(100);
        btn_cancel->when_click.connect([this](MouseButtonEventContext &) {
            if (on_cancelled) on_cancelled();
            on_close();
        });

        auto btn_accept = std::make_unique<Button<AquaObject>>();
        btn_accept->set_text(m_mode == FileDialogMode::Save ? "Save" : "Open");
        btn_accept->set_width(100);
        btn_accept->set_accent_color(WidgetAccentColor::Primary);
        btn_accept->when_click.connect([this](MouseButtonEventContext &) {
            handle_accept();
        });

        row2->add_child(std::move(lbl_filter));
        row2->add_child(std::move(combo_filter));
        row2->add_child(Spacer());
        row2->add_child(std::move(btn_cancel));
        row2->add_child(std::move(btn_accept));

        footer_wrapper->add_child(std::move(row1));
        footer_wrapper->add_child(std::move(row2));

        // Final assembly
        auto root_container = std::make_unique<Widget>();
        root_container->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        root_container->add_child(std::move(main_panel));
        root_container->add_child(std::move(footer_wrapper));

        app_window->set_content(std::move(root_container));
        set_root(std::move(window_widget));

        // Connect view selection to filename input
        m_view->when_item_opened.connect([this](const arkutils::FileInfo &f) {
            if (f.type == arkutils::FileType::Regular) {
                m_filename_input->set_text(f.name);
                if (m_mode == FileDialogMode::Open) handle_accept();
            }
        });
    }

    void FileDialog::handle_accept()
    {
        std::string filename = m_filename_input->text();
        if (filename.empty()) {
            auto selection = m_view->get_selection();
            if (!selection.empty()) filename = selection[0].name;
            else return;
        }

        std::filesystem::path p = std::filesystem::path(m_view->current_path()) / filename;
        if (on_accepted) on_accepted(p.string());
        on_close();
    }

    void FileDialog::set_current_path(const std::string &path)
    {
        if (m_view) m_view->navigate_to(path);
    }

    std::string FileDialog::selected_path() const
    {
        return std::filesystem::path(m_view->current_path()) / m_filename_input->text();
    }

} // namespace horizon
