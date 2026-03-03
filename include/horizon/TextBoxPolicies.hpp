#pragma once
#include <cctype>
#include <regex>
#include <string>

namespace horizon
{
    struct TextBoxConfig
    {
        size_t min_length = 0;
        size_t max_length = 1024;
        std::string format = ""; // e.g., "**.**" for double, "**/**/****" for date
        long long min_int = -2147483648LL;
        long long max_int = 2147483647LL;
        double min_double = -1e308;
        double max_double = 1e308;
    };

    struct TextPolicy
    {
        static bool validate(const std::string &text, const TextBoxConfig &config)
        {
            return text.length() >= config.min_length && text.length() <= config.max_length;
        }

        static std::string get_display_text(const std::string &text, const TextBoxConfig &)
        {
            return text;
        }
    };

    struct PasswordPolicy
    {
        static bool validate(const std::string &text, const TextBoxConfig &config)
        {
            return text.length() >= config.min_length && text.length() <= config.max_length;
        }

        static std::string get_display_text(const std::string &text, const TextBoxConfig &)
        {
            return std::string(text.length(), '*');
        }
    };

    struct IntegerPolicy
    {
        static bool validate(const std::string &text, const TextBoxConfig &config)
        {
            if (text.empty())
                return config.min_length == 0;
            try
            {
                size_t pos;
                long long val = std::stoll(text, &pos);
                if (pos != text.length())
                    return false;
                return val >= config.min_int && val <= config.max_int &&
                       text.length() >= config.min_length && text.length() <= config.max_length;
            }
            catch (...)
            {
                return false;
            }
        }

        static std::string get_display_text(const std::string &text, const TextBoxConfig &)
        {
            return text;
        }
    };

    struct DoublePolicy
    {
        static bool validate(const std::string &text, const TextBoxConfig &config)
        {
            if (text.empty())
                return config.min_length == 0;
            try
            {
                size_t pos;
                double val = std::stod(text, &pos);
                if (pos != text.length())
                    return false;
                return val >= config.min_double && val <= config.max_double &&
                       text.length() >= config.min_length && text.length() <= config.max_length;
            }
            catch (...)
            {
                return false;
            }
        }

        static std::string get_display_text(const std::string &text, const TextBoxConfig &)
        {
            return text;
        }
    };

    struct EmailPolicy
    {
        static bool validate(const std::string &text, const TextBoxConfig &config)
        {
            if (text.empty())
                return config.min_length == 0;
            const std::regex pattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
            return std::regex_match(text, pattern) && text.length() >= config.min_length &&
                   text.length() <= config.max_length;
        }

        static std::string get_display_text(const std::string &text, const TextBoxConfig &)
        {
            return text;
        }
    };

    struct DatePolicy
    {
        static bool validate(const std::string &text, const TextBoxConfig &config)
        {
            if (text.empty())
                return config.min_length == 0;
            // Basic check against format string if provided, assume DD/MM/YYYY if format is empty
            std::string fmt = config.format.empty() ? "**/**/****" : config.format;
            if (text.length() != fmt.length())
                return false;

            for (size_t i = 0; i < fmt.length(); ++i)
            {
                if (fmt[i] == '*')
                {
                    if (!std::isdigit(text[i]))
                        return false;
                }
                else if (text[i] != fmt[i])
                {
                    return false;
                }
            }
            return true;
        }

        static std::string get_display_text(const std::string &text, const TextBoxConfig &)
        {
            return text;
        }
    };
} // namespace horizon
