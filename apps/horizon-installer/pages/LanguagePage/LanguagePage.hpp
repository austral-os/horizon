#pragma once
#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/TableView.hpp>
#include <string>
#include <vector>

namespace horizon::installer
{
    struct LanguageItem
    {
        std::string code;
        std::string name;
    };

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
        TableView<LanguageItem> *m_table{nullptr};
        std::string selected_code;
        std::string selected_name;
        std::vector<LanguageItem> m_languages;
    };
} // namespace horizon::installer
