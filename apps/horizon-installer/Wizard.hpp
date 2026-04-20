#pragma once

#include <horizon/Widget.hpp>
#include <vector>
#include <memory>

namespace horizon::installer
{
    /**
     * @brief A widget that manages a sequence of pages.
     */
    class Wizard : public Widget
    {
    public:
        Wizard();
        ~Wizard() override;

        /**
         * @brief Adds a page to the wizard.
         */
        void add_page(std::unique_ptr<Widget> page);

        /**
         * @brief Shows the next page.
         */
        void next();

        /**
         * @brief Shows the previous page.
         */
        void back();

        /**
         * @brief Returns the index of the current page.
         */
        size_t current_page_index() const { return m_current_index; }

    private:
        std::vector<Widget*> m_pages;
        size_t m_current_index = 0;

        void update_visible_page();
    };
} // namespace horizon::installer
