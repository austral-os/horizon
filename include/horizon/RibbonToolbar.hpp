#pragma once

#include <horizon/Frame.hpp>
#include <horizon/Widget.hpp>
#include <memory>
#include <string>
#include <vector>

namespace horizon
{

    class RibbonFrame : public Frame
    {
    public:
        RibbonFrame();
        void draw(GraphicsContext &ctx) override;
    };

    class RibbonSection : public Widget
    {
    public:
        RibbonSection(const std::string &title);
        virtual ~RibbonSection() = default;

        void add_widget(std::unique_ptr<Widget> widget);
        int preferred_width() const override;

    protected:
        void draw(GraphicsContext &ctx) override;
        void calculate_layout() override;

    private:
        std::string m_title;
        Widget *m_content_area; // Horizontal layout area for section widgets
    };

    class RibbonToolbar : public Widget
    {
    public:
        RibbonToolbar();
        virtual ~RibbonToolbar() = default;

        /**
         * @brief Adds a new tab and returns its index.
         */
        int add_tab(const std::string &title);

        /**
         * @brief Adds a section to a specific tab.
         */
        RibbonSection *add_section(int tab_index, const std::string &section_title);

        void set_active_tab(int index);

    protected:
        void render(GraphicsContext &gc, int cx, int cy, int cw, int ch,
                    bool force = false) override;
        void draw(GraphicsContext &ctx) override;
        Widget *hit_test(int x, int y) override;
        void calculate_layout() override;
        void set_application_recursive(WaylandWindow *app) override;

    private:
        Widget *m_header;             // Container for tab titles (sidebar_bg)
        RibbonFrame *m_content_frame; // Content frame (window_bg)
        Widget *m_container;

        struct TabData
        {
            std::string title;
            std::unique_ptr<Widget> content_container; // Horizontal layout for sections
            Widget *button;
        };

        std::vector<TabData> m_tabs;
        int m_active_tab_index = -1;

        double m_scroll_x = 0;
        double m_max_scroll_x = 0;

        void handle_mouse_wheel(MouseWheelEventContext &ev);
    };

} // namespace horizon
