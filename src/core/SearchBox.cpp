#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/SearchBox.hpp>

namespace horizon
{
    SearchBox::SearchBox() : TextBox<TextPolicy>()
    {
        m_padding_left = 32;
        m_padding_right = 40; // More space for the clear icon
        m_corner_radius = 16;

        auto search_icon = std::make_unique<Icon>();
        search_icon->set_icon_name("system-search");
        search_icon->set_icon_size(16);
        search_icon->set_position_type(FREE);
        m_search_ptr = search_icon.get();
        add_child(std::move(search_icon));

        auto clear_icon = std::make_unique<Icon>();
        clear_icon->set_icon_name("edit-clear");
        clear_icon->set_icon_size(16);
        clear_icon->set_position_type(FREE);
        clear_icon->set_visible(false);
        m_clear_ptr = clear_icon.get();
        add_child(std::move(clear_icon));

        set_placeholder("Search...");

        when_text_changed.connect(
            [this](EventContext &ev)
            {
                if (m_clear_ptr)
                {
                    m_clear_ptr->set_visible(!m_text.empty());
                    invalidate();
                }
            });

        // Handle clear button click
        when_mouse_press.connect(
            [this](EventContext &ev)
            {
                if (m_text.empty() || !m_clear_ptr || !m_clear_ptr->is_visible())
                    return;

                int icon_x = m_clear_ptr->x();
                int icon_y = m_clear_ptr->y();

                // Hit test for clear icon (with a bit of extra padding for easier clicking)
                if (ev.eventX >= icon_x - 4 && ev.eventX <= icon_x + 20 &&
                    ev.eventY >= icon_y - 4 && ev.eventY <= icon_y + 20)
                {
                    set_text("");
                    m_cursor_pos = 0;
                    m_has_pending_click = false; // Prevent TextBoxBase from moving cursor

                    EventContext dummy;
                    dummy.sender = this;
                    when_text_changed.run(dummy);

                    ev.stop_propagation = true;
                    invalidate();
                }
            });
    }

    void SearchBox::calculate_layout()
    {
        TextBox<TextPolicy>::calculate_layout();

        if (m_search_ptr)
        {
            m_search_ptr->set_position(m_x + 8, m_y + (m_height - 16) / 2);
            m_search_ptr->set_size(16, 16);
        }

        if (m_clear_ptr)
        {
            // Margin of 16px from the right edge
            m_clear_ptr->set_position(m_x + m_width - 32, m_y + (m_height - 16) / 2);
            m_clear_ptr->set_size(16, 16);
        }
    }

    void SearchBox::draw(GraphicsContext &gc)
    {
        // Just call base. Children (Icons) will be drawn by Widget::render
        TextBoxBase::draw(gc);
    }
} // namespace horizon
