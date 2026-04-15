#pragma once

#include <horizon/VPanel.hpp>

namespace horizon {
namespace pdf {

class PdfTabContent : public horizon::VPanel {
public:
    PdfTabContent() : VPanel() {}
    virtual ~PdfTabContent() = default;

    bool supports_fullscreen() const override { return true; }
};

} // namespace pdf
} // namespace horizon
