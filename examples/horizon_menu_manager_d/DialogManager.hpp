#pragma once
#include "Dialog.hpp"
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace horizon
{
    class DialogManager
    {
    public:
        void add_dialog(std::unique_ptr<Dialog> dialog)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_dialogs[dialog->id()] = std::move(dialog);
        }

        Dialog *get_dialog(const std::string &id)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_dialogs.find(id);
            if (it != m_dialogs.end())
            {
                return it->second.get();
            }
            return nullptr;
        }

        void remove_dialog(const std::string &id)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_dialogs.erase(id);
        }

    private:
        std::map<std::string, std::unique_ptr<Dialog>> m_dialogs;
        std::mutex m_mutex;
    };
} // namespace horizon
