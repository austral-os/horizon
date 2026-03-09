#include <cstring>
#include <grp.h>
#include <horizon/arkutils/FileInfo.hpp>
#include <libgen.h>
#include <pwd.h>
#include <vector>

namespace horizon::arkutils
{
    FileInfo FileInfo::from_path(const std::string &full_path)
    {
        FileInfo info;
        info.path = full_path;

        char path_buffer[1024];
        strncpy(path_buffer, full_path.c_str(), sizeof(path_buffer));
        path_buffer[sizeof(path_buffer) - 1] = '\0';
        info.name = basename(path_buffer);

        struct stat st;
        if (stat(full_path.c_str(), &st) != 0)
        {
            return info;
        }

        info.size = st.st_size;
        info.last_modified = std::chrono::system_clock::from_time_t(st.st_mtime);
        info.permissions = st.st_mode & 0777;

        if (S_ISREG(st.st_mode))
            info.type = FileType::Regular;
        else if (S_ISDIR(st.st_mode))
            info.type = FileType::Directory;
        else if (S_ISLNK(st.st_mode))
            info.type = FileType::Symlink;
        else if (S_ISSOCK(st.st_mode))
            info.type = FileType::Socket;
        else if (S_ISCHR(st.st_mode))
            info.type = FileType::CharacterDevice;
        else if (S_ISBLK(st.st_mode))
            info.type = FileType::BlockDevice;
        else if (S_ISFIFO(st.st_mode))
            info.type = FileType::FIFO;

        // Hidden files
        if (!info.name.empty() && info.name[0] == '.')
        {
            info.is_hidden = true;
        }

        // Extension
        size_t dot_pos = info.name.find_last_of('.');
        if (dot_pos != std::string::npos && dot_pos != 0)
        {
            info.extension = info.name.substr(dot_pos + 1);
        }

        // Owner/Group (basic implementation)
        struct passwd *pw = getpwuid(st.st_uid);
        if (pw)
            info.owner = pw->pw_name;

        struct group *gr = getgrgid(st.st_gid);
        if (gr)
            info.group = gr->gr_name;

        return info;
    }
} // namespace horizon::arkutils
