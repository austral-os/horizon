#pragma once

#include <string>
#include <vector>

namespace horizon::files
{
    class FileHistory
    {
    public:
        void push(const std::string &path)
        {
            if (!m_history.empty() && m_history.back() == path)
            {
                return;
            }

            m_history.push_back(path);
            m_forward_stack.clear();
        }

        std::string back()
        {
            if (!can_back())
            {
                return "";
            }

            m_forward_stack.push_back(m_history.back());
            m_history.pop_back();
            return m_history.back();
        }

        std::string forward()
        {
            if (!can_forward())
            {
                return "";
            }

            std::string path = m_forward_stack.back();
            m_forward_stack.pop_back();
            m_history.push_back(path);
            return path;
        }

        bool can_back() const
        {
            return m_history.size() > 1;
        }

        bool can_forward() const
        {
            return !m_forward_stack.empty();
        }

        const std::string &current_path() const
        {
            static const std::string empty = "";
            return m_history.empty() ? empty : m_history.back();
        }

    private:
        std::vector<std::string> m_history;
        std::vector<std::string> m_forward_stack;
    };
} // namespace horizon::files
