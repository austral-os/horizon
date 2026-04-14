#include "horizon/ColorSelector.hpp"
#include "horizon/Application.hpp"
#include <thread>

namespace horizon
{
    void ColorSelector::ColorBox::draw(GraphicsContext &gc)
    {
        gc.setColor(color);
        gc.fillRect(m_x, m_y, m_width, m_height, {4, 4, 4, 4});
        
        // Border
        gc.setColor(0.1f, 0.1f, 0.1f, 0.3f);
        gc.drawRect(m_x, m_y, m_width, m_height, 4, 1.0f);
    }

    void ColorSelector::ColorBox::calculate_layout()
    {
        if (m_parent)
        {
            m_x = m_parent->x() + 5;
            m_y = m_parent->y() + 5;
            m_width = m_parent->width() - 10;
            // The Button (SolidObject) renders with an effective height of (height - 3).
            // To have a symmetric 5px margin:
            // Top: y + 5
            // Bottom: y + (height - 3) - 5 = y + height - 8
            // Total height: (y + height - 8) - (y + 5) = height - 13
            m_height = m_parent->height() - 13;
        }
    }

    ColorSelector::ColorSelector()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);

        m_button = std::make_unique<Button<SolidObject>>();
        m_button->set_size(120, 32);
        
        auto box = std::make_unique<ColorBox>();
        m_preview_ptr = box.get();
        m_preview_ptr->set_position_type(FREE);
        
        m_button->add_child(std::move(box));

        m_button->when_click.connect([this](MouseButtonEventContext &) {
            std::thread([this]() {
                auto dialog = std::make_unique<ColorPickerDialog>();
                dialog->set_color(m_color);
                
                dialog->when_accepted.connect([this](ColorPickerDialogAcceptedContext &ctx) {
                    Color color = ctx.color;
                    if (this->application()) {
                        this->application()->post_task([this, color]() {
                            set_color(color);
                            ColorPickerDialogAcceptedContext new_ctx;
                            new_ctx.color = color;
                            when_color_changed.run(new_ctx);
                        });
                    }
                });

                dialog->show();
            }).detach();
        });

        add_child(std::move(m_button));

        m_color = Color(1.0f, 1.0f, 1.0f);
        set_color(m_color);
    }

    void ColorSelector::set_color(const Color &color)
    {
        m_color = color;
        m_preview_ptr->color = color;
        invalidate();
    }
} // namespace horizon
