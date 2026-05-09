#pragma once

#include <horizon/FileWatcher.hpp>
#include <atomic>
#include <string>

namespace horizon
{
    /**
     * @class MouseSettings
     * @brief Singleton that manages mouse settings and watches for configuration changes.
     */
    class MouseSettings : public FileWatcher
    {
    public:
        /**
         * @brief Get the singleton instance.
         */
        static MouseSettings &instance();

        /**
         * @brief Get the current double click speed in milliseconds.
         */
        int double_click_speed() const;

    protected:
        /**
         * @brief Called when mouse.json changes.
         */
        void on_file_changed() override;

        /**
         * @brief Implementation of FileWatcher hook to execute tasks.
         * Since we only update atomic values, we can execute directly.
         */
        void post_watcher_task(std::function<void()> task) override;

    private:
        MouseSettings();
        virtual ~MouseSettings();

        void load();

        std::atomic<int> m_double_click_speed{250};
        std::string m_config_path;
    };
} // namespace horizon
