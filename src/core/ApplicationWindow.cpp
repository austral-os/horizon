#include <horizon/ApplicationWindow.hpp>
#include <horizon/Toolbar.hpp>

namespace horizon
{
    ApplicationWindow::ApplicationWindow(std::string title)
        : Window(std::make_unique<Toolbar>(std::move(title)))
    {
    }

    Toolbar *ApplicationWindow::toolbar() const
    {
        // m_titlebar is Titlebar*, but in ApplicationWindow it points to a Toolbar
        return static_cast<Toolbar *>(m_titlebar);
    }
} // namespace horizon
