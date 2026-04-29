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
    virtual ~DownloadView() = default;

    void refresh();

protected:
    void calculate_layout() override;

private:
    void setup_ui();
    horizon::TableView<std::shared_ptr<DownloadTask>>* m_table = nullptr;
};

} // namespace download
} // namespace horizon
