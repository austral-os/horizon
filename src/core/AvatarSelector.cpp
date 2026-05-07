#include <horizon/AvatarSelector.hpp>
#include <horizon/Application.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/IconThemeLookup.hpp>
#include <horizon/Button.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Icon.hpp>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace horizon
{
    std::string AvatarSelector::s_avatars_directory = "/usr/share/horizon/avatars";

    AvatarSelector::AvatarSelector() : Widget()
    {
        set_focusable(true);
        set_fixed_size(64); // Default reasonable size
        m_selected_avatar = "avatar-default";

        when_mouse_enter.connect([this](EventContext &) {
            m_is_hovered = true;
            invalidate();
        });

        when_mouse_leave.connect([this](EventContext &) {
            m_is_hovered = false;
            invalidate();
        });

        build_vault();
    }

    void AvatarSelector::set_avatars_directory(const std::string &path)
    {
        s_avatars_directory = path;
    }

    void AvatarSelector::build_vault()
    {
        auto vault = std::make_unique<Vault>();

        auto content = std::make_unique<Widget>();
        content->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        content->set_spacing(10);
        content->set_width(480);

        // Title and Clear Button
        auto header = std::make_unique<Widget>();
        header->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
        header->set_fixed_size(30);

        auto title = std::make_unique<Label>("Select Avatar");
        title->set_font_weight(FONT_WEIGHT_BOLD);
        header->add_child(std::move(title));

        header->add_child(Spacer());

        auto clear_btn = std::make_unique<Button<AquaObject>>();
        clear_btn->set_text("Clear");
        clear_btn->set_fixed_size(80);
        clear_btn->when_click.connect([this](EventContext &) {
            clear_selection();
            if (application()) {
                application()->close_vault();
            }
        });
        header->add_child(std::move(clear_btn));

        content->add_child(std::move(header));

        // IconView for avatars
        auto icon_view = std::make_unique<IconView<std::string>>();
        m_icon_view = icon_view.get();
        m_icon_view->set_position_type(FILL);
        m_icon_view->set_size(480, 320);
        m_icon_view->set_zoom(1.0f);
        
        m_icon_view->set_item_size(64, 64);
        m_icon_view->set_side_margin(10);
        m_icon_view->set_item_factory([](const std::string &path, float zoom, bool selected) -> std::unique_ptr<Widget> {
            auto icon = std::make_unique<Icon>();
            icon->set_icon_size(64);
            icon->set_horizontal_alignment(TextAlignment::Center);
            icon->set_vertical_alignment(VerticalAlignment::Middle);
            icon->set_icon_name(path);
            icon->set_focusable(true);
            
            if (selected) {
                icon->set_background_color(Color(0.0f, 0.5f, 1.0f, 0.3f));
                icon->set_border_radius(8);
            }
            
            return icon;
        });

        m_icon_view->when_item_click.connect([this](IconViewItemMouseClickContext<std::string> &ctx) {
            m_selected_avatar = ctx.item_data;
            invalidate();
            EventContext ev;
            ev.sender = this;
            when_selection_changed.run(ev);
            if (application()) {
                application()->close_vault();
            }
        });

        content->add_child(std::move(icon_view));

        vault->set_content(std::move(content));
        set_vault(std::move(vault));
    }

    void AvatarSelector::set_application_recursive(WaylandWindow *app)
    {
        Widget::set_application_recursive(app);
        load_avatars(); // Load when we have application context
    }

    void AvatarSelector::load_avatars()
    {
        if (!m_icon_view) return;

        std::vector<std::string> avatars;
        std::string avatar_dir = s_avatars_directory;

        if (fs::exists(avatar_dir) && fs::is_directory(avatar_dir))
        {
            for (const auto &entry : fs::directory_iterator(avatar_dir))
            {
                if (entry.is_regular_file())
                {
                    avatars.push_back(entry.path().string());
                }
            }
        }
        
        // Sort alphabetically
        std::sort(avatars.begin(), avatars.end());
        
        m_icon_view->set_data(avatars);
    }

    std::string AvatarSelector::selected_avatar() const
    {
        return m_selected_avatar;
    }

    void AvatarSelector::clear_selection()
    {
        m_selected_avatar = "avatar-default";
        invalidate();
        EventContext ev;
        ev.sender = this;
        when_selection_changed.run(ev);
    }

    void AvatarSelector::draw(GraphicsContext &gc)
    {
        int size = std::min(m_width, m_height);
        int radius = size / 2;
        int center_x = m_x + m_width / 2;
        int center_y = m_y + m_height / 2;

        gc.save();
        
        // Circular clip
        gc.clipRoundedRect(center_x - radius, center_y - radius, size, size, CornerRadius(radius));

        // Draw background (Radial Gradient for a premium look)
        gc.fillGradientCircle(center_x, center_y, radius, 
                              Color(0.25f, 0.65f, 1.0f, 1.0f), 
                              Color(0.05f, 0.45f, 0.85f, 1.0f), 
                              GradientDirection::Radial);

        // Draw image or icon
        if (m_selected_avatar.empty() || m_selected_avatar == "avatar-default") {
            if (application() && application()->theme_manager) {
                std::string path = IconThemeLookup::find_icon("user-identity", size);
                if (path.empty()) path = IconThemeLookup::find_icon("avatar-default", size);
                
                if (!path.empty()) {
                    gc.drawImage(path, center_x - radius, center_y - radius, size, size);
                }
            }
        } else {
            gc.drawImage(m_selected_avatar, center_x - radius, center_y - radius, size, size);
        }

        // Draw hover effect
        if (m_is_hovered) {
            gc.setColor(Color(1.0f, 1.0f, 1.0f, 0.2f));
            gc.fillRect(center_x - radius, center_y - radius, size, size);
        }

        gc.restore();
        
        // Draw border
        gc.setColor(Color(0.5f, 0.5f, 0.5f, 0.5f));
        gc.drawCircle(center_x, center_y, radius, 2.0f);
    }
} // namespace horizon
