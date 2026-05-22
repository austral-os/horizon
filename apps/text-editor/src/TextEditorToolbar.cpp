#include "TextEditorToolbar.hpp"
#include <horizon/GroupButton.hpp>
#include <horizon/I18n.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Notification.hpp>
#include <horizon/Spacer.hpp>
#include <memory>

namespace horizon
{
    namespace text_editor
    {

        TextEditorToolbar::TextEditorToolbar() : Widget()
        {
            set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            set_position_type(WidgetPositionTypes::FILL);
            set_margin(5);
            set_spacing(10);

            auto create_icon = [](const std::string &name, const std::string &tr_key)
            {
                auto icon = std::make_unique<horizon::Icon>();
                icon->set_icon_name(name);
                icon->set_icon_size(16);
                icon->set_use_theme_colors(true);
                if (!tr_key.empty())
                {
                    auto tooltip = std::make_unique<horizon::Notification>();
                    tooltip->set_message(i18n().tr(tr_key));
                    icon->set_tooltip(std::move(tooltip));
                }
                return icon;
            };

            // 1. File Actions group
            auto file_group = std::make_unique<horizon::GroupButton>();
            m_file_group = file_group.get();
            m_file_group->set_fixed_size(150);

            m_file_group->add_item(create_icon("document-new", "text_editor.toolbar.new"));
            m_file_group->add_item(create_icon("document-open", "text_editor.toolbar.open"));
            m_file_group->add_item(create_icon("document-save", "text_editor.toolbar.save"));

            m_file_group->when_button_clicked.connect(
                [this](horizon::GroupButtonClickEvent &ctx)
                {
                    EventContext dummy;
                    if (ctx.button_index == 0)
                        when_new_clicked.run(dummy);
                    else if (ctx.button_index == 1)
                        when_open_clicked.run(dummy);
                    else if (ctx.button_index == 2)
                        when_save_clicked.run(dummy);
                });

            // 2. Edit Actions group
            auto edit_group = std::make_unique<horizon::GroupButton>();
            m_edit_group = edit_group.get();
            m_edit_group->set_fixed_size(100);

            m_edit_group->add_item(create_icon("edit-undo", "text_editor.toolbar.undo"));
            m_edit_group->add_item(create_icon("edit-redo", "text_editor.toolbar.redo"));

            m_edit_group->when_button_clicked.connect(
                [this](horizon::GroupButtonClickEvent &ctx)
                {
                    EventContext dummy;
                    if (ctx.button_index == 0)
                        when_undo_clicked.run(dummy);
                    else if (ctx.button_index == 1)
                        when_redo_clicked.run(dummy);
                });

            add_child(std::move(file_group));
            add_child(std::move(edit_group));
            add_child(horizon::Spacer());

            // 3. Settings Actions group (Right)
            auto settings_group = std::make_unique<horizon::GroupButton>();
            m_settings_group = settings_group.get();
            m_settings_group->set_fixed_size(100);

            m_settings_group->add_item(
                create_icon("view-fullscreen", "text_editor.toolbar.fullscreen"));
            m_settings_group->add_item(create_icon("configure", "text_editor.toolbar.preferences"));

            m_settings_group->when_button_clicked.connect(
                [this](horizon::GroupButtonClickEvent &ctx)
                {
                    EventContext dummy;
                    if (ctx.button_index == 0)
                        when_fullscreen_clicked.run(dummy);
                    else if (ctx.button_index == 1)
                        when_preferences_clicked.run(dummy);
                });

            add_child(std::move(settings_group));
        }

    } // namespace text_editor
} // namespace horizon
