#include "horizon/Icon.hpp"
#include "horizon/IconThemeLookup.hpp"
#include <iostream>

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

    void Icon::resolve_icon()
    {
        if (m_icon_name.empty())
        {
            m_resolved_path.clear();
            return;
        }

        m_resolved_path = IconThemeLookup::find_icon(m_icon_name, m_icon_size);
        if (m_resolved_path.empty())
        {
            std::cout << "[Icon] Failed to resolve icon: \"" << m_icon_name << "\" at size "
                      << m_icon_size << std::endl;
        }
    }

    void Icon::draw(GraphicsContext &ctx)
    {
        if (m_resolved_path.empty())
            return;

        // Use the requested icon size
        int draw_size = m_icon_size;

        int icon_x = m_start_draw_x + (m_available_draw_width - draw_size) / 2;
        int icon_y = m_start_draw_y + (m_available_draw_height - draw_size) / 2;

        ctx.drawImage(m_resolved_path, icon_x, icon_y, draw_size, draw_size);
    }

} // namespace horizon
