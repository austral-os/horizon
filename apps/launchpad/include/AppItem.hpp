#pragma once

#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{

    struct AppData
    {
        std::string name;
        std::string icon_name;
        std::string exec;
    };

    class AppItem : public Widget
    {
    public:
        AppItem();

        void set_data(const AppData &data, float zoom, bool selected);
        void set_font_size(int size);

        int preferred_height(int width) const override;
        void calculate_layout() override;
        void draw(GraphicsContext &gc) override;

    private:
        Icon *m_icon_ptr{nullptr};
        Label *m_label_ptr{nullptr};
        float m_zoom{1.0f};
        int m_icon_size{64};
        bool m_selected{false};
    };

} // namespace horizon
