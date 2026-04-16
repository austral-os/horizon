#pragma once

#include <horizon/Widget.hpp>
#include <horizon/ToolbarButton.hpp>
#include <string>
#include <memory>
#include <vector>

namespace horizon
{
    class PreferencesContent;
    class GraphicsContext;

    /**
     * @brief A toolbar widget for selecting sections in a PreferencesContent.
     */
    class PreferencesToolbar : public Widget
    {
    public:
        PreferencesToolbar(PreferencesContent *content);
        ~PreferencesToolbar() override = default;

        PreferencesContent *content() const { return m_content; }
        int active_index() const { return m_active_index; }
        void set_active_index(int index);

    private:
        PreferencesContent *m_content;
        int m_active_index{0};
    };
} // namespace horizon
