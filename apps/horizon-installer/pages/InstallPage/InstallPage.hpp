#pragma once
#include <horizon/Widget.hpp>
#include <horizon/EventsManager.hpp>
#include <horizon/ProgressBar.hpp>
#include <horizon/Label.hpp>
#include <string>

namespace horizon::installer
{
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
} // namespace horizon::installer
