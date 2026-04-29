#pragma once
#include "horizon/Widget.hpp"
#include <memory>

namespace horizon {
namespace download {
    class DownloadView;
}

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
    
    download::DownloadView* m_view = nullptr;
};

} // namespace nova
} // namespace horizon
