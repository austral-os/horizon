#pragma once

#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>

namespace horizon {
class GroupButton;

namespace text_editor {

class TextEditorToolbar : public Widget {
public:
    TextEditorToolbar();
    virtual ~TextEditorToolbar() = default;

    EventsManager<EventContext> when_new_clicked;
    EventsManager<EventContext> when_open_clicked;
    EventsManager<EventContext> when_save_clicked;
    EventsManager<EventContext> when_undo_clicked;
    EventsManager<EventContext> when_redo_clicked;

private:
    horizon::GroupButton* m_file_group = nullptr;
    horizon::GroupButton* m_edit_group = nullptr;
};

} // namespace text_editor
} // namespace horizon
