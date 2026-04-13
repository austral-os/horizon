#pragma once

#include "horizon/Widget.hpp"
#include "horizon/Label.hpp"
#include "horizon/Button.hpp"
#include "horizon/SolidObject.hpp"
#include "horizon/dialogs/FontDialog.hpp"
#include <memory>
#include <string>

namespace horizon
{
    class FontSelector : public Widget
    {
    public:
        FontSelector();
        ~FontSelector() override = default;

        const FontSelection &selection() const { return m_selection; }
        void set_selection(const FontSelection &selection);

        EventsManager<FontDialogAcceptedContext> when_font_changed;

    private:
        void update_labels();

        Label *m_label_ptr{nullptr};
        std::unique_ptr<Button<SolidObject>> m_button;
        FontSelection m_selection;
    };
} // namespace horizon
