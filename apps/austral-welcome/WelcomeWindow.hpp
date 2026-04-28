#pragma once

#include <horizon/Window.hpp>
#include <horizon/Widget.hpp>
#include <horizon/ConfigManager.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/AquaObject.hpp>
#include <memory>

namespace horizon
{
    class WelcomeWindow : public Window
    {
    public:
        WelcomeWindow();

    private:
        void setup_ui();
        void load_config();
        void save_config();

        std::unique_ptr<ConfigManager> m_config;
        bool m_show_welcome{true};
        Checkbox<AquaObject> *m_start_on_boot_check{nullptr};
    };
} // namespace horizon
