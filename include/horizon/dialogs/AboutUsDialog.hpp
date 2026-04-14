#pragma once

#include "horizon/Notebook.hpp"
#include "horizon/WaylandWindow.hpp"
#include <memory>
#include <string>

namespace horizon
{

    struct AboutDialogContent
    {
        std::string title;
        std::string version;
        std::string icon;
        std::unique_ptr<Widget> about;
        std::unique_ptr<Widget> components;
        std::unique_ptr<Widget> auths;
        std::unique_ptr<Widget> thanks;
        std::unique_ptr<Widget> translate;
    };

    class AboutUsDialog : public WaylandWindow
    {
    public:
        AboutUsDialog() = default;
        ~AboutUsDialog() = default;

        void show()
        {
            setup_ui();
            build_tabs();
            WaylandWindow::run();
        }

        void build_tabs();

        void set_content(std::unique_ptr<AboutDialogContent> content)
        {
            m_content = std::move(content);
        }
        AboutDialogContent *get_content()
        {
            return m_content.get();
        }

    private:
        void setup_ui();

        Notebook *m_notebook;
        std::unique_ptr<AboutDialogContent> m_content = nullptr;
    };
} // namespace horizon
