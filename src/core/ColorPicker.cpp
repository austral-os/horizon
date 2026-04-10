#include <horizon/ColorPicker.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/ThemeManager.hpp>
#include <horizon/Widget.hpp>
#include <horizon/I18n.hpp>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <type_traits>

namespace horizon
{
    // --- ColorPreview ---
    void ColorPicker::ColorPreview::draw(GraphicsContext &gc)
    {
        int x = m_x, y = m_y, w = m_width, h = m_height;
        gc.setColor(current);
        gc.fillRect(x, y, w, h / 2, {4, 4, 0, 0});
        gc.setColor(previous);
        gc.fillRect(x, y + h / 2, w, h / 2, {0, 0, 4, 4});

        // Border
        gc.setColor(0.1f, 0.1f, 0.1f, 0.5f);
        gc.drawRect(x, y, w, h, 4, 1.0f);
    }

    // --- ColorPicker ---
    ColorPicker::ColorPicker()
    {
        m_color = Color(0.26f, 0.45f, 0.72f); // Default blue matching image
        m_previous_color = m_color;
        set_size(600, 400);
        setup_layout();
        update_ui_from_color();
    }

    ColorPicker::~ColorPicker() {}

    void ColorPicker::setup_layout()
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_spacing(10);
        set_margin(10);

        // --- Main Top Area (Horizontal Split) ---
        auto main_top = std::make_unique<Widget>();
        main_top->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        main_top->set_spacing(15);
        main_top->set_position_type(FILL);

        // Left Panel: SV Area + Hue Bar
        auto left_panel = std::make_unique<Widget>();
        left_panel->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        left_panel->set_spacing(5);
        left_panel->set_fixed_size(300);

        auto sv_area = std::make_unique<ColorArea2D>();
        m_area2d_sv = sv_area.get();
        m_area2d_sv->set_position_type(FILL);
        left_panel->add_child(std::move(sv_area));

