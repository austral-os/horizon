#pragma once

#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>

namespace horizon::capture {

struct SelectionRect {
    int x, y, width, height;
};

class SelectionWidget : public Widget {
public:
    SelectionWidget();
    ~SelectionWidget();

    void draw(GraphicsContext& ctx) override;

    EventsManager<SelectionRect>& when_selected() { return m_when_selected; }
    EventsManager<EventContext>& when_cancelled() { return m_when_cancelled; }

private:
    int m_start_x{-1};
    int m_start_y{-1};
    int m_current_x{-1};
    int m_current_y{-1};
    bool m_selecting{false};

    EventsManager<SelectionRect> m_when_selected;
    EventsManager<EventContext> m_when_cancelled;

    SelectionRect get_current_rect() const;
};

} // namespace horizon::capture
