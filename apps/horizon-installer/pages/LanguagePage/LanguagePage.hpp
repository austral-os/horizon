#pragma once
#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/TreeView.hpp>
#include <string>
#include <map>

namespace horizon::installer
{
    /**
     * @brief Page 0: Language Selection
     */
    class LanguagePage : public Widget
    {
    public:
        LanguagePage();
        ~LanguagePage() override = default;

        void load_languages();
        std::string selected_language_code() const { return selected_code; }
        std::string selected_language_name() const { return selected_name; }

        EventsManager<EventContext> when_continue;

    private:
        TreeView *m_tree{nullptr};
        std::string selected_code;
        std::string selected_name;
        std::map<std::string, std::string> m_name_to_code;
    };
} // namespace horizon::installer
