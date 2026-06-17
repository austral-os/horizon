#include <cstdio>
#include <filesystem>
#include <fstream>
#include <horizon/arkutils/FileOperations.hpp>

namespace fs = std::filesystem;

namespace horizon::arkutils
{
    static uintmax_t calculate_total_size(const fs::path &path, const fs::path &exclude_path)
    {
        try
        {
            if (!fs::exists(path)) return 0;
            if (!fs::is_directory(path))
                return fs::file_size(path);
            uintmax_t size = 0;
            if (fs::is_directory(path))
            {
                auto it = fs::recursive_directory_iterator(path);
                for (; it != fs::recursive_directory_iterator(); ++it)
                {
                    if (it->path() == exclude_path)
                    {
                        it.disable_recursion_pending();
                        continue;
                    }
                    if (fs::is_regular_file(it->status()))
                        size += fs::file_size(it->path());
                }
            }
            return size;
        }
        catch (...)
        {
            return 0;
        }
    }

    static void copy_recursive(const fs::path &src, const fs::path &dest, const fs::path &root_dest,
                               uintmax_t &current_size, uintmax_t total_size,
                               FileOperations::ProgressCallback on_progress)
    {
        if (fs::is_directory(src))
        {
            fs::create_directories(dest);
            for (const auto &entry : fs::directory_iterator(src))
            {
                if (entry.path() == root_dest) continue;
                copy_recursive(entry.path(), dest / entry.path().filename(), root_dest, current_size,
                               total_size, on_progress);
            }
        }
        else
        {
            std::ifstream source(src, std::ios::binary);
            std::ofstream destination(dest, std::ios::binary);
            char buffer[65536];
            while (source.read(buffer, sizeof(buffer)))
            {
                destination.write(buffer, source.gcount());
                current_size += source.gcount();
                if (on_progress && total_size > 0)
                    on_progress(static_cast<double>(current_size) / total_size);
            }
            destination.write(buffer, source.gcount());
            current_size += source.gcount();
            if (on_progress && total_size > 0)
                on_progress(static_cast<double>(current_size) / total_size);
        }
    }

    std::future<FileOperations::Result> FileOperations::copy(const std::string &src,
                                                             const std::string &dest,
                                                             ProgressCallback on_progress)
    {
        return std::async(std::launch::async,
                          [src, dest, on_progress]()
                          {
                              try
                              {
                                  fs::path s(src);
                                  fs::path d(dest);

                                  if (!fs::exists(s))
                                      return Result::NotFound;

                                  // If dest is a directory, append src filename
                                  if (fs::exists(d) && fs::is_directory(d))
                                  {
                                      d /= s.filename();
                                  }

                                  uintmax_t total_size = calculate_total_size(s, d);
                                  uintmax_t current_size = 0;

                                  copy_recursive(s, d, d, current_size, total_size, on_progress);

                                  if (on_progress)
                                      on_progress(1.0);
                                  return Result::Success;
                              }
                              catch (const fs::filesystem_error &e)
                              {
                                  return Result::Error;
                              }
                              catch (...)
                              {
                                  return Result::Error;
                              }
                          });
    }


    std::future<FileOperations::Result> FileOperations::move(const std::string &src,
                                                             const std::string &dest)
    {
        return std::async(std::launch::async,
                          [src, dest]()
                          {
                              try
                              {
                                  fs::rename(src, dest);
                                  return Result::Success;
                              }
                              catch (const fs::filesystem_error &e)
                              {
                                  return Result::Error;
                              }
                          });
    }

    std::future<FileOperations::Result> FileOperations::remove(const std::string &path)
    {
        return std::async(std::launch::async,
                          [path]()
                          {
                              try
                              {
                                  fs::remove_all(path);
                                  return Result::Success;
                              }
                              catch (const fs::filesystem_error &e)
                              {
                                  return Result::Error;
                              }
                          });
    }

    std::future<FileOperations::Result> FileOperations::rename(const std::string &old_path,
                                                               const std::string &new_path)
    {
        return move(old_path, new_path);
    }

    std::future<FileOperations::Result> FileOperations::create_directory(const std::string &path)
    {
        return std::async(std::launch::async,
                          [path]()
                          {
                              try
                              {
                                  if (fs::create_directories(path))
                                      return Result::Success;
                                  return Result::AlreadyExists;
                              }
                              catch (const fs::filesystem_error &e)
                              {
                                  return Result::Error;
                              }
                          });
    }
} // namespace horizon::arkutils
