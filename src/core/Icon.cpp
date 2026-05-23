#include "horizon/Icon.hpp"
#include "horizon/IconThemeLookup.hpp"
#include <horizon/Logger.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/ThemeManager.hpp>

namespace horizon
{

    Icon::Icon() : Widget()
    {
        // Default to FILL so we can center content in available space (e.g. inside Buttons)
        m_fixed_size = -1;
    }

    Icon::~Icon() = default;

    void Icon::set_icon_path(const std::string &path)
    {
        if (m_resolved_path == path)
            return;

        m_icon_name = "";
        m_resolved_path = path;
        invalidate();
    }

    void Icon::set_icon_name(const std::string &name)
    {
        if (m_icon_name == name)
            return;

        m_icon_name = name;
        resolve_icon();
        invalidate();
    }

    const std::string &Icon::icon_name() const
    {
        return m_icon_name;
    }

    void Icon::set_icon_size(int size)
    {
        if (m_icon_size == size)
            return;

        m_icon_size = size;
        // Do NOT call set_fixed_size here, so we can fill the parent's available space
        // and center the icon internally.
        resolve_icon();
        invalidate();
    }

    int Icon::icon_size() const
    {
        return m_icon_size;
    }

    const std::string &Icon::resolved_path() const
    {
        return m_resolved_path;
    }

    void Icon::set_vertical_alignment(VerticalAlignment alignment)
    {
        if (m_vertical_alignment == alignment)
            return;

        m_vertical_alignment = alignment;
        invalidate();
    }

    VerticalAlignment Icon::vertical_alignment() const
    {
        return m_vertical_alignment;
    }

    void Icon::set_horizontal_alignment(TextAlignment alignment)
    {
        if (m_horizontal_alignment == alignment)
            return;

        m_horizontal_alignment = alignment;
        invalidate();
    }

    TextAlignment Icon::horizontal_alignment() const
    {
        return m_horizontal_alignment;
    }

    void Icon::set_fixed_size(int size)
    {
        // Ignore 0 as it's often a sign of uninitialized layout in parent
        if (size == 0)
            return;

        Widget::set_fixed_size(size);
    }

    int Icon::preferred_width() const
    {
        return m_fixed_size > 0 ? m_fixed_size : m_icon_size;
    }

    int Icon::preferred_height() const
    {
        return m_fixed_size > 0 ? m_fixed_size : m_icon_size;
    }

    int Icon::preferred_height(int /*width*/) const
    {
        return preferred_height();
    }

    void Icon::resolve_icon()
    {
        if (m_icon_name.empty())
        {
            return;
        }

        m_resolved_path = IconThemeLookup::find_icon(m_icon_name, m_icon_size);

        if (m_resolved_path.empty())
        {
            // Fallback for arrows
            if (m_icon_name.find("pan-") == 0 || m_icon_name.find("go-") == 0)
            {
                std::string alt = (m_icon_name.find("down") != std::string::npos) ? "arrow-down" : "arrow-right";
                m_resolved_path = IconThemeLookup::find_icon(alt, m_icon_size);
            }
        }

        if (m_resolved_path.empty() && m_icon_name != "application-x-executable")
        {
            m_resolved_path = IconThemeLookup::find_icon("application-x-executable", m_icon_size);
        }

        if (m_resolved_path.empty())
        {
            m_resolved_path = IconThemeLookup::find_icon("system-run", m_icon_size);
        }
    }
    
    void Icon::set_opacity(float opacity)
    {
        m_opacity = opacity;
        invalidate();
    }
    
    float Icon::opacity() const
    {
        return m_opacity;
    }

    void Icon::set_icon_color(Color color)
    {
        m_icon_color = color;
        invalidate();
    }

    Color Icon::icon_color() const
    {
        return m_icon_color;
    }

    void Icon::set_use_theme_colors(bool use)
    {
        if (m_use_theme_colors == use)
            return;

        m_use_theme_colors = use;
        invalidate();
    }

    bool Icon::use_theme_colors() const
    {
        return m_use_theme_colors;
    }

    void Icon::set_theme_color_key(const std::string &key)
    {
        if (m_theme_color_key == key)
            return;

        m_theme_color_key = key;
        if (m_use_theme_colors)
            invalidate();
    }

    const std::string &Icon::theme_color_key() const
    {
        return m_theme_color_key;
    }

    void Icon::draw(GraphicsContext &ctx)
    {
        if (m_resolved_path.empty())
            return;

        int draw_size = m_icon_size;
        int icon_x = m_start_draw_x;
        int icon_y = m_start_draw_y;


        if (m_horizontal_alignment == TextAlignment::Center)
        {
            icon_x += (m_available_draw_width - draw_size) / 2;
        }
        else if (m_horizontal_alignment == TextAlignment::Right)
        {
            icon_x += (m_available_draw_width - draw_size);
        }

        if (m_vertical_alignment == VerticalAlignment::Middle)
        {
            icon_y += (m_available_draw_height - draw_size) / 2;
        }
        else if (m_vertical_alignment == VerticalAlignment::Bottom)
        {
            icon_y += (m_available_draw_height - draw_size);
        }

        float alpha = m_opacity * (is_enabled() ? 1.0f : 0.4f);
        Color paint_color = m_icon_color;
        bool has_color = m_icon_color.a > 0.001f;

        if (m_use_theme_colors && theme_manager())
        {
            paint_color = theme_manager()->get_color(m_theme_color_key);
            has_color = true;
        }

        if (has_color)
        {
            ctx.drawImage(m_resolved_path, icon_x, icon_y, draw_size, draw_size, paint_color, alpha);
        }
        else
        {
            ctx.drawImage(m_resolved_path, icon_x, icon_y, draw_size, draw_size, alpha);
        }
    }

} // namespace horizon
