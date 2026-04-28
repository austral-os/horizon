#pragma once

#include "horizon/Widget.hpp"
#include "horizon/Label.hpp"
#include "horizon/ScrollArea.hpp"
#include "DownloadManager.hpp"

namespace horizon {
namespace download {

class DownloadPopover : public horizon::Widget {
public:
    DownloadPopover(horizon::Widget* parent);
    virtual ~DownloadPopover() = default;

    void refresh();
    void show_relative_to(horizon::Widget* target);

protected:
    void draw(GraphicsContext& gc) override;

private:
    horizon::Widget* m_container = nullptr;
    horizon::Label* m_empty_label = nullptr;
};

} // namespace download
} // namespace horizon
