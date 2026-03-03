#pragma once
#include <horizon/Icon.hpp>
#include <horizon/TextBox.hpp>
#include <memory>

namespace horizon
{
    /**
     * @class SearchBox
     * @brief A specialized TextBox with a search icon and a clear button.
     */
    class SearchBox : public TextBox<TextPolicy>
    {
    public:
        SearchBox();
        ~SearchBox() = default;

        void draw(GraphicsContext &gc) override;

        void calculate_layout() override;

    protected:
        // No need to override event handlers for clear click if we handle it in draw/press
    private:
        Icon *m_search_ptr{nullptr};
        Icon *m_clear_ptr{nullptr};

        bool m_is_clear_hovered{false};
    };
} // namespace horizon
