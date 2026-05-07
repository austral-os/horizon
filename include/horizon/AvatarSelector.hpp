#pragma once

#include <horizon/Widget.hpp>
#include <horizon/Vault.hpp>
#include <horizon/IconView.hpp>
#include <string>

namespace horizon
{
    class AvatarSelector : public Widget
    {
    public:
        AvatarSelector();
        ~AvatarSelector() override = default;

        std::string selected_avatar() const;
        void set_selected_avatar(const std::string &path);
        void clear_selection();

        static void set_avatars_directory(const std::string &path);

        EventsManager<EventContext> when_selection_changed;

        void set_application_recursive(WaylandWindow *app) override;

    protected:
        void draw(GraphicsContext &gc) override;

    private:
        static std::string s_avatars_directory;
        std::string m_selected_avatar;
        IconView<std::string> *m_icon_view = nullptr;
        bool m_is_hovered = false;

        void build_vault();
        void load_avatars();
    };
} // namespace horizon
