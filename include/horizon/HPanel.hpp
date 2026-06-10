#pragma once
#include <horizon/Widget.hpp>

namespace horizon
{
    /**
     * @class HPanel
     * @brief A container that splits its area into top and bottom panels with a vertically resizable divider.
     */
    class HPanel : public Widget
    {
    public:
        HPanel();
        ~HPanel() = default;

        void draw(GraphicsContext &gc) override;
        void calculate_layout() override;
        Widget *hit_test(int x, int y) override;

        /**
         * @brief Override add_child to ensure we only have two widgets and track them.
         */
        void add_child(std::unique_ptr<Widget> child) override;

        /**
         * @brief Set the top panel height in pixels.
         */
        void set_top_height(int height);
        int top_height() const;

    private:
        int m_top_height{200};
        int m_divider_height{6};
        bool m_is_resizing{false};

        Widget *m_top_ptr{nullptr};
        Widget *m_bottom_ptr{nullptr};

        bool is_over_divider(int cursor_x, int cursor_y) const;
    };
} // namespace horizon
