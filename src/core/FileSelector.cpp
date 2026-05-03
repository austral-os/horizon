#include "horizon/FileSelector.hpp"
#include "horizon/Application.hpp"
#include "horizon/I18n.hpp"
#include <thread>
#include <filesystem>

namespace horizon
{
    FileSelector::FileSelector(FileDialogMode mode) : m_mode(mode)
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(10);

        auto label = std::make_unique<Label>();
        m_label_ptr = label.get();
        m_label_ptr->set_position_type(FILL);
        m_label_ptr->set_alignment(TextAlignment::Left);
        m_label_ptr->set_vertical_alignment(VerticalAlignment::Middle);
        m_label_ptr->set_text(i18n().tr("core.dialog.file.no_selection"));

        m_button = std::make_unique<Button<SolidObject>>();
        m_button->set_fixed_size(150);
        m_button->set_text(i18n().tr("core.dialog.file.browse"));

        m_button->when_click.connect([this](MouseButtonEventContext &) {
            std::thread([this]() {
                auto dialog = std::make_unique<FileDialog>(m_mode, i18n().tr("core.dialog.file.select_title"));
                if (!m_path.empty()) {
                    dialog->set_current_path(m_path);
                }
                
                dialog->when_accepted.connect([this](FileDialogAcceptedContext &ctx) {
                    auto path = ctx.selected_path;
                    if (this->application()) {
                        this->application()->post_task([this, path]() {
                            set_path(path);
                            FileDialogAcceptedContext new_ctx;
                            new_ctx.selected_path = path;
                            when_path_changed.run(new_ctx);
                        });
                    }
                });

                dialog->run();
            }).detach();
        });

        add_child(std::move(label));
        add_child(std::move(m_button));
    }

    void FileSelector::set_path(const std::string &path)
    {
        m_path = path;
        update_labels();
        invalidate();
    }

    void FileSelector::update_labels()
    {
        if (m_label_ptr)
        {
            if (m_path.empty()) {
                m_label_ptr->set_text(i18n().tr("core.dialog.file.no_selection"));
            } else {
                std::filesystem::path p(m_path);
                m_label_ptr->set_text(p.string());
            }
        }
    }
} // namespace horizon
