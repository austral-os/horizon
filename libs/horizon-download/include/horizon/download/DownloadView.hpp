#pragma once
#include "horizon/Widget.hpp"
#include "horizon/TableView.hpp"
#include "DownloadTask.hpp"
#include <memory>

namespace horizon {
namespace download {

class DownloadView : public horizon::Widget {
public:
    DownloadView();
    ~DownloadView() override;

    void refresh();

protected:
    void calculate_layout() override;

private:
    void setup_ui();
    horizon::TableView<std::shared_ptr<DownloadTask>>* m_table = nullptr;

    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
    size_t m_conn_added = 0;
    size_t m_conn_removed = 0;
    std::vector<std::pair<std::shared_ptr<DownloadTask>, size_t>> m_task_conns;
};

} // namespace download
} // namespace horizon
