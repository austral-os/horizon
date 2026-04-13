#include "FontPreview.hpp"
#include <algorithm>

namespace horizon {

FontPreview::FontPreview() : Widget() {
    set_layout_type(WIDGET_LAYOUT_VERTICAL);
    set_background_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
    set_border_color(Color(0.7f, 0.7f, 0.7f, 1.0f));
    set_border_width(1);
    set_border_radius(4);
    set_margin(8);

    auto p_lbl = std::make_unique<Label>("00Q 1Il!| 5S 8B rnm :; ,. \"'` ~-= ({[<>]}) \n"
                                         "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~ \n"
                                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 \n"
                                         "abcdefghijklmnopqrstuvwxyz");
    m_label = p_lbl.get();
    m_label->set_vertical_alignment(VerticalAlignment::Top);
    add_child(std::move(p_lbl));
    
    m_family = "Inter";
    m_style = "Regular";
    m_size = 12.0f;
    update_preview();
}

void FontPreview::set_font_family(const std::string& family) {
    m_family = family;
    update_preview();
}

void FontPreview::set_font_style(const std::string& style) {
    m_style = style;
    update_preview();
}

void FontPreview::set_font_size(float size) {
    m_size = size;
    update_preview();
}

void FontPreview::update_preview() {
    if (m_label) {
        m_label->set_font_family(m_family);
        m_label->set_font_size(static_cast<int>(m_size));

        FontWeight weight = FONT_WEIGHT_NORMAL;
        FontSlant slant = FONT_SLANT_NORMAL;

        std::string style = m_style;
        std::transform(style.begin(), style.end(), style.begin(), ::tolower);

        if (style.find("bold") != std::string::npos)
            weight = FONT_WEIGHT_BOLD;

        if (style.find("italic") != std::string::npos)
            slant = FONT_SLANT_ITALIC;
        else if (style.find("oblique") != std::string::npos)
            slant = FONT_SLANT_OBLIQUE;

        m_label->set_font_weight(weight);
        m_label->set_font_slant(slant);

        m_label->invalidate();
    }
}

} // namespace horizon
