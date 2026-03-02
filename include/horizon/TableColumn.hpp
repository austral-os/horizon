#pragma once

#include <functional>
#include <horizon/Widget.hpp>
#include <memory>
#include <string>

namespace horizon
{
    /**
     * @brief Policy for column width calculation.
     */
    enum class ColumnWidthPolicy
    {
        Fixed,
        Flexible
    };

    /**
     * @struct TableColumn
     * @brief Defines a column in the TableView for a specific data type T.
     */
    template <typename T> struct TableColumn
    {
        std::string id;
        std::string title;
        int width{100};
        ColumnWidthPolicy width_policy{ColumnWidthPolicy::Flexible};
        bool sortable{true};

        /**
         * @brief Factory function to create a widget for a cell in this column.
         * Default implementation returns a Label with the string representation of the data.
         */
        std::function<std::unique_ptr<Widget>(const T &)> cell_factory;

        /**
         * @brief Function to compare two objects for sorting based on this column.
         * Should return true if a < b for ascending sort.
         */
        std::function<bool(const T &, const T &)> sort_predicate;
    };
} // namespace horizon
