#pragma once

#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    class Icon;
    class Label;

    /**
     * @class IconViewItem
     * @brief A widget that represents a single item in an IconView (e.g., a file or directory).
     * Consists of an Icon and a Label below it.
     */
    class IconViewItem : public Widget
    {
    public:
        IconViewItem();
        ~IconViewItem() = default;

        void set_text(const std::string &text);
        const std::string &text() const;

        void set_icon_name(const std::string &icon_name);
        const std::string &icon_name() const;

        void calculate_layout() override;
        void draw(GraphicsContext &gc) override;

        int preferred_width() const override;
        int preferred_height() const override;

    private:
        Icon *m_icon_ptr{nullptr};
        Label *m_label_ptr{nullptr};
        int m_icon_size{48};
    };
} // namespace horizon
