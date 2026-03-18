#include "horizon/Icon.hpp"
#include "horizon/IconThemeLookup.hpp"
#include <horizon/Logger.hpp>

namespace horizon
{

    Icon::Icon() : Widget()
    {
        set_fixed_size(m_icon_size);
    }

    Icon::~Icon() = default;

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
        set_fixed_size(size);
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

    void Icon::resolve_icon()
    {
        if (m_icon_name.empty())
        {
            m_resolved_path = "";
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

        ctx.drawImage(m_resolved_path, icon_x, icon_y, draw_size, draw_size);
    }

} // namespace horizon
