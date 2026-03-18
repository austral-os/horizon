#include "ArkfmIconProvider.hpp"
#include <horizon/DesktopEntry.hpp>

namespace horizon::arkfm
{
    std::string ArkfmIconProvider::get_icon_name(const arkutils::FileInfo &f)
    {
        if (f.type == arkutils::FileType::Directory)
        {
            return "folder";
        }

        const auto &ext = f.extension;

        if (ext == "desktop")
        {
            std::string desktop_icon = DesktopEntry::get_value_from_desktop_file(f.path, "Icon");
            if (!desktop_icon.empty())
            {
                return desktop_icon;
            }
        }

        // Images
        if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "svg" || ext == "gif" ||
            ext == "bmp")
        {
            return "image-x-generic";
        }

        // Source code
        if (ext == "cpp" || ext == "hpp" || ext == "c" || ext == "h" || ext == "cc" ||
            ext == "hh" || ext == "js" || ext == "ts" || ext == "py" || ext == "go" ||
            ext == "rs" || ext == "java")
        {
            return "text-x-csrc";
        }

        // Documents
        if (ext == "pdf")
        {
            return "document-pdf";
        }
        if (ext == "txt" || ext == "md" || ext == "log")
        {
            return "text-x-generic";
        }
        if (ext == "doc" || ext == "docx" || ext == "odt")
        {
            return "x-office-document";
        }
        if (ext == "xls" || ext == "xlsx" || ext == "ods")
        {
            return "x-office-spreadsheet";
        }
        if (ext == "ppt" || ext == "pptx" || ext == "odp")
        {
            return "x-office-presentation";
        }

        // Configuration / Scripts
        if (ext == "json" || ext == "yaml" || ext == "yml" || ext == "xml" || ext == "sh" ||
            ext == "bash")
        {
            return "text-x-script";
        }

        // Archives
        if (ext == "zip" || ext == "tar" || ext == "gz" || ext == "bz2" || ext == "xz" ||
            ext == "7z" || ext == "rar")
        {
            return "package-x-generic";
        }

        // Audio
        if (ext == "mp3" || ext == "wav" || ext == "ogg" || ext == "flac" || ext == "m4a")
        {
            return "audio-x-generic";
        }

        // Video
        if (ext == "mp4" || ext == "mkv" || ext == "avi" || ext == "mov" || ext == "webm")
        {
            return "video-x-generic";
        }

        return "text-x-generic";
    }
} // namespace horizon::arkfm
