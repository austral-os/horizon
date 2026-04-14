#include "horizon/Link.hpp"
#include "horizon/ApplicationLauncher.hpp"
#include "horizon/Color.hpp"

namespace horizon
{

    Link::Link() : Label()
    {
        set_cursor_type(CursorType::Pointer);
        // Typical primary link color: blueish. We use the accent color approach or default blue
        set_text_color(Color(0.2f, 0.4f, 0.8f, 1.0f));

        when_click.connect([this](auto & /*event*/) {
            if (!m_url.empty())
            {
                ApplicationLauncher::launch_binary("xdg-open", {m_url});
            }
        });
    }

    Link::Link(const std::string &text, const std::string &url) : Label(text), m_url(url)
    {
        set_cursor_type(CursorType::Pointer);
        set_text_color(Color(0.2f, 0.4f, 0.8f, 1.0f));

        when_click.connect([this](auto & /*event*/) {
            if (!m_url.empty())
            {
                ApplicationLauncher::launch_binary("xdg-open", {m_url});
            }
        });
    }

    void Link::set_url(const std::string &url)
    {
        m_url = url;
    }

    const std::string &Link::url() const
    {
        return m_url;
    }

} // namespace horizon
