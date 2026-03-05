#include <algorithm>
#include <filesystem>
#include <horizon/Application.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/IconView.hpp>
#include <horizon/IconViewItem.hpp>
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

std::string get_icon_for_file(const FileData &data)
{
    if (data.is_directory)
    {
        return "folder";
    }

    fs::path p(data.name);
    std::string ext = p.extension().string();
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
            auto item = std::make_unique<IconViewItem>();
            item->set_text(data.name);
            item->set_icon_name(get_icon_for_file(data));
            item->set_zoom(zoom);
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
                          return a.is_directory; // true (dir) < false (file) is not what we want,
                                                 // so dir comes first
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
