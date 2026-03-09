#include <cstdio>
#include <filesystem>
#include <fstream>
#include <horizon/arkutils/FileOperations.hpp>

namespace fs = std::filesystem;

namespace horizon::arkutils
{
    std::future<FileOperations::Result> FileOperations::copy(const std::string &src,
                                                             const std::string &dest,
                                                             ProgressCallback on_progress)
    {
        return std::async(std::launch::async,
                          [src, dest, on_progress]()
                          {
                              try
                              {
                                  if (!fs::exists(src))
                                      return Result::NotFound;

                                  // For a simple non-blocking copy of a large file, we could do
                                  // block by block to report progress. For now, let's use
                                  // std::filesystem::copy.
                                  fs::copy(src, dest,
                                           fs::copy_options::recursive |
                                               fs::copy_options::overwrite_existing);

                                  if (on_progress)
                                      on_progress(1.0);
                                  return Result::Success;
                              }
                              catch (const fs::filesystem_error &e)
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
