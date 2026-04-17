#pragma once

#include <horizon/WaylandWindow.hpp>
#include <string>

namespace horizon
{
    class Label;
    class LoadingBar;

    namespace disks
    {
        /**
         * @brief A dialog that shows the progress of a disk operation.
         */
        class DiskProgressDialog : public WaylandWindow
        {
        public:
            DiskProgressDialog(const std::string &title, const std::string &initial_status);
            ~DiskProgressDialog() override = default;

            /**
             * @brief Set the status text.
             */
            void set_status(const std::string &status);

            /**
             * @brief Set the progress percentage (0.0 to 1.0).
             */
            void set_progress(float percent, const std::string& operation = "");

        private:
            void setup_ui(const std::string &initial_status);

            std::string m_base_status;
            Label *m_status_label{nullptr};
            LoadingBar *m_loading_bar{nullptr};
        };
    }
}
