#pragma once

#include "horizon/WaylandWindow.hpp"
#include "horizon/EventsManager.hpp"
#include "horizon/Color.hpp"
#include <string>
#include <memory>

namespace horizon
{
    class ColorPickerDialogAcceptedContext : public EventContext
    {
    public:
        Color color;
    };

    class ColorPickerDialogCancelledContext : public EventContext
    {
    public:
    };

    class ColorPickerDialog : public WaylandWindow
    {
    public:
        ColorPickerDialog(const std::string &title = "Seleccionar Color");
        ~ColorPickerDialog() override;

        Color color() const;
        void set_color(const Color &color);

        EventsManager<ColorPickerDialogAcceptedContext> when_accepted;
        EventsManager<ColorPickerDialogCancelledContext> when_cancelled;
        
        void show() { WaylandWindow::run(); }

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace horizon
