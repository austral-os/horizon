#pragma once

#include <horizon/Widget.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/Icon.hpp>

namespace horizon::preferences
{
    class PreferencesToolbar : public Widget
    {
    public:
        PreferencesToolbar();
        ~PreferencesToolbar() override = default;

        SearchBox* search_box() const { return m_search_box; }
        GroupButton* navigation() const { return m_navigation; }
        GroupButton* home_button() const { return m_home_button; }
        
    private:
        GroupButton* m_navigation{nullptr};
        GroupButton* m_home_button{nullptr};
        SearchBox* m_search_box{nullptr};
    };
} // namespace horizon::preferences
