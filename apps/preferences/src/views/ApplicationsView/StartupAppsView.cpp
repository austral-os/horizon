#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TableColumn.hpp>
#include <views/ApplicationsView/DefaultAppsView.hpp>
#include <views/ApplicationsView/StartupAppsView.hpp>
#include <horizon/dialogs/AppPickerDialog.hpp>
#include <views/ApplicationsView/StartupEditDialog.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/I18n.hpp>

namespace horizon::preferences
{
    StartupAppsView::StartupAppsView() : Widget()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_position_type(WidgetPositionTypes::FILL);
        set_margin(15);
        set_spacing(10);

        setup_ui();
        load_data();
    }

    void StartupAppsView::setup_ui()
    {
        // TableView for applications
        auto table = std::make_unique<TableView<horizon::DesktopEntry>>();
        m_table = table.get();
        m_table->set_position_type(WidgetPositionTypes::FILL);
        m_table->set_header_visible(true);

        // Column: Icon
        TableColumn<horizon::DesktopEntry> col_icon;
        col_icon.title = "";
        col_icon.width = 48;
        col_icon.cell_factory = [](const horizon::DesktopEntry& entry) {
            auto container = std::make_unique<Widget>();
            container->set_margin(4);
            auto icon = std::make_unique<Icon>();
            icon->set_icon_name(entry.icon.empty() ? "application-x-executable" : entry.icon);
            icon->set_icon_size(24);
            icon->set_fixed_size(24);
            container->add_child(std::move(icon));
            return container;
        };
        m_table->add_column(col_icon);

        // Column: Name
        TableColumn<horizon::DesktopEntry> col_name;
        col_name.title = i18n().tr("preferences.applications.labels.name");
        col_name.width = 200;
        col_name.cell_factory = [](const horizon::DesktopEntry& entry) {
            auto label = std::make_unique<Label>(entry.name);
            label->set_font_size(14);
            label->set_margin(10);
            return label;
        };
        m_table->add_column(col_name);

        // Column: Command
        TableColumn<horizon::DesktopEntry> col_exec;
        col_exec.title = i18n().tr("preferences.applications.labels.command");
        col_exec.width = 300;
        col_exec.cell_factory = [](const horizon::DesktopEntry& entry) {
            auto label = std::make_unique<Label>(entry.exec);
            label->set_font_size(14);
            label->set_margin(10);
            return label;
        };
        m_table->add_column(col_exec);

        add_child(std::move(table));

        // Buttons
        auto buttons = std::make_unique<Widget>();
        buttons->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        buttons->set_fixed_size(35);
        buttons->set_spacing(10);

        auto btn_add = std::make_unique<Button<AquaObject>>();
        btn_add->set_text(i18n().tr("preferences.applications.buttons.add"));
        btn_add->set_fixed_size(100);
        btn_add->set_accent_color(WidgetAccentColor::Primary);
        btn_add->when_click.connect([this](MouseButtonEventContext&) { this->add_app(); });
        buttons->add_child(std::move(btn_add));

        auto btn_edit = std::make_unique<Button<AquaObject>>();
        btn_edit->set_text(i18n().tr("preferences.applications.buttons.edit"));
        btn_edit->set_fixed_size(100);
        btn_edit->when_click.connect([this](MouseButtonEventContext&) { this->edit_app(); });
        buttons->add_child(std::move(btn_edit));

        auto btn_remove = std::make_unique<Button<AquaObject>>();
        btn_remove->set_text(i18n().tr("preferences.applications.buttons.remove"));
        btn_remove->set_fixed_size(100);
        btn_remove->when_click.connect([this](MouseButtonEventContext&) { this->remove_app(); });
        buttons->add_child(std::move(btn_remove));

        add_child(std::move(buttons));
    }

    void StartupAppsView::load_data()
    {
        m_data = DesktopManager::load_autostart_entries();
        m_table->set_data(m_data);
    }

    void StartupAppsView::add_app()
    {
        auto dialog = std::make_unique<AppPickerDialog>();
        auto* ptr_dialog = dialog.get();
        ptr_dialog->when_accepted.connect([this](const horizon::DesktopEntry& entry) {
            DesktopManager::add_to_autostart(entry);
            load_data();
        });
        
        std::thread([d = std::move(dialog)]() mutable {
            d->initialize();
            d->run();
        }).detach();
    }

    void StartupAppsView::edit_app()
    {
        int idx = m_table->selected_index();
        if (idx < 0 || idx >= (int)m_data.size()) return;

        const auto& entry = m_data[idx];
        auto dialog = std::make_unique<StartupEditDialog>(entry);
        auto* ptr_dialog = dialog.get();
        ptr_dialog->when_accepted.connect([this, path = entry.path](const std::string& new_cmd) {
            DesktopManager::update_autostart_cmd(path, new_cmd);
            load_data();
        });
        
        std::thread([d = std::move(dialog)]() mutable {
            d->initialize();
            d->run();
        }).detach();
    }

    void StartupAppsView::remove_app()
    {
        int idx = m_table->selected_index();
        if (idx < 0 || idx >= (int)m_data.size()) return;

        const auto& entry = m_data[idx];
        DesktopManager::remove_from_autostart(entry.path);
        load_data();
    }
} // namespace horizon::preferences
