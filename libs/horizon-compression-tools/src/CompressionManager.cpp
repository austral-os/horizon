#include "horizon/compression/CompressionManager.hpp"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <cstdio>
#include <memory>
#include <array>
#include <set>

namespace fs = std::filesystem;

namespace horizon::compression
{
    // Helper to execute command and get output
    static std::vector<std::string> exec_list(const std::string& cmd) {
        std::vector<std::string> result;
        std::array<char, 256> buffer;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe) return result;
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            std::string line = buffer.data();
            if (!line.empty() && line.back() == '\n') line.pop_back();
            if (!line.empty()) result.push_back(line);
        }
        return result;
    }

    // Helper to execute command and get exit code + output
    static std::pair<int, std::string> exec_with_output(const std::string& cmd) {
        std::string result;
        std::array<char, 256> buffer;
        // Redirect stderr to stdout to capture error messages
        std::string full_cmd = cmd + " 2>&1";
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(full_cmd.c_str(), "r"), pclose);
        if (!pipe) return {-1, "No se pudo iniciar el proceso."};
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        // pclose returns the exit status of the command
        int status = pclose(pipe.release());
        return {WEXITSTATUS(status), result};
    }

    class GenericCompressionTask : public CompressionTask
    {
    public:
        enum class Type { Extract, Compress };
        
        GenericCompressionTask(Type t, std::vector<std::string> src_list, std::string dst, ArchiveFormat fmt = ArchiveFormat::Zip)
            : m_type(t), m_src_list(std::move(src_list)), m_dst(dst), m_format(fmt) {}

    protected:
        void run() override {
            if (m_type == Type::Extract) {
                do_extract();
            } else {
                do_compress();
            }
        }

    private:
        void do_extract() {
            try {
                if (m_src_list.empty()) return;
                std::string src = m_src_list[0];

                // 1. Analyze for smart extraction
                std::string list_cmd;
                if (src.find(".zip") != std::string::npos) list_cmd = "zipinfo -1 \"" + src + "\"";
                else if (src.find(".tar") != std::string::npos) list_cmd = "tar -tf \"" + src + "\"";
                else if (src.find(".7z") != std::string::npos || src.find(".rar") != std::string::npos) list_cmd = "7z l -ba -slt \"" + src + "\" | grep '^Path =' | cut -d' ' -f3-";
                
                std::vector<std::string> files;
                if (!list_cmd.empty()) {
                    files = exec_list(list_cmd);
                } else if (src.find(".gz") != std::string::npos && src.find(".tar") == std::string::npos) {
                    files.push_back(fs::path(src).stem().string());
                }

                if (files.empty()) {
                    report_finished(false, "No se pudo leer el contenido del archivo o el archivo está vacío.");
                    return;
                }

                // Find root entries
                std::set<std::string> root_entries;
                for (const auto& f : files) {
                    fs::path p(f);
                    auto it = p.begin();
                    if (it != p.end()) {
                        root_entries.insert(it->string());
                    }
                }

                bool single_folder = (root_entries.size() == 1);
                
                std::string final_dest = m_dst;
                if (!single_folder) {
                    fs::path archive_name = fs::path(src).stem();
                    if (archive_name.extension() == ".tar") archive_name = archive_name.stem();
                    
                    final_dest = (fs::path(m_dst) / archive_name).string();
                    fs::create_directories(final_dest);
                }

                report_progress(0.1, "", "Extrayendo...");

                std::string extract_cmd;
                if (src.find(".zip") != std::string::npos) extract_cmd = "unzip -o \"" + src + "\" -d \"" + final_dest + "\"";
                else if (src.find(".tar") != std::string::npos) extract_cmd = "tar -xf \"" + src + "\" -C \"" + final_dest + "\"";
                else if (src.find(".7z") != std::string::npos || src.find(".rar") != std::string::npos) extract_cmd = "7z x \"" + src + "\" -o\"" + final_dest + "\" -y";
                else if (src.find(".gz") != std::string::npos) {
                    std::string out_file = (fs::path(final_dest) / fs::path(src).stem()).string();
                    extract_cmd = "gzip -dc \"" + src + "\" > \"" + out_file + "\"";
                }

                auto [res, output] = exec_with_output(extract_cmd);
                if (res == 0) {
                    report_progress(1.0, "", "Completado");
                    report_finished(true, "", final_dest);
                } else {
                    report_finished(false, output.empty() ? "Error desconocido al extraer." : output);
                }
            } catch (const std::exception& e) {
                report_finished(false, e.what());
            }
        }

        void do_compress() {
            if (m_src_list.empty()) return;

            report_progress(0.1, "", "Comprimiendo...");
            
            // To avoid storing absolute paths, we CD into the parent directory of the first item
            fs::path first_path(m_src_list[0]);
            fs::path base_dir = first_path.parent_path();
            fs::path abs_output = fs::absolute(m_dst);

            std::string relative_sources;
            for (const auto& s : m_src_list) {
                fs::path p(s);
                relative_sources += "\"" + p.filename().string() + "\" ";
            }

            std::string cmd;
            std::string cd_cmd = "cd \"" + base_dir.string() + "\" && ";

            if (m_format == ArchiveFormat::Zip) cmd = cd_cmd + "zip -r \"" + abs_output.string() + "\" " + relative_sources;
            else if (m_format == ArchiveFormat::TarGz) cmd = cd_cmd + "tar -czf \"" + abs_output.string() + "\" " + relative_sources;
            else if (m_format == ArchiveFormat::TarXz) cmd = cd_cmd + "tar -cJf \"" + abs_output.string() + "\" " + relative_sources;
            else if (m_format == ArchiveFormat::SevenZip) cmd = cd_cmd + "7z a \"" + abs_output.string() + "\" " + relative_sources;
            else if (m_format == ArchiveFormat::Gz) cmd = cd_cmd + "gzip -c " + relative_sources + " > \"" + abs_output.string() + "\"";

            auto [res, output] = exec_with_output(cmd);
            if (res == 0) {
                report_progress(1.0, "", "Completado");
                report_finished(true, "", m_dst);
            } else {
                report_finished(false, output.empty() ? "Error desconocido al comprimir." : output);
            }
        }

        Type m_type;
        std::vector<std::string> m_src_list;
        std::string m_dst;
        ArchiveFormat m_format;
    };

    std::shared_ptr<CompressionTask> CompressionManager::extract_smart(const std::string& archive_path, const std::string& destination_dir)
    {
        return std::make_shared<GenericCompressionTask>(GenericCompressionTask::Type::Extract, std::vector<std::string>{archive_path}, destination_dir);
    }

    std::shared_ptr<CompressionTask> CompressionManager::compress(const std::vector<std::string>& sources, const std::string& output_path, ArchiveFormat format)
    {
        return std::make_shared<GenericCompressionTask>(GenericCompressionTask::Type::Compress, sources, output_path, format);
    }

    bool CompressionManager::is_supported_archive(const std::string& path)
    {
        std::string p = path;
        std::transform(p.begin(), p.end(), p.begin(), ::tolower);
        return (p.find(".zip") != std::string::npos || 
                p.find(".tar") != std::string::npos || 
                p.find(".7z") != std::string::npos || 
                p.find(".rar") != std::string::npos ||
                p.find(".gz") != std::string::npos);
    }

    ArchiveFormat CompressionManager::format_from_extension(const std::string& path)
    {
        std::string p = path;
        std::transform(p.begin(), p.end(), p.begin(), ::tolower);
        if (p.find(".zip") != std::string::npos) return ArchiveFormat::Zip;
        if (p.find(".tar.gz") != std::string::npos) return ArchiveFormat::TarGz;
        if (p.find(".tar.xz") != std::string::npos) return ArchiveFormat::TarXz;
        if (p.find(".7z") != std::string::npos) return ArchiveFormat::SevenZip;
        if (p.find(".gz") != std::string::npos) return ArchiveFormat::Gz;
        return ArchiveFormat::Zip;
    }

} // namespace horizon::compression
