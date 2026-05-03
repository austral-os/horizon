#pragma once

#include <horizon/WaylandLayerWindow.hpp>
#include <horizon/capture/SelectionWidget.h>

namespace horizon::capture {

class SelectionWindow : public WaylandLayerWindow {
public:
    SelectionWindow();
    ~SelectionWindow();

    SelectionWidget* selection_widget() { return m_selection_widget; }

private:
    SelectionWidget* m_selection_widget;
};

} // namespace horizon::capture
