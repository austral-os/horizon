#pragma once

#include "horizon/Notebook.hpp"
#include "horizon/WaylandWindow.hpp"
#include <memory>
#include <string>

namespace horizon
{

    class AboutUsDialog : public WaylandWindow
    {
    public:
        AboutUsDialog(const std::string &title = "About us");
        ~AboutUsDialog() = default;

        void show()
        {
            build_tabs();
            WaylandWindow::run();
        }

        void build_tabs();

        void set_about_content(std::unique_ptr<Widget> content);
        Widget *get_about_content();

        void set_components_content(std::unique_ptr<Widget> content);
        Widget *get_components_content();

        void set_auths_content(std::unique_ptr<Widget> content);
        Widget *get_auths_content();

        void set_thanks_content(std::unique_ptr<Widget> content);
        Widget *get_thanks_content();

        void set_translate_content(std::unique_ptr<Widget> content);
        Widget *get_translate_content();

    private:
        void setup_ui(const std::string &title);

        Notebook *m_notebook;
        std::unique_ptr<Widget> m_page_about;
        std::unique_ptr<Widget> m_page_components;
        std::unique_ptr<Widget> m_page_auths;
        std::unique_ptr<Widget> m_page_thanks;
        std::unique_ptr<Widget> m_page_translate;
    };
} // namespace horizon
