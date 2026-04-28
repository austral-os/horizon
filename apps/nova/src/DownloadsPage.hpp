#pragma once
#include "horizon/Widget.hpp"
#include "horizon/TableView.hpp"
#include "horizon/download/DownloadTask.hpp"
#include <memory>

namespace horizon {
namespace nova {

class DownloadsPage : public horizon::Widget {
public:
    DownloadsPage();
    virtual ~DownloadsPage() = default;

    std::string get_url() const { return "nova://downloads"; }
    std::string get_title() const { return "Descargas"; }

protected:
    void calculate_layout() override;

private:
    void setup_ui();
    void refresh_list();

    horizon::TableView<std::shared_ptr<download::DownloadTask>>* m_table = nullptr;
};

} // namespace nova
} // namespace horizon
