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
        FileSave = 1 << 3,
        FileSaveAs = 1 << 4,
        FileAll = FileOpen | FileOpenFolder | FileClose | FileSave | FileSaveAs
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
        virtual std::string current_file_path() const { return ""; }

        bool draw_background() const { return m_draw_background; }
        void set_draw_background(bool draw) { m_draw_background = draw; invalidate(); }

        SignalManager signals;

        struct FileOpenedContext : public EventContext
        {
            std::string path;
        };

        struct FileSaveContext : public EventContext
        {
            std::string path;
        };

        EventsManager<FileOpenedContext> when_file_opened;
        EventsManager<FileOpenedContext> when_folder_opened;
        EventsManager<EventContext> when_file_close;
        EventsManager<FileSaveContext> when_save;
        EventsManager<FileSaveContext> when_save_as;

    protected:
        explicit Window(std::unique_ptr<Titlebar> custom_titlebar);

        void draw(GraphicsContext &gc) override;
        Titlebar *m_titlebar;
        bool m_draw_background = true;
    };

} // namespace horizon