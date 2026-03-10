#include "ArkfmView.hpp"
#include "ArkfmListView.hpp"
#include <memory>

namespace horizon::arkfm
{

    ArkfmView::ArkfmView(std::string path) : Widget(), m_current_path(std::move(path))
    {
        set_view_mode(ViewMode::List);
    }

    void ArkfmView::set_view_mode(ViewMode vm)
    {
        m_view_mode = vm;
        clear_children();

        if (m_view_mode == ViewMode::List)
        {
            auto view_mode_list = std::make_unique<ArkfmListView>(m_current_path);

            add_child(std::move(view_mode_list));
        }
    }

} // namespace horizon::arkfm