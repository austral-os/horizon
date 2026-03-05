#include <algorithm>
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

        auto scroll_area = std::make_unique<ScrollArea>();
        scroll_area->set_position_type(FILL);
        m_scroll_area = scroll_area.get();

        auto content_pane = std::make_unique<Widget>();
        content_pane->set_position_type(FREE);
        m_content_pane = content_pane.get();

        m_scroll_area->set_content(std::move(content_pane));
        add_child(std::move(scroll_area));
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

    void IconView::set_directories_first(bool first)
    {
        if (m_directories_first != first)
        {
            m_directories_first = first;
            refresh();
        }
    }

    bool IconView::directories_first() const
    {
        return m_directories_first;
    }

    void IconView::set_zoom(float zoom)
    {
        if (m_zoom != zoom)
        {
            m_zoom = zoom;
            for (auto &child : m_content_pane->children())
            {
                auto item = dynamic_cast<IconViewItem *>(child.get());
                if (item)
                {
                    item->set_zoom(m_zoom);
                }
            }
            invalidate();
            calculate_layout();
        }
    }

    float IconView::zoom() const
    {
        return m_zoom;
    }

    void IconView::refresh()
    {
        m_content_pane->clear_children();

        if (m_directory_path.empty() || !fs::exists(m_directory_path) ||
            !fs::is_directory(m_directory_path))
        {
            return;
        }

        try
        {
            std::vector<fs::directory_entry> entries;
            for (const auto &entry : fs::directory_iterator(m_directory_path))
            {
                entries.push_back(entry);
            }

            // Sort entries
            std::sort(entries.begin(), entries.end(),
                      [this](const fs::directory_entry &a, const fs::directory_entry &b)
                      {
                          if (m_directories_first)
                          {
                              bool a_is_dir = a.is_directory();
                              bool b_is_dir = b.is_directory();
                              if (a_is_dir != b_is_dir)
                              {
                                  return a_is_dir; // Directories first
                              }
                          }
                          // Sort by filename (case-insensitive would be better, but let's stick to
                          // simple for now)
                          return a.path().filename().string() < b.path().filename().string();
                      });

            for (const auto &entry : entries)
            {
                auto item = std::make_unique<IconViewItem>();
                item->set_text(entry.path().filename().string());
                std::string icon = get_icon_for_entry(entry);
                item->set_icon_name(icon);
                item->set_zoom(m_zoom);
                m_content_pane->add_child(std::move(item));
            }
            std::cout << "[IconView] Loaded " << m_content_pane->children().size() << " items from "
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
        Widget::calculate_layout();

        if (m_width <= 0 || m_height <= 0)
        {
            if (m_parent && m_parent->width() > 0 && m_parent->height() > 0)
            {
                m_width = m_parent->width();
                m_height = m_parent->height();
            }
            else
            {
                return;
            }
        }

        if (m_scroll_area)
        {
            m_scroll_area->set_size(m_width, m_height);
        }

        m_item_width = static_cast<int>(BASE_ITEM_WIDTH * m_zoom);
        m_item_height = static_cast<int>(BASE_ITEM_HEIGHT * m_zoom);
        m_grid_spacing = static_cast<int>(BASE_GRID_SPACING * m_zoom);

        int available_width = m_width - 2 * m_margin;
        int columns = std::max(1, available_width / (m_item_width + m_grid_spacing));

        int current_col = 0;
        int current_row = 0;

        for (auto &child : m_content_pane->children())
        {
            int x = m_x - m_scroll_area->scroll_x() + m_margin +
                    current_col * (m_item_width + m_grid_spacing);
            int y = m_y - m_scroll_area->scroll_y() + m_margin +
                    current_row * (m_item_height + m_grid_spacing);

            child->set_position(x, y);
            child->set_size(m_item_width, m_item_height);

            current_col++;
            if (current_col >= columns)
            {
                current_col = 0;
                current_row++;
            }
        }

        // Set content height based on rows
        int total_rows = (m_content_pane->children().size() + columns - 1) / columns;
        int needed_height = total_rows * (m_item_height + m_grid_spacing) + 2 * m_margin;

        m_content_pane->set_size(m_width, needed_height);
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
