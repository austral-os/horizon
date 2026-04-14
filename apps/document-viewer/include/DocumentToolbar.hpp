#pragma once

#include <horizon/Widget.hpp>
#include <horizon/GroupButton.hpp>
#include <memory>

namespace horizon {
namespace pdf {

class DocumentToolbar : public horizon::Widget {
public:
    DocumentToolbar();
    ~DocumentToolbar() = default;

    // Signals for actions
    horizon::EventsManager<horizon::EventContext> when_open_clicked;
    horizon::EventsManager<horizon::GroupButtonClickEvent> when_zoom_clicked;
    horizon::EventsManager<horizon::GroupButtonClickEvent> when_view_clicked;

private:
    void setup_ui();
};

} // namespace pdf
} // namespace horizon