        auto hue_bar = std::make_unique<GradientBar>();
        m_hue_bar = hue_bar.get();
        m_hue_bar->set_vertical(true);
        m_hue_bar->set_fixed_size(25);
        m_hue_bar->set_stops(
            {{1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {1, 0, 0}});
        left_panel->add_child(std::move(hue_bar));

        main_top->add_child(std::move(left_panel));

        // Right Panel: Sliders and Inputs
        auto right_panel = std::make_unique<Widget>();
        right_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        right_panel->set_spacing(4);
        right_panel->set_position_type(FILL);

        // Helper for creating slider rows
        auto create_slider_row =
            [&](const std::string &label_text, GradientBar **bar_out, auto **box_out)
        {
            using ActualTextBox =
                typename std::remove_pointer<typename std::decay<decltype(*box_out)>::type>::type;

            auto row = std::make_unique<Widget>();
            row->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
            row->set_spacing(5);
            row->set_fixed_size(26);

            auto label = std::make_unique<Label>();
            label->set_text(label_text);
            label->set_fixed_size(20);
            row->add_child(std::move(label));

            auto bar = std::make_unique<GradientBar>();
            *bar_out = bar.get();
            (*bar_out)->set_show_marker(true);
            (*bar_out)->set_position_type(FILL);
            row->add_child(std::move(bar));

            auto box = std::make_unique<ActualTextBox>();
            *box_out = box.get();
            (*box_out)->set_fixed_size(60);

            // Focus management for active input
            (*box_out)->when_focus.connect([this, box_ptr = (*box_out)](EventContext &)
                                           { m_active_input = box_ptr; });
            (*box_out)->when_blur.connect(
                [this, box_ptr = (*box_out)](EventContext &)
                {
                    if (m_active_input == box_ptr)
                        m_active_input = nullptr;
                });

            row->add_child(std::move(box));

            right_panel->add_child(std::move(row));
        };

        create_slider_row("R", &m_r_slider, &m_r_box);
        create_slider_row("G", &m_g_slider, &m_g_box);
        create_slider_row("B", &m_b_slider, &m_b_box);
        create_slider_row("A", &m_a_slider, &m_a_box);

        // Spacer for HSV
        auto spacer = std::make_unique<Widget>();
        spacer->set_fixed_size(10);
        right_panel->add_child(std::move(spacer));

        create_slider_row("H", &m_h_slider, &m_h_box);
        create_slider_row("S", &m_s_slider, &m_s_box);
        create_slider_row("V", &m_v_slider, &m_v_box);

        main_top->add_child(std::move(right_panel));
        add_child(std::move(main_top));

        // --- Bottom Area ---
        auto bottom_area = std::make_unique<Widget>();
        bottom_area->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        bottom_area->set_spacing(20);
        bottom_area->set_fixed_size(80);

        auto preview = std::make_unique<ColorPreview>();
        m_preview = preview.get();
        m_preview->set_fixed_size(120);
        bottom_area->add_child(std::move(preview));

        auto hex_panel = std::make_unique<Widget>();
        hex_panel->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        hex_panel->set_spacing(5);
        hex_panel->set_position_type(FILL);

        auto hex_label = std::make_unique<Label>();
        hex_label->set_text(i18n().tr("core.color_picker.html_notation"));
        hex_panel->add_child(std::move(hex_label));

        auto hex_box_container = std::make_unique<Widget>();
        hex_box_container->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        hex_box_container->set_fixed_size(30);

        auto hex_box = std::make_unique<TextBox<TextPolicy>>();
        m_hex_box = hex_box.get();
        m_hex_box->set_fixed_size(150);

        // Focus management for HEX input
        m_hex_box->when_focus.connect([this, hb_ptr = m_hex_box](EventContext &)
                                      { m_active_input = hb_ptr; });
        m_hex_box->when_blur.connect(
            [this, hb_ptr = m_hex_box](EventContext &)
            {
                if (m_active_input == hb_ptr)
                    m_active_input = nullptr;
            });

        hex_box_container->add_child(std::move(hex_box));

        hex_panel->add_child(std::move(hex_box_container));

        bottom_area->add_child(std::move(hex_panel));
        add_child(std::move(bottom_area));

        // --- Connect Signals ---
        m_hue_bar->when_value_changed.connect(
            [this](EventContext &)
            {
                float h, s, v;
                m_color.to_hsv(h, s, v);
                set_color(Color(m_hue_bar->value() * 360.0f, s, v, true));
            });

        m_area2d_sv->when_values_changed.connect(
            [this](EventContext &)
            {
                float h, s, v;
                m_color.to_hsv(h, s, v);
                set_color(Color(h, m_area2d_sv->value_x(), 1.0f - m_area2d_sv->value_y(), true));
            });

        auto connect_rgb = [&](GradientBar *b, int comp)
        {
            b->when_value_changed.connect(
                [this, b, comp](EventContext &)
                {
                    Color c = m_color;
                    if (comp == 0)
                        c.r = b->value();
                    else if (comp == 1)
                        c.g = b->value();
                    else if (comp == 2)
                        c.b = b->value();
                    set_color(c);
                });
        };
        connect_rgb(m_r_slider, 0);
        connect_rgb(m_g_slider, 1);
        connect_rgb(m_b_slider, 2);

        m_a_slider->when_value_changed.connect(
            [this](EventContext &)
            {
                Color c = m_color;
                c.a = m_a_slider->value();
                set_color(c);
            });

        // Connect HSV sliders
        m_h_slider->when_value_changed.connect(
            [this](EventContext &)
            {
                float h, s, v;
                m_color.to_hsv(h, s, v);
                set_color(Color(m_h_slider->value() * 360.0f, s, v, true));
            });

        m_s_slider->when_value_changed.connect(
            [this](EventContext &)
            {
                float h, s, v;
                m_color.to_hsv(h, s, v);
                set_color(Color(h, m_s_slider->value(), v, true));
            });

        m_v_slider->when_value_changed.connect(
            [this](EventContext &)
            {
                float h, s, v;
                m_color.to_hsv(h, s, v);
                set_color(Color(h, s, m_v_slider->value(), true));
            });

        // Connect RGB textboxes
        auto connect_rgb_box = [&](TextBoxBase *box, int comp)
        {
            box->when_text_changed.connect(
                [this, box, comp](EventContext &)
                {
                    try
                    {
                        float val = std::stof(box->text()) / 255.0f;
                        val = std::clamp(val, 0.0f, 1.0f);
                        Color c = m_color;
                        if (comp == 0)
                            c.r = val;
                        else if (comp == 1)
                            c.g = val;
                        else if (comp == 2)
                            c.b = val;
                        set_color(c);
                    }
                    catch (...)
                    {
                    } // ignore invalid input while typing
                });
        };
        connect_rgb_box(m_r_box, 0);
        connect_rgb_box(m_g_box, 1);
        connect_rgb_box(m_b_box, 2);

        m_a_box->when_text_changed.connect(
            [this](EventContext &)
            {
                try
                {
                    float val = std::stof(m_a_box->text()) / 255.0f;
                    val = std::clamp(val, 0.0f, 1.0f);
                    Color c = m_color;
                    c.a = val;
                    set_color(c);
                }
                catch (...)
                {
                }
            });

        // Connect HSV textboxes
        m_h_box->when_text_changed.connect(
            [this](EventContext &)
            {
                try
                {
                    float h = std::stof(m_h_box->text()); // degrees 0-360
                    float hs, s, v;
                    m_color.to_hsv(hs, s, v);
                    set_color(Color(std::clamp(h, 0.0f, 360.0f), s, v, true));
                }
                catch (...)
                {
                }
            });

        m_s_box->when_text_changed.connect(
            [this](EventContext &)
            {
                try
                {
                    float s = std::stof(m_s_box->text()) / 100.0f; // 0-100
                    float h, sv, v;
                    m_color.to_hsv(h, sv, v);
                    set_color(Color(h, std::clamp(s, 0.0f, 1.0f), v, true));
                }
                catch (...)
                {
                }
            });

        m_v_box->when_text_changed.connect(
            [this](EventContext &)
            {
                try
                {
                    float v = std::stof(m_v_box->text()) / 100.0f; // 0-100
                    float h, s, sv;
                    m_color.to_hsv(h, s, sv);
                    set_color(Color(h, s, std::clamp(v, 0.0f, 1.0f), true));
                }
                catch (...)
                {
                }
            });

        // Connect Hex textbox
        m_hex_box->when_text_changed.connect(
            [this](KeyEventContext &)
            {
                std::string text = m_hex_box->text();
                // Basic check to avoid immediate exceptions on empty/short strings
                if (text.length() >= 4)
                {
                    if (text[0] != '#')
                        text = "#" + text;

                    if (text.length() == 7 || text.length() == 9 || text.length() == 4)
                    {
                        try
                        {
                            set_color(Color(text));
                        }
                        catch (...)
                        {
                        }
                    }
                }
            });
    }

