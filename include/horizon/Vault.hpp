#pragma once
#include <horizon/Widget.hpp>
#include <memory>

namespace horizon
{
    /**
     * @brief A Vault is a floating popover widget that can contain any other widget.
     * It is used to display interactive content in a floating bubble relative to an owner widget.
     */
    class Vault : public Widget
    {
    public:
        Vault();
        ~Vault() = default;

        /**
         * @brief Sets the content widget to be displayed inside the vault.
         */
        void set_content(std::unique_ptr<Widget> content);

        /**
         * @brief Returns the current content widget.
         */
        Widget* content() const;

        /**
         * @brief Calculates the layout based on the content's preferred size.
         */
        void calculate_layout() override;

        /**
         * @brief Performs a hit test, including the content widget.
         */
        Widget *hit_test(int x, int y) override;

        /**
         * @brief Sets the arrow position (relative to the vault).
         */
        void set_arrow_position(int x, int y);

    protected:
        void draw(GraphicsContext &gc) override;

    private:
        int m_arrow_x = 0;
        int m_arrow_y = 0;
        int m_padding = 12;
        int m_border_radius = 12;
    };

} // namespace horizon
