#pragma once

#include "Widget.hpp"
#include "horizon/Titlebar.hpp"
#include <horizon/GraphicsContext.hpp>
#include <horizon/SignalManager.hpp>
#include <string>

namespace horizon
{

    enum FileCapability : uint32_t
    {
        FileNone = 0,
        FileOpen = 1 << 0,
        FileOpenFolder = 1 << 1,
        FileClose = 1 << 2,
        FileAll = FileOpen | FileOpenFolder | FileClose
    };

    class Window : public Widget
    {
    public:
        explicit Window(std::string title);

        void set_size(int width, int height);

        const std::string &title() const;
        void set_title(std::string title);

        Titlebar *titlebar() const { return m_titlebar; }

        virtual CornerRadius get_window_corners() const;

        void render(GraphicsContext &gc, int cx, int cy, int cw, int ch,
                    bool force = false) override;

        virtual uint32_t file_capabilities() const { return FileNone; }

        SignalManager signals;

        struct FileOpenedContext : public EventContext
        {
            std::string path;
        };

        EventsManager<FileOpenedContext> when_file_opened;
        EventsManager<FileOpenedContext> when_folder_opened;
        EventsManager<EventContext> when_file_close;

    protected:
        explicit Window(std::unique_ptr<Titlebar> custom_titlebar);

        void draw(GraphicsContext &gc) override;
        Titlebar *m_titlebar;
    };

} // namespace horizon