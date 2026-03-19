#pragma once

#include <horizon/Widget.hpp>
#include <horizon/Label.hpp>
#include <horizon/Image.hpp>
#include <ConfigSection.hpp>
#include <horizon/TreeView.hpp>
#include <ImagesView.hpp>
#include <horizon/Combo.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Button.hpp>

namespace horizon::preferences
{
    struct BackgroundRoute
    {
        std::string name;
        std::string path;
    };

    struct BackgroundSource
    {
        std::string name;
        std::vector<BackgroundRoute> routes;
    };

    /**
     * @class DesktopView
     * @brief View for configuring desktop wallpaper and related settings.
     */
    class DesktopView : public horizon::Widget, public ConfigSection
    {
    public:
        DesktopView();
        ~DesktopView() override = default;

        // ConfigSection implementation
        void from_json(const nlohmann::json& j) override;
        nlohmann::json to_json() const override;

        void calculate_layout() override;

        /**
         * @brief Get the list of background sources.
         * @return const std::vector<BackgroundSource>& 
         */
        const std::vector<BackgroundSource>& get_sources() const { return m_sources; }

    protected:
        void update_layout();

    private:
        void create_section_b(horizon::Widget* parent);
        void create_section_ac(horizon::Widget* parent);
        void create_section_d(horizon::Widget* parent);
        
        void update_tree_view();
        void save_config();

    private:
        // Section B: Preview
        horizon::Image* m_preview_image{nullptr};
        horizon::Label* m_image_name_label{nullptr};
        horizon::Combo* m_fit_combo{nullptr};

        // Section A: TreeView
        horizon::TreeView* m_tree_view{nullptr};

        // Section C: ImagesView
        ImagesView* m_images_view{nullptr};

        // Section D: Actions & Settings
        horizon::Button<horizon::AquaObject>* m_add_button{nullptr};
        horizon::Button<horizon::AquaObject>* m_remove_button{nullptr};
        horizon::Checkbox<horizon::AquaObject>* m_change_check{nullptr};
        horizon::Checkbox<horizon::AquaObject>* m_random_check{nullptr};
        horizon::Checkbox<horizon::AquaObject>* m_translucent_check{nullptr};
        horizon::Combo* m_timer_combo{nullptr};

        std::vector<BackgroundSource> m_sources;
        horizon::TreeViewItem* m_first_route_to_select{nullptr};
        bool m_initial_selection_done{false};

        std::string m_current_image_name;
        std::string m_current_image_full_path;
        std::string m_current_source;
    };
} // namespace horizon::preferences
