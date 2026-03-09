#pragma once

#include <functional>
#include <future>
#include <string>
#include <vector>

namespace horizon::arkutils
{
    /**
     * @class FileOperations
     * @brief Singleton or static utility for non-blocking filesystem operations.
     */
    class FileOperations
    {
    public:
        enum class Result
        {
            Success,
            Error,
            AccessDenied,
            NotFound,
            AlreadyExists
        };

        using ProgressCallback = std::function<void(double progress)>;
        using CompletionCallback = std::function<void(Result result)>;

        static std::future<Result> copy(const std::string &src, const std::string &dest,
                                        ProgressCallback on_progress = nullptr);
        static std::future<Result> move(const std::string &src, const std::string &dest);
        static std::future<Result> remove(const std::string &path);
        static std::future<Result> rename(const std::string &old_path, const std::string &new_path);
        static std::future<Result> create_directory(const std::string &path);
    };
} // namespace horizon::arkutils
