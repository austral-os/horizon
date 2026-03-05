#pragma once

#include <filesystem>
#include <horizon/Widget.hpp>
#include <string>
#include <vector>

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

        void refresh();

        void calculate_layout() override;
        void draw(GraphicsContext &gc) override;

    private:
        std::string m_directory_path;
        int m_item_width{100};
        int m_item_height{120};
        int m_grid_spacing{10};

        std::string get_icon_for_entry(const std::filesystem::directory_entry &entry);
    };
} // namespace horizon
