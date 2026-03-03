#pragma once
#include <horizon/Widget.hpp>

namespace horizon
{
    /**
     * @class VPanel
     * @brief A container that splits its area into left and right panels with a resizable divider.
     */
    class VPanel : public Widget
    {
    public:
        VPanel();
        ~VPanel() = default;

        void draw(GraphicsContext &gc) override;
        void calculate_layout() override;

        /**
         * @brief Override add_child to ensure we only have two widgets and track them.
         */
        void add_child(std::unique_ptr<Widget> child) override;

        /**
         * @brief Set the left panel width in pixels.
         */
        void set_left_width(int width);
        int left_width() const;

    private:
        int m_left_width{200};
        int m_divider_width{6};
        bool m_is_resizing{false};

        Widget *m_left_ptr{nullptr};
        Widget *m_right_ptr{nullptr};

        bool is_over_divider(int cursor_x, int cursor_y) const;
    };
} // namespace horizon
