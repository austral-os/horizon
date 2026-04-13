#pragma once

#include <horizon/Widget.hpp>
#include <string>
#include <memory>
#include <vector>

namespace horizon
{
    class PreferencesContent;
    class PreferencesToolbar;
    class GraphicsContext;

    /**
     * @brief Individual button in the PreferencesToolbar.
     */
    class PreferencesToolbarButton : public Widget
    {
    public:
        PreferencesToolbarButton(std::string title, std::string icon, int index, PreferencesToolbar *toolbar);
        ~PreferencesToolbarButton() override = default;

        void draw(GraphicsContext &gc) override;

    private:
        std::string m_title;
        std::string m_icon_name;
        int m_index;
        PreferencesToolbar *m_toolbar;
    };

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
