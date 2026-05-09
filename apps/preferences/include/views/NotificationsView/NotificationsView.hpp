#pragma once
#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/ConfigManager.hpp>
#include <horizon/SolidObject.hpp>

namespace horizon::preferences
{
    class NotificationsView : public Widget
    {
    public:
        NotificationsView();
        ~NotificationsView() override = default;
    private:
        Label* m_title_label{nullptr};
        Checkbox<SolidObject>* m_enable_checkbox{nullptr};
        std::unique_ptr<ConfigManager> m_config;
    };
}
