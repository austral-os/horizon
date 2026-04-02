#pragma once

#include <horizon/Widget.hpp>
#include <horizon/TreeView.hpp>
#include <horizon/Label.hpp>

namespace horizon::preferences
{
    class MimeTypesView : public Widget
    {
    public:
        MimeTypesView();
        ~MimeTypesView() override = default;

        void load_mime_types();

    private:
        TreeView* m_tree_view{nullptr};
        Label* m_selected_mime_label{nullptr};
    };
} // namespace horizon::preferences
