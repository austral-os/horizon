#pragma once

#include <horizon/Widget.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Label.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/TreeView.hpp>
#include <horizon/Image.hpp>
#include <horizon/ScrollArea.hpp>
#include <horizon/Textarea.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon-installer-utils/InstallerManager.hpp>
#include <vector>
#include <string>

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

    /**
     * @brief Page 1: Welcome / Presentation
     */
    class WelcomePage : public Widget
    {
    public:
        WelcomePage();
        ~WelcomePage() override = default;

        EventsManager<EventContext> when_continue;
    };

    /**
     * @brief Page 2: License Agreement
     */
    class LicensePage : public Widget
    {
    public:
        LicensePage();
        ~LicensePage() override = default;

        EventsManager<EventContext> when_agree;
        EventsManager<EventContext> when_disagree;
    };

    /**
     * @brief Page 3: Disk Selection
     */
    class DiskPage : public Widget
    {
    public:
        DiskPage();
        ~DiskPage() override = default;

        std::string selected_device_str;
        
        EventsManager<EventContext> when_install;
        EventsManager<EventContext> when_back;

    private:
        TreeView* m_disk_tree{nullptr};
        void refresh_disks();
    };

    /**
     * @brief Page 4: Installation Progress
     */
    class InstallPage : public Widget
    {
    public:
        InstallPage();
        ~InstallPage() override = default;

        void update_progress(float progress, const std::string& message);
        
        EventsManager<EventContext> when_cancel;

    private:
        ProgressBar* m_progress{nullptr};
        Label* m_status{nullptr};
    };

    /**
     * @brief Final success page.
     */
    class SuccessPage : public Widget
    {
    public:
        SuccessPage();
        EventsManager<EventContext> when_finish;
    };

} // namespace horizon::installer
