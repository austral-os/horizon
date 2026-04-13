#include "horizon/FontSelector.hpp"
#include "horizon/I18n.hpp"
#include <iomanip>
#include <sstream>
#include <thread>

namespace horizon
{
    FontSelector::FontSelector()
    {
        set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        set_spacing(10);

        auto label = std::make_unique<Label>();
        m_label_ptr = label.get();
        m_label_ptr->set_position_type(FILL);
        m_label_ptr->set_alignment(TextAlignment::Left);
        m_label_ptr->set_vertical_alignment(VerticalAlignment::Middle);

        m_button = std::make_unique<Button<SolidObject>>();
        m_button->set_fixed_size(150);
        m_button->set_text(i18n().tr("core.dialog.font.select"));

        m_button->when_click.connect([this](MouseButtonEventContext &) {
            std::thread([this]() {
                auto dialog = std::make_unique<FontDialog>();
                dialog->set_selection(m_selection);
                
                dialog->when_accepted.connect([this](FontDialogAcceptedContext &ctx) {
                    auto selection = ctx.selection;
                    if (this->application()) {
                        this->application()->post_task([this, selection]() {
                            set_selection(selection);
                            FontDialogAcceptedContext new_ctx;
                            new_ctx.selection = selection;
                            when_font_changed.run(new_ctx);
                        });
                    }
                });

                dialog->show();
            }).detach();
        });

        add_child(std::move(label));
        add_child(std::move(m_button));

        m_selection.family = "Sans";
        m_selection.size = 10.0f;
        update_labels();
    }

    void FontSelector::set_selection(const FontSelection &selection)
    {
        m_selection = selection;
        update_labels();
        invalidate();
    }

    void FontSelector::update_labels()
    {
        std::stringstream ss;
        ss << m_selection.family << " " << (int)m_selection.size << "pt";
        
        if (m_label_ptr)
        {
            m_label_ptr->set_text(ss.str());
        }
    }
} // namespace horizon
