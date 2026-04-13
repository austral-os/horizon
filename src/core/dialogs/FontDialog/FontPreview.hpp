#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <string>

namespace horizon {

class FontPreview : public Widget {
public:
    FontPreview();
    
    void set_font_family(const std::string& family);
    void set_font_style(const std::string& style);
    void set_font_size(float size);
    
    void update_preview();

private:
    Label* m_label;
    std::string m_family;
    std::string m_style;
    float m_size;
};

} // namespace horizon
