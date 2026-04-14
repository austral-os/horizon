#pragma once

#include <horizon/Frame.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <string>

namespace horizon
{

    struct NotebookPage
    {
        std::string label{""};
        std::string icon{""};
        std::unique_ptr<Widget> body{nullptr};
        NotebookPage(std::string label, std::string icon, std::unique_ptr<Widget> body)
            : label(label), icon(icon), body(std::move(body))
        {
        }
        NotebookPage(std::string label, std::unique_ptr<Widget> body)
            : label(label), body(std::move(body))
        {
        }
    };

    class Notebook : public Widget
    {
    public:
        Notebook();
        ~Notebook();

        void render(GraphicsContext &ctx, int cx, int cy, int cw, int ch,
                    bool force = false) override;
        void draw(GraphicsContext &ctx) override;

        void add_tab(NotebookPage page);
        void add_tab(std::unique_ptr<NotebookPage> page);

        void set_current_tab(int index);

    private:
        void configure_header();

    protected:
        Widget *m_margin_top;
        Widget *m_header;
        Frame *m_body;
        Widget *m_tab_content;

        int m_current_tab{-1};
    };
} // namespace horizon
