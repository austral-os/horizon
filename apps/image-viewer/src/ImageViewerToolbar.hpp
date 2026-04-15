#pragma once

#include <horizon/Widget.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/EventsManager.hpp>

namespace horizon {
namespace image {

class ImageViewerToolbar : public Widget {
public:
    ImageViewerToolbar();

    // Signal-like managers
    EventsManager<EventContext> when_open_clicked;
    EventsManager<GroupButtonClickEvent> when_navigation_clicked;
    EventsManager<GroupButtonClickEvent> when_zoom_clicked;
    EventsManager<GroupButtonClickEvent> when_transform_clicked;
    EventsManager<GroupButtonClickEvent> when_extra_clicked;

private:
    void setup_ui();
};

} // namespace image
} // namespace horizon
