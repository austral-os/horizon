#pragma once

#include <horizon/WaylandWindow.hpp>
#include <string>

namespace horizon
{
    class Label;
    class ProgressBar;

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

        private:
            void setup_ui(const std::string &initial_status);

            Label *m_status_label{nullptr};
            ProgressBar *m_progress_bar{nullptr};
        };
    }
}