    void ColorPicker::set_color(const Color &color)
    {
        m_color = color;
        update_ui_from_color();

        EventContext ev;
        ev.sender = this;
        when_color_changed.run(ev);
    }

    void ColorPicker::update_ui_from_color()
    {
        float h, s, v;
        m_color.to_hsv(h, s, v);

        m_hue_bar->set_value(h / 360.0f);
        m_area2d_sv->set_hue(h / 360.0f);
        m_area2d_sv->set_values(s, 1.0f - v);

        // RGB
        m_r_slider->set_value(m_color.r);
        m_g_slider->set_value(m_color.g);
        m_b_slider->set_value(m_color.b);

        m_r_slider->set_stops({Color(0, m_color.g, m_color.b), Color(1, m_color.g, m_color.b)});
        m_g_slider->set_stops({Color(m_color.r, 0, m_color.b), Color(m_color.r, 1, m_color.b)});
        m_b_slider->set_stops({Color(m_color.r, m_color.g, 0), Color(m_color.r, m_color.g, 1)});
        m_a_slider->set_stops({Color(m_color.r, m_color.g, m_color.b, 0.0f),
                               Color(m_color.r, m_color.g, m_color.b, 1.0f)});

        m_a_slider->set_value(m_color.a);

        // HSV
        m_h_slider->set_value(h / 360.0f);
        m_s_slider->set_value(s);
        m_v_slider->set_value(v);

        m_preview->current = m_color;
        m_preview->previous = m_previous_color;

        if (m_active_input != m_hex_box)
            m_hex_box->set_text(m_color.to_hex().substr(0, 7)); // Just #RRGGBB

        auto to_string_dec = [](float f, int prec = 1)
        {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(prec) << f;
            return ss.str();
        };

        if (m_active_input != m_r_box)
            m_r_box->set_text(std::to_string((int)(m_color.r * 255)));
        if (m_active_input != m_g_box)
            m_g_box->set_text(std::to_string((int)(m_color.g * 255)));
        if (m_active_input != m_b_box)
            m_b_box->set_text(std::to_string((int)(m_color.b * 255)));
        if (m_active_input != m_a_box)
            m_a_box->set_text(std::to_string((int)(m_color.a * 255)));

        if (m_active_input != m_h_box)
            m_h_box->set_text(to_string_dec(h));
        if (m_active_input != m_s_box)
            m_s_box->set_text(to_string_dec(s * 100.0f));
        if (m_active_input != m_v_box)
            m_v_box->set_text(to_string_dec(v * 100.0f));

        invalidate();
    }
} // namespace horizon
