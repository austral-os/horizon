#include <horizon/arkutils/DirectoryWatcher.hpp>
#include <iostream>
#include <sys/inotify.h>
#include <unistd.h>

namespace horizon::arkutils
{
    DirectoryWatcher::DirectoryWatcher()
    {
        m_fd = inotify_init1(IN_NONBLOCK);
    }

    DirectoryWatcher::~DirectoryWatcher()
    {
        stop();
        if (m_fd != -1)
        {
            close(m_fd);
        }
    }

    bool DirectoryWatcher::watch(const std::string &path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_fd == -1)
            return false;

        int wd = inotify_add_watch(m_fd, path.c_str(),
                                   IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO |
                                       IN_ATTRIB);
        if (wd == -1)
            return false;

        m_watch_descriptors[wd] = path;
        m_path_to_wd[path] = wd;
        return true;
    }

    void DirectoryWatcher::unwatch(const std::string &path)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_path_to_wd.find(path);
        if (it != m_path_to_wd.end())
        {
            inotify_rm_watch(m_fd, it->second);
            m_watch_descriptors.erase(it->second);
            m_path_to_wd.erase(it);
        }
    }

    void DirectoryWatcher::unwatch_all()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto &[path, wd] : m_path_to_wd)
        {
            inotify_rm_watch(m_fd, wd);
        }
        m_watch_descriptors.clear();
        m_path_to_wd.clear();
    }

    void DirectoryWatcher::set_callback(Callback callback)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callback = callback;
    }

    void DirectoryWatcher::start()
    {
        if (m_running)
            return;
        m_running = true;
        m_thread = std::thread(&DirectoryWatcher::run, this);
    }

    void DirectoryWatcher::stop()
    {
        m_running = false;
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void DirectoryWatcher::run()
    {
        char buffer[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
        const struct inotify_event *event;

        while (m_running)
        {
            ssize_t len = read(m_fd, buffer, sizeof(buffer));
            if (len <= 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            for (char *ptr = buffer; ptr < buffer + len;
                 ptr += sizeof(struct inotify_event) + event->len)
            {
                event = (const struct inotify_event *)ptr;

                std::string path;
                Callback cb;
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    path = m_watch_descriptors[event->wd];
                    cb = m_callback;
                }

                if (!cb)
                    continue;

                WatchEvent we;
                if (event->mask & IN_CREATE)
                    we = WatchEvent::Created;
                else if (event->mask & IN_DELETE)
                    we = WatchEvent::Deleted;
                else if (event->mask & IN_MODIFY)
                    we = WatchEvent::Modified;
                else if (event->mask & IN_MOVED_FROM)
                    we = WatchEvent::MovedFrom;
                else if (event->mask & IN_MOVED_TO)
                    we = WatchEvent::MovedTo;
                else if (event->mask & IN_ATTRIB)
                    we = WatchEvent::AttributeChanged;
                else
                    continue;

                cb(path, event->len ? event->name : "", we);
            }
        }
    }
} // namespace horizon::arkutils
