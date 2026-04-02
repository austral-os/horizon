#pragma once

#include <ConfigSection.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Label.hpp>
#include <horizon/Slider.hpp>
#include <horizon/Widget.hpp>

namespace horizon::preferences
{
    /**
     * @class DockView
     * @brief View for configuring the Dock appearance and behavior.
     */
    class DockView : public horizon::Widget, public ConfigSection
    {
    public:
        DockView();
        ~DockView() override = default;

        // ConfigSection implementation
        void from_json(const nlohmann::json &j) override;
        nlohmann::json to_json() const override;

    private:
        void save_config();

    private:
        horizon::Slider *m_size_slider{nullptr};
        horizon::Label *m_size_label{nullptr};
        horizon::Checkbox<horizon::AquaObject> *m_magnification_check{nullptr};

        int m_icon_size{64};
        bool m_magnification_enabled{true};
        nlohmann::json m_config_data;
    };
} // namespace horizon::preferences
