#pragma once

#include "horizon/Widget.hpp"
#include "horizon/Label.hpp"
#include "horizon/ProgressBar.hpp"
#include "horizon/Button.hpp"
#include "horizon/Icon.hpp"
#include "horizon/SolidObject.hpp"
#include "DownloadTask.hpp"

namespace horizon {
namespace download {

class DownloadItemWidget : public horizon::Widget {
public:
    DownloadItemWidget(std::shared_ptr<DownloadTask> task);
    virtual ~DownloadItemWidget();

protected:
    void calculate_layout() override;

private:
    void update_ui();
    std::string format_size(size_t bytes);

    std::shared_ptr<DownloadTask> m_task;

    horizon::Label* m_name_label = nullptr;
    horizon::Label* m_status_label = nullptr;
    horizon::ProgressBar* m_progress_bar = nullptr;
    horizon::Button<horizon::SolidObject>* m_action_button = nullptr; // Pause/Resume
    horizon::Button<horizon::SolidObject>* m_cancel_button = nullptr;
    horizon::Icon* m_action_icon = nullptr;

    size_t m_progress_conn = 0;
    size_t m_state_conn = 0;
};

} // namespace download
} // namespace horizon
