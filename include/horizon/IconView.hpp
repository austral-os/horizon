#pragma once

#include <filesystem>
#include <horizon/ScrollArea.hpp>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    /**
     * @class IconView
     * @brief A widget that displays a grid of files and directories from a given path.
     */
    class IconView : public Widget
    {
    public:
        IconView();
        ~IconView() = default;

        void set_directory(const std::string &path);
        const std::string &directory() const;

        void set_directories_first(bool first);
        bool directories_first() const;

        void set_zoom(float zoom);
        float zoom() const;

        void refresh();

        void calculate_layout() override;
        void draw(GraphicsContext &gc) override;

    private:
        ScrollArea *m_scroll_area{nullptr};
        Widget *m_content_pane{nullptr};

        std::string m_directory_path;
        bool m_directories_first{true};
        float m_zoom{1.0f};

        int m_item_width{100};
        int m_item_height{120};
        int m_grid_spacing{10};

        const int BASE_ITEM_WIDTH{100};
        const int BASE_ITEM_HEIGHT{120};
        const int BASE_GRID_SPACING{10};

        std::string get_icon_for_entry(const std::filesystem::directory_entry &entry);
    };
} // namespace horizon
