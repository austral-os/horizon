#pragma once

#include "horizon/Notebook.hpp"
#include "horizon/WaylandWindow.hpp"
#include "horizon/About.hpp"
#include <memory>
#include <string>

namespace horizon
{
    class AboutUsDialog : public WaylandWindow
    {
    public:
        AboutUsDialog(AboutManager &manager);
        ~AboutUsDialog() = default;

        void show()
        {
            setup_ui();
            build_tabs();
            WaylandWindow::run();
        }

    private:
        void setup_ui();
        void build_tabs();
        std::unique_ptr<Widget> create_info_page(const About &data);

        Notebook *m_notebook;
        AboutManager &m_manager;
    };
} // namespace horizon
