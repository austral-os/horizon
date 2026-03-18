#pragma once

#include <horizon/Widget.hpp>
#include <horizon/GroupButton.hpp>
#include <horizon/SearchBox.hpp>
#include <horizon/Icon.hpp>
#include <memory>

namespace horizon::preferences
{
    class PreferencesToolbar : public Widget
    {
    public:
        PreferencesToolbar();
        ~PreferencesToolbar() override = default;

        SearchBox* search_box() const { return m_search_box; }
        
    private:
        GroupButton* m_navigation{nullptr};
        SearchBox* m_search_box{nullptr};
    };
} // namespace horizon::preferences
