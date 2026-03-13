#include <algorithm>
#include <filesystem>
#include <horizon/Application.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/Icon.hpp>
#include <horizon/IconView.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
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
        label->set_vertical_alignment(VerticalAlignment::Middle);
        m_label_ptr = label.get();
        add_child(std::move(label));

        m_position_type = FREE;

        when_mouse_enter.connect([this](EventContext &) { invalidate(); });
        when_mouse_leave.connect([this](EventContext &) { invalidate(); });
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

    int preferred_height(int width) const override
    {
        int padding = static_cast<int>(4 * m_zoom);
        int gap = 2; // icon-to-label gap
        int label_width = width - 4;

        int label_h = m_label_ptr->preferred_height(label_width);

        return padding + m_icon_size + gap + label_h + padding;
    }

    void calculate_layout() override
    {
        int padding = static_cast<int>(4 * m_zoom); // Reduced from 6
        int icon_y = padding;

        // Center icon horizontally
        m_icon_ptr->set_position(m_x + (m_width - m_icon_size) / 2, m_y + icon_y);
        m_icon_ptr->set_size(m_icon_size, m_icon_size);

        // Position label below icon
        int label_y = icon_y + m_icon_size + 2; // Reduced from 4
        m_label_ptr->set_position(m_x + 2, m_y + label_y);
        m_label_ptr->set_size(m_width - 4, m_height - label_y - 2);
    }

    void draw(GraphicsContext &gc) override
    {
        if (m_selected)
        {
            // MacOS Mountain Lion selection blue: #3875d7
            Color base_color = Color(0.22f, 0.46f, 0.84f, 1.0f);
            Color border_color = Color(0.22f, 0.46f, 0.84f, 1.0f);

            int padding = static_cast<int>(4 * m_zoom);
            int label_y = padding + m_icon_size + 2;
            int h_x = m_x + 2;
            int h_y = m_y + label_y;
            int h_w = m_width - 4;
            int h_h = m_height - label_y - 2;

            gc.setColor(base_color);
            gc.fillRect(h_x, h_y, h_w, h_h, CornerRadius(6));

            gc.setColor(border_color);
            gc.drawRect(h_x, h_y, h_w, h_h, CornerRadius(6));

            m_label_ptr->set_text_color(Color(1.0f, 1.0f, 1.0f, 1.0f));
            m_label_ptr->set_font_weight(FONT_WEIGHT_BOLD);
        }
        else
        {
            m_label_ptr->set_text_color(Color(0.0f, 0.0f, 0.0f, 1.0f));
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
    auto app = std::make_unique<Application>("org.horizon.iconview_demo", 600, 400);
    app->set_name("Icon view");

    auto window = std::make_unique<ApplicationWindow>(app.get(), "IconView Demo");
    window->set_size(600, 410);

    auto icon_view = std::make_unique<IconView<FileData>>();
    auto icon_view_ptr = icon_view.get();
    icon_view->set_zoom(1.5f);
    icon_view->set_item_size(80, 85); // Adjusted for better fit with labels

    // Factory for creating items
    icon_view->set_item_factory(
        [icon_view_ptr](const FileData &data, float zoom, bool selected)
        {
            auto item = std::make_unique<FileItem>();
            item->set_data(data, zoom, selected);

            int base_font_size = icon_view_ptr->get_theme_font_size();
            item->set_font_size(static_cast<int>(base_font_size * zoom));

            // Handle selection is now done automatically by IconView
            // but we could still add custom logic here if needed.

            return item;
        });

    icon_view->when_item_click.connect(
        [](IconViewItemMouseClickContext<FileData> &ctx)
        { LOG_INFO << "Clicked: " << ctx.item_data.name << " (index: " << ctx.item_index << ")"; });

    icon_view->when_item_dbl_click.connect(
        [](IconViewItemMouseClickContext<FileData> &ctx)
        { LOG_INFO << "Double clicked: " << ctx.item_data.name; });

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

    window->show_status_bar();
    window->set_status_text("Icon View Demo - 0 items selected");
    window->set_content(std::move(icon_view));
    app->set_root(std::move(window));

    LOG_INFO << "Starting IconView Demo...";
    app->run();
    return 0;
}
