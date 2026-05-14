#pragma once

#include "CompressionTask.hpp"
#include <string>
#include <vector>
#include <memory>

namespace horizon::compression
{
    enum class ArchiveFormat
    {
        Zip,
        TarGz,
        TarXz,
        SevenZip,
        Rar // Extraction only usually
    };

    class CompressionManager
    {
    public:
        /**
         * @brief Intelligent extraction: 
         * If archive has a single root folder, extracts directly.
         * Otherwise, creates a folder with the archive name.
         */
        static std::shared_ptr<CompressionTask> extract_smart(const std::string& archive_path, const std::string& destination_dir);

        /**
         * @brief Compresses multiple files/folders into an archive.
         */
        static std::shared_ptr<CompressionTask> compress(const std::vector<std::string>& sources, const std::string& output_path, ArchiveFormat format);

        /**
         * @brief Check if a file is a supported archive.
         */
        static bool is_supported_archive(const std::string& path);
        
        /**
         * @brief Get format from extension.
         */
        static ArchiveFormat format_from_extension(const std::string& path);
    };
} // namespace horizon::compression
