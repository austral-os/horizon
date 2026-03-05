#include <algorithm>
#include <filesystem>
#include <horizon/Application.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/Icon.hpp>
#include <horizon/IconView.hpp>
#include <horizon/Label.hpp>
#include <iostream>
#include <vector>

using namespace horizon;
namespace fs = std::filesystem;

struct FileData
{
    std::string name;
    bool is_directory;
    std::string path;
};

class FileItem : public Widget
{
public:
    FileItem() : Widget()
    {
        auto icon = std::make_unique<Icon>();
        icon->set_position_type(FREE);
        m_icon_ptr = icon.get();
        add_child(std::move(icon));

        auto label = std::make_unique<Label>();
        label->set_position_type(FREE);
        label->set_alignment(TextAlignment::Center);
        label->set_vertical_alignment(VerticalAlignment::Top);
        m_label_ptr = label.get();
        add_child(std::move(label));

        m_position_type = FREE;
    }

    void set_data(const FileData &data, float zoom, bool selected)
    {
        m_zoom = zoom;
        m_selected = selected;
        m_label_ptr->set_text(data.name);

        std::string icon_name = data.is_directory ? "folder" : "text-x-generic";
        if (!data.is_directory)
        {
            fs::path p(data.name);
            std::string ext = p.extension().string();
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".svg")
                icon_name = "image-x-generic";
            else if (ext == ".mp3" || ext == ".wav" || ext == ".ogg")
                icon_name = "audio-x-generic";
            else if (ext == ".mp4" || ext == ".mkv" || ext == ".avi")
                icon_name = "video-x-generic";
            else if (ext == ".pdf")
                icon_name = "document-pdf";
            else if (ext == ".zip" || ext == ".tar" || ext == ".gz" || ext == ".7z")
                icon_name = "package-x-generic";
        }

        m_icon_size = static_cast<int>(32 * m_zoom); // Slightly smaller icon
        m_icon_ptr->set_icon_name(icon_name);
        m_icon_ptr->set_icon_size(m_icon_size);
        // Default font size handled by set_data caller or base theme
        invalidate();
    }

    void set_font_size(int size)
    {
        m_label_ptr->set_font_size(size);
        invalidate();
    }

    void calculate_layout() override
    {
        int padding = static_cast<int>(6 * m_zoom);
        int icon_y = padding;

        // Center icon horizontally
        m_icon_ptr->set_position(m_x + (m_width - m_icon_size) / 2, m_y + icon_y);
        m_icon_ptr->set_size(m_icon_size, m_icon_size);

        // Position label below icon
        int label_y = icon_y + m_icon_size + 4;
        m_label_ptr->set_position(m_x + 2, m_y + label_y);
        m_label_ptr->set_size(m_width - 4, m_height - label_y - 2);
    }

    void draw(GraphicsContext &gc) override
    {
        bool highlighted = m_selected || is_hovered() || is_pressed();
        if (highlighted)
        {
            Color base_color;
            Color border_color;

            if (m_selected)
            {
                base_color = Color(0.2f, 0.4f, 0.8f, 0.2f);
                border_color = Color(0.2f, 0.4f, 0.8f, 0.4f);
            }
            else if (is_pressed())
            {
                base_color = Color(0.4f, 0.4f, 0.4f, 0.2f);
                border_color = Color(0.4f, 0.4f, 0.4f, 0.3f);
            }
            else
            {
                base_color = Color(0.0f, 0.0f, 0.0f, 0.05f);
                border_color = Color(0.0f, 0.0f, 0.0f, 0.1f);
            }

            gc.setColor(base_color);
            gc.fillRect(m_x, m_y, m_width, m_height, CornerRadius(6));

            gc.setColor(border_color);
            gc.drawRect(m_x, m_y, m_width, m_height, CornerRadius(6));
        }

        if (m_selected)
        {
            m_label_ptr->set_font_weight(FONT_WEIGHT_BOLD);
        }
        else
        {
            m_label_ptr->set_font_weight(FONT_WEIGHT_NORMAL);
        }
    }

private:
    Icon *m_icon_ptr{nullptr};
    Label *m_label_ptr{nullptr};
    float m_zoom{1.0f};
    int m_icon_size{32};
    bool m_selected{false};
};

int main(int argc, char *argv[])
{
    auto app = std::make_unique<Application>(600, 400);
    app->set_app_id("org.horizon.iconview_demo");

    auto window = std::make_unique<ApplicationWindow>("IconView Demo");
    window->set_size(600, 410);

    auto icon_view = std::make_unique<IconView<FileData>>();
    auto icon_view_ptr = icon_view.get();
    icon_view->set_zoom(1.5f);

    // Factory for creating items
    icon_view->set_item_factory(
        [icon_view_ptr](const FileData &data, float zoom, bool selected)
        {
            auto item = std::make_unique<FileItem>();
            item->set_data(data, zoom, selected);

            int base_font_size = icon_view_ptr->get_theme_font_size();
            item->set_font_size(static_cast<int>(base_font_size * zoom));

            // Handle selection
            item->when_mouse_press.connect(
                [icon_view_ptr, item_ptr = item.get()](EventContext &ctx)
                {
                    // Find index of this item in content pane
                    auto &children = icon_view_ptr->children(); // Faulty, it's ScrollArea
                    // We need to access content_pane children. IconViewBase has it as protected.
                    // Wait, IconViewBase is the parent of rebuilding items.
                    // Re-calculating index is better.

                    // Let's use a capture to know our index or just loop.
                    auto content_pane = item_ptr->parent();
                    if (content_pane)
                    {
                        auto &items = content_pane->children();
                        for (int i = 0; i < (int)items.size(); ++i)
                        {
                            if (items[i].get() == item_ptr)
                            {
                                icon_view_ptr->set_selected_index(i);
                                break;
                            }
                        }
                    }
                });

            return item;
        });

    // Load data from filesystem
    std::string path = "/home/horacio";
    std::vector<FileData> files;

    if (fs::exists(path) && fs::is_directory(path))
    {
        for (const auto &entry : fs::directory_iterator(path))
        {
            files.push_back(
                {entry.path().filename().string(), entry.is_directory(), entry.path().string()});
        }

        // Sort: directories first, then alphabetical
        std::sort(files.begin(), files.end(),
                  [](const FileData &a, const FileData &b)
                  {
                      if (a.is_directory != b.is_directory)
                      {
                          return a.is_directory;
                      }
                      return a.name < b.name;
                  });
    }

    icon_view->set_data(files);

    window->add_child(std::move(icon_view));
    app->set_root(std::move(window));

    std::cout << "Starting IconView Demo..." << std::endl;
    app->run();
    return 0;
}
