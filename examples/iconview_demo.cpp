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

    void set_data(const FileData &data, float zoom)
    {
        m_zoom = zoom;
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

        m_icon_size = static_cast<int>(48 * m_zoom);
        m_icon_ptr->set_icon_name(icon_name);
        m_icon_ptr->set_icon_size(m_icon_size);
        m_label_ptr->set_font_size(static_cast<int>(12 * m_zoom));
        invalidate();
    }

    void calculate_layout() override
    {
        int padding = static_cast<int>(4 * m_zoom);
        int icon_y = padding;
        int label_y = icon_y + m_icon_size + padding;

        m_icon_ptr->set_position(m_x + (m_width - m_icon_size) / 2, m_y + icon_y);
        m_icon_ptr->set_size(m_icon_size, m_icon_size);

        m_label_ptr->set_position(m_x + padding, m_y + label_y);
        m_label_ptr->set_size(m_width - 2 * padding, m_height - label_y - padding);
    }

    void draw(GraphicsContext &gc) override
    {
        if (is_hovered())
        {
            gc.setColor(Color(0.8f, 0.8f, 0.9f, 0.5f));
            gc.fillRect(m_x, m_y, m_width, m_height, CornerRadius(6));
        }
        else if (is_pressed())
        {
            gc.setColor(Color(0.7f, 0.7f, 0.8f, 0.7f));
            gc.fillRect(m_x, m_y, m_width, m_height, CornerRadius(6));
        }
    }

private:
    Icon *m_icon_ptr{nullptr};
    Label *m_label_ptr{nullptr};
    float m_zoom{1.0f};
    int m_icon_size{48};
};

int main(int argc, char *argv[])
{
    auto app = std::make_unique<Application>(600, 400);
    app->set_app_id("org.horizon.iconview_demo");

    auto window = std::make_unique<ApplicationWindow>("IconView Demo");
    window->set_size(600, 400);

    auto icon_view = std::make_unique<IconView<FileData>>();
    icon_view->set_zoom(1.0f);

    // Factory for creating items
    icon_view->set_item_factory(
        [](const FileData &data, float zoom)
        {
            auto item = std::make_unique<FileItem>();
            item->set_data(data, zoom);
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
