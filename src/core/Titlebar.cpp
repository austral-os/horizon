#include "horizon/EventsManager.hpp"
#include "horizon/Widget.hpp"
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Titlebar.hpp>
#include <memory>
#include <unistd.h>

namespace horizon
{
    Titlebar::Titlebar(std::string title)
    {
        set_title(std::move(title));
    }

    void Titlebar::set_title(std::string title)
    {
        m_title = std::move(title);

        if (!m_children.empty())
            return;

        m_layout_type = WIDGET_LAYOUT_HORIZONTAL;
        m_position_type = FILL;

        auto spacer = std::make_unique<Widget>();
        spacer->set_position_type(FILL);
        spacer->set_fixed_size(4);

        auto close_button = std::make_unique<TitlebarCircleButton>(Color{1.0f, 0.0f, 0.0f, 0.6f});
        auto minimize_button =
            std::make_unique<TitlebarCircleButton>(Color{1.0f, 0.7f, 0.0f, 0.6f});
        auto maximize_button =
            std::make_unique<TitlebarCircleButton>(Color{0.6f, 0.9f, 0.29f, 0.6f});

        close_button->set_fixed_size(16);
        close_button->set_size(16, 16);

        minimize_button->set_fixed_size(16);
        minimize_button->set_size(16, 16);

        maximize_button->set_fixed_size(16);
        maximize_button->set_size(16, 16);

        m_close_button = close_button.get();
        m_minimize_button = minimize_button.get();
        m_maximize_button = maximize_button.get();

        if (m_close_button)
        {

            m_close_button->when_mouse_press.connect(
                [this](MouseButtonEventContext &context)
                {
                    if (application())
                    {
                        application()->send_remote_signal(getpid(), "close");
                    }
                });
        }

        if (m_minimize_button)
        {
            m_minimize_button->when_mouse_press.connect(
                [this](MouseButtonEventContext &context)
                {
                    if (application())
                    {
                        application()->minimize();
                    }
                });
        }

        if (m_maximize_button)
        {

            m_maximize_button->when_mouse_press.connect(
                [this](MouseButtonEventContext &context)
                {
                    if (application())
                    {
                        if (application()->is_maximized())
                        {
                            application()->restore();
                        }
                        else
                        {
                            application()->maximize();
                        }
                    }
                });
        }

        when_mouse_press.connect([this](MouseButtonEventContext &context)
                                 { m_dragging_requested = false; });

        when_mouse_drag.connect(
            [this](MouseMoveEventContext &context)
            {
                if (!m_dragging_requested && application())
                {
                    application()->request_move();
                    m_dragging_requested = true;
                }
            });

        when_mouse_release.connect([this](MouseButtonEventContext &context)
                                   { m_dragging_requested = false; });

        add_child(std::move(spacer));
        add_child(std::move(close_button));
        add_child(std::move(minimize_button));
        add_child(std::move(maximize_button));
    }

    const std::string &Titlebar::title() const
    {
        return m_title;
    }

    void Titlebar::render(GraphicsContext &gc, int cx, int cy, int cw, int ch, bool force)
    {
        Widget::render(gc, cx, cy, cw, ch, force);
    }

    void Titlebar::draw(GraphicsContext &gc)
    {
        // Dibujarmos una barra de titulo como la de mac os mountain lion.
        auto *tm = application()->theme_manager.get();
        auto font = tm->get_font("titlebar");

        set_background_colors(tm->get_color("titlebar_bg1"), tm->get_color("titlebar_bg2"));
        set_border_color(tm->get_color("titlebar_border"));
        set_corner_radius(CornerRadius(10, 10, 0, 0));

        // Let Panel handle the background and border
        Panel::draw(gc);

        Color title_fg = tm->get_color("titlebar_fg");

        TextMetrics metrics = gc.getTextMetrics(m_title.c_str(), font.family.c_str(), font.size,
                                                FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);

        int text_x = (m_width / 2) - (metrics.width / 2);
        int text_y = (m_height / 2) + (metrics.height / 2) - 1;

        gc.setDrawFont(font.family.c_str(), font.size, FONT_SLANT_NORMAL, FONT_WEIGHT_BOLD);
        gc.setColor(title_fg);
        gc.drawText(text_x, text_y, m_title.c_str());
    }

} // namespace horizon