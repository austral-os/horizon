#pragma once

#include "horizon/Widget.hpp"
#include "horizon/Label.hpp"
#include "horizon/Button.hpp"
#include "horizon/SolidObject.hpp"
#include "horizon/dialogs/FileDialog.hpp"
#include <memory>
#include <string>

namespace horizon
{
    class FileSelector : public Widget
    {
    public:
        FileSelector(FileDialogMode mode = FileDialogMode::Open);
        ~FileSelector() override = default;

        const std::string &path() const { return m_path; }
        void set_path(const std::string &path);

        FileDialogMode mode() const { return m_mode; }
        void set_mode(FileDialogMode mode) { m_mode = mode; }

        EventsManager<FileDialogAcceptedContext> when_path_changed;

    private:
        void update_labels();

        Label *m_label_ptr{nullptr};
        std::unique_ptr<Button<SolidObject>> m_button;
        std::string m_path;
        FileDialogMode m_mode;
    };
} // namespace horizon
