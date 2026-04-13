#pragma once

#include "horizon/WaylandWindow.hpp"
#include "horizon/EventsManager.hpp"
#include <string>
#include <vector>
#include <memory>

namespace horizon
{
    struct FontSelection
    {
        std::string family;
        std::string style;
        float size;
        std::string features;
    };

    class FontDialogAcceptedContext : public EventContext
    {
    public:
        FontSelection selection;
    };

    class FontDialogCancelledContext : public EventContext
    {
    public:
    };

    class FontDialog : public WaylandWindow
    {
    public:
        FontDialog(const std::string &title = "Seleccionar tipo de letra");
        ~FontDialog() override;

        FontSelection selection() const;
        void set_selection(const FontSelection &sel);

        EventsManager<FontDialogAcceptedContext> when_accepted;
        EventsManager<FontDialogCancelledContext> when_cancelled;
        
        void show() { WaylandWindow::run(); }

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace horizon
