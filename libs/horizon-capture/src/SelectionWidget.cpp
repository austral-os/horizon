#include <horizon/capture/SelectionWidget.h>
#include <horizon/GraphicsContext.hpp>
#include <algorithm>

namespace horizon::capture {

SelectionWidget::SelectionWidget() {
    set_focusable(true);

    when_mouse_press.connect([this](MouseButtonEventContext& ev) {
        set_focus(true);
        m_start_x = ev.x;
        m_start_y = ev.y;
        m_current_x = ev.x;
        m_current_y = ev.y;
        m_selecting = true;
        invalidate();
    });

    auto handle_move = [this](MouseMoveEventContext& ev) {
        if (m_selecting) {
            m_current_x = ev.x;
            m_current_y = ev.y;
            invalidate();
        }
    };

    when_mouse_move.connect(handle_move);
    when_mouse_drag.connect(handle_move);

    when_mouse_release.connect([this](MouseButtonEventContext& ev) {
        if (m_selecting) {
            m_selecting = false;
            auto rect = get_current_rect();
            
            // Clear selection state immediately
            m_start_x = -1;
            m_start_y = -1;
            invalidate();
            
            if (rect.width > 5 && rect.height > 5) {
                m_when_selected.run(rect);
            }
        }
    });

    when_key_press.connect([this](KeyEventContext& ev) {
        if (ev.keysym == 0xFF1B) { // Escape
            EventContext ctx;
            m_when_cancelled.run(ctx);
        }
    });
}

SelectionWidget::~SelectionWidget() = default;

SelectionRect SelectionWidget::get_current_rect() const {
    int x1 = std::clamp(std::min(m_start_x, m_current_x), 0, width());
    int y1 = std::clamp(std::min(m_start_y, m_current_y), 0, height());
    int x2 = std::clamp(std::max(m_start_x, m_current_x), 0, width());
    int y2 = std::clamp(std::max(m_start_y, m_current_y), 0, height());
    
    if (m_start_x == -1) return {0,0,0,0};
    
    return {x1, y1, x2 - x1, y2 - y1};
}

void SelectionWidget::draw(GraphicsContext& ctx) {
    int w = width();
    int h = height();

    if (m_start_x == -1) {
        return;
    }

    auto rect = get_current_rect();
    
    ctx.setColor(0, 0, 0, 0.4f);
    
    if (rect.y > 0)
        ctx.fillRect(0, 0, w, rect.y);
    
    if (rect.y + rect.height < h)
        ctx.fillRect(0, rect.y + rect.height, w, h - (rect.y + rect.height));
    
    if (rect.x > 0)
        ctx.fillRect(0, rect.y, rect.x, rect.height);
    
    if (rect.x + rect.width < w)
        ctx.fillRect(rect.x + rect.width, rect.y, w - (rect.x + rect.width), rect.height);

    ctx.setColor(1.0f, 1.0f, 1.0f, 0.8f);
    ctx.drawRect(rect.x, rect.y, rect.width, rect.height, 0, 1.5f);
    
    ctx.setColor(1.0f, 1.0f, 1.0f, 1.0f);
    ctx.fillRect(rect.x - 3, rect.y - 3, 6, 6);
    ctx.fillRect(rect.x + rect.width - 3, rect.y - 3, 6, 6);
    ctx.fillRect(rect.x - 3, rect.y + rect.height - 3, 6, 6);
    ctx.fillRect(rect.x + rect.width - 3, rect.y + rect.height - 3, 6, 6);
}

} // namespace horizon::capture
