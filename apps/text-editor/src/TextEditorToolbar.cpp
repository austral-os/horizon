#include "TextEditorToolbar.hpp"
#include <horizon/GroupButton.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Spacer.hpp>
#include <memory>

namespace horizon {
namespace text_editor {

TextEditorToolbar::TextEditorToolbar() : Widget() {
    set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
    set_position_type(WidgetPositionTypes::FILL);
    set_margin(5);
    set_spacing(10);

    // 1. File Actions group
    auto file_group = std::make_unique<horizon::GroupButton>();
    m_file_group = file_group.get();
    
    auto new_icon = std::make_unique<horizon::Icon>();
    new_icon->set_icon_name("document-new");
    new_icon->set_icon_size(16);
    m_file_group->add_item(std::move(new_icon));

    auto open_icon = std::make_unique<horizon::Icon>();
    open_icon->set_icon_name("document-open");
    open_icon->set_icon_size(16);
    m_file_group->add_item(std::move(open_icon));

    auto save_icon = std::make_unique<horizon::Icon>();
    save_icon->set_icon_name("document-save");
    save_icon->set_icon_size(16);
    m_file_group->add_item(std::move(save_icon));

    m_file_group->when_button_clicked.connect([this](horizon::GroupButtonClickEvent& ctx) {
        EventContext dummy;
        if (ctx.button_index == 0) when_new_clicked.run(dummy);
        else if (ctx.button_index == 1) when_open_clicked.run(dummy);
        else if (ctx.button_index == 2) when_save_clicked.run(dummy);
    });

    // 2. Edit Actions group
    auto edit_group = std::make_unique<horizon::GroupButton>();
    m_edit_group = edit_group.get();

    auto undo_icon = std::make_unique<horizon::Icon>();
    undo_icon->set_icon_name("edit-undo");
    undo_icon->set_icon_size(16);
    m_edit_group->add_item(std::move(undo_icon));

    auto redo_icon = std::make_unique<horizon::Icon>();
    redo_icon->set_icon_name("edit-redo");
    redo_icon->set_icon_size(16);
    m_edit_group->add_item(std::move(redo_icon));

    m_edit_group->when_button_clicked.connect([this](horizon::GroupButtonClickEvent& ctx) {
        EventContext dummy;
        if (ctx.button_index == 0) when_undo_clicked.run(dummy);
        else if (ctx.button_index == 1) when_redo_clicked.run(dummy);
    });

    add_child(std::move(file_group));
    add_child(std::move(edit_group));
    add_child(horizon::Spacer()); // Push everything to left
}

} // namespace text_editor
} // namespace horizon
