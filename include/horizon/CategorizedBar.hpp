#pragma once

#include <functional>
#include <horizon/Color.hpp>
#include <horizon/Widget.hpp>
#include <string>
#include <vector>

namespace horizon
{
    /**
     * @brief A category for the CategorizedBar widget.
     */
    struct BarCategory
    {
        std::string name;
        double value;
        Color color;
    };

    /**
     * @brief A widget that displays multiple categories in a horizontal bar with a legend.
     */
    class CategorizedBar : public Widget
    {
    public:
        CategorizedBar();
        virtual ~CategorizedBar() = default;

        /**
         * @brief Add a category to the bar.
         */
        void add_category(const std::string &name, double value, const Color &color);

        /**
         * @brief Clear all categories.
         */
        void clear_categories();

        /**
         * @brief Set the total value (capacity). If 0, the sum of category values is used.
         */
        void set_total_value(double total);

        /**
         * @brief Set a custom formatter for the values in the legend.
         */
        void set_value_formatter(std::function<std::string(double)> formatter);

        void draw(GraphicsContext &gc) override;

        int preferred_width() const override;
        int preferred_height() const override;
        int preferred_height(int width) const override;

        /**
         * @brief A helper formatter for bytes (B, KB, MB, GB, TB).
         */
        static std::string format_bytes(double bytes);

    private:
        std::vector<BarCategory> m_categories;
        double m_total_value{0.0};
        std::function<std::string(double)> m_formatter;

        static std::string default_formatter(double value);
    };
} // namespace horizon
