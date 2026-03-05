#include <filesystem>
#include <horizon/GraphicsContext.hpp>
#include <horizon/IconView.hpp>
#include <horizon/IconViewItem.hpp>
#include <iostream>

namespace fs = std::filesystem;

namespace horizon
{
    IconView::IconView() : Widget()
    {
        m_background_color = Color(1.0f, 1.0f, 1.0f, 1.0f); // Default white background
    }

    void IconView::set_directory(const std::string &path)
    {
        m_directory_path = path;
        refresh();
    }

    const std::string &IconView::directory() const
    {
        return m_directory_path;
    }

    void IconView::refresh()
    {
        clear_children();

        if (m_directory_path.empty() || !fs::exists(m_directory_path) ||
            !fs::is_directory(m_directory_path))
        {
            return;
        }

        try
        {
            for (const auto &entry : fs::directory_iterator(m_directory_path))
            {
                auto item = std::make_unique<IconViewItem>();
                item->set_text(entry.path().filename().string());
                std::string icon = get_icon_for_entry(entry);
                item->set_icon_name(icon);
                add_child(std::move(item));
            }
            std::cout << "[IconView] Loaded " << m_children.size() << " items from "
                      << m_directory_path << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "IconView Error: " << e.what() << std::endl;
        }

        invalidate();
    }

    void IconView::calculate_layout()
    {
        if (m_width <= 0)
        {
            // If we have a parent (like ScrollArea), try to take its width
            if (m_parent && m_parent->width() > 0)
            {
                m_width = m_parent->width();
            }
            else
            {
                return;
            }
        }

        int available_width = m_width - 2 * m_margin;
        int columns = std::max(1, available_width / (m_item_width + m_grid_spacing));

        int current_col = 0;
        int current_row = 0;

        for (auto &child : m_children)
        {
            int x = m_x + m_margin + current_col * (m_item_width + m_grid_spacing);
            int y = m_y + m_margin + current_row * (m_item_height + m_grid_spacing);

            child->set_position(x, y);
            child->set_size(m_item_width, m_item_height);

            current_col++;
            if (current_col >= columns)
            {
                current_col = 0;
                current_row++;
            }
        }

        // Set our height based on content
        int total_rows = (m_children.size() + columns - 1) / columns;
        int needed_height = total_rows * (m_item_height + m_grid_spacing) + 2 * m_margin;

        if (m_height != needed_height)
        {
            m_height = needed_height;
            // std::cout << "[IconView] New height: " << m_height << " (rows: " << total_rows << ")"
            // << std::endl; No need to invalidate() here as we are in calculate_layout, but parent
            // might need it
        }
    }

    void IconView::draw(GraphicsContext &gc)
    {
        gc.setColor(m_background_color);
        gc.fillRect(m_x, m_y, m_width, m_height);
    }

    std::string IconView::get_icon_for_entry(const fs::directory_entry &entry)
    {
        if (entry.is_directory())
        {
            return "folder";
        }

        std::string ext = entry.path().extension().string();
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".svg")
        {
            return "image-x-generic";
        }
        if (ext == ".mp3" || ext == ".wav" || ext == ".ogg")
        {
            return "audio-x-generic";
        }
        if (ext == ".mp4" || ext == ".mkv" || ext == ".avi")
        {
            return "video-x-generic";
        }
        if (ext == ".pdf")
        {
            return "document-pdf";
        }
        if (ext == ".zip" || ext == ".tar" || ext == ".gz" || ext == ".7z")
        {
            return "package-x-generic";
        }

        return "text-x-generic";
    }
} // namespace horizon
