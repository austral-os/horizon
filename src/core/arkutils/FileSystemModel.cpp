#include <algorithm>
#include <filesystem>
#include <horizon/arkutils/FileInfo.hpp>
#include <horizon/arkutils/FileSystemModel.hpp>

namespace fs = std::filesystem;

namespace horizon::arkutils
{
    FileSystemModel::FileSystemModel()
    {
        m_watcher = std::make_unique<DirectoryWatcher>();
        m_watcher->set_callback(
            [this](const std::string &path, const std::string &filename, WatchEvent event)
            { this->on_watcher_event(path, filename, event); });
        m_watcher->start();
    }

    FileSystemModel::~FileSystemModel()
    {
        if (m_watcher)
            m_watcher->stop();
    }

    void FileSystemModel::unwatch_all()
    {
        if (m_watcher)
            m_watcher->unwatch_all();
    }

    std::vector<FileInfo> FileSystemModel::list_directory(const std::string &path,
                                                          bool force_refresh)
    {
        {
            std::lock_guard<std::mutex> lock(m_cache_mutex);
            if (!force_refresh && m_cache.find(path) != m_cache.end())
            {
                return m_cache[path];
            }
        }

        // Add to watcher if not already there
        m_watcher->watch(path);

        refresh_cache(path);

        std::lock_guard<std::mutex> lock(m_cache_mutex);
        return m_cache[path];
    }

    FileInfo FileSystemModel::get_info(const std::string &path)
    {
        return FileInfo::from_path(path);
    }

    void FileSystemModel::on_watcher_event(const std::string &path, const std::string &filename,
                                           WatchEvent event)
    {
        // Whenever something changes, we refresh the cache for that directory
        refresh_cache(path);

        // Notify the rest of the system
        SignalContext ctx;
        ctx.data = (void *)&path;
        m_signals.emit(SIGNAL_DIRECTORY_CHANGED, ctx);
    }

    void FileSystemModel::refresh_cache(const std::string &path)
    {
        std::vector<FileInfo> files;
        try
        {
            for (const auto &entry : fs::directory_iterator(path))
            {
                files.push_back(FileInfo::from_path(entry.path().string()));
            }

            // Sort by type (directories first) and then name
            std::sort(files.begin(), files.end(),
                      [](const FileInfo &a, const FileInfo &b)
                      {
                          if (a.type != b.type)
                          {
                              if (a.type == FileType::Directory)
                                  return true;
                              if (b.type == FileType::Directory)
                                  return false;
                          }
                          return a.name < b.name;
                      });

            std::lock_guard<std::mutex> lock(m_cache_mutex);
            m_cache[path] = std::move(files);
        }
        catch (...)
        {
            // Handle inaccessible directories
        }
    }
} // namespace horizon::arkutils
