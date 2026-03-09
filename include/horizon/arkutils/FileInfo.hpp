#pragma once

#include <chrono>
#include <string>
#include <sys/stat.h>

namespace horizon::arkutils
{
    /**
     * @enum FileType
     * @brief Represents the type of a filesystem entry.
     */
    enum class FileType
    {
        Unknown,
        Regular,
        Directory,
        Symlink,
        Socket,
        CharacterDevice,
        BlockDevice,
        FIFO
    };

    /**
     * @struct FileInfo
     * @brief Contains metadata for a single file or directory.
     */
    struct FileInfo
    {
        std::string name;
        std::string path;
        FileType type = FileType::Unknown;
        uint64_t size = 0;
        std::chrono::system_clock::time_point last_modified;
        bool is_hidden = false;
        std::string extension;
        uint32_t permissions = 0;
        std::string owner;
        std::string group;

        /**
         * @brief Creates a FileInfo object from a given path using stat.
         * @param full_path The absolute path to the file.
         * @return A FileInfo object with populated metadata.
         */
        static FileInfo from_path(const std::string &full_path);
    };
} // namespace horizon::arkutils
