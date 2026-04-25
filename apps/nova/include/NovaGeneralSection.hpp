#pragma once

#include <functional>
#include <horizon/AirObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/Label.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Widget.hpp>
#include <string>

namespace horizon
{
    namespace nova
    {

        class NovaGeneralSection : public Widget, public ConfigSection
        {
        public:
            NovaGeneralSection(std::function<void()> on_change) : Widget(), m_on_change(on_change)
            {
                set_layout_type(WIDGET_LAYOUT_VERTICAL);
                set_margin(30);
                set_spacing(15);

                // 1. Homepage URL
                auto label = std::make_unique<Label>("Página de inicio:");
                label->set_fixed_size(35);
                add_child(std::move(label));

                auto url_box = std::make_unique<TextBox<>>();
                m_url_box = url_box.get();
                m_url_box->when_text_changed.connect(
                    [this](const KeyEventContext &)
                    {
                        if (m_on_change)
                            m_on_change();
                    });
                add_child(std::move(url_box));

                // 2. Buttons for URL
                auto btn_hbox = std::make_unique<Widget>();
                btn_hbox->set_layout_type(WIDGET_LAYOUT_HORIZONTAL);
                btn_hbox->set_spacing(10);
                btn_hbox->set_fixed_size(30);

                auto btn_blank = std::make_unique<Button<AirObject>>();
                btn_blank->set_text("Página en blanco");
                // btn_blank->set_fixed_size(150);
                btn_blank->when_click.connect(
                    [this](auto &)
                    {
                        m_url_box->set_text("about:blank");
                        m_url_box->set_enabled(false);
                        if (m_on_change)
                            m_on_change();
                    });

                auto btn_custom = std::make_unique<Button<AirObject>>();
                btn_custom->set_text("Personalizado");
                // btn_custom->set_fixed_size(120);
                btn_custom->when_click.connect(
                    [this](auto &)
                    {
                        m_url_box->set_enabled(true);
                        m_url_box->set_text("");
                        m_url_box->set_focus(true);
                        if (m_on_change)
                            m_on_change();
                    });

                btn_hbox->add_child(std::move(btn_blank));
                btn_hbox->add_child(std::move(btn_custom));
                add_child(std::move(btn_hbox));

                // 3. GPU Acceleration
                auto gpu_check = std::make_unique<Checkbox<AquaObject>>();
                gpu_check->set_text("Usar renderizado por GPU");
                m_gpu_check = gpu_check.get();
                m_gpu_check->when_toggle.connect(
                    [this](ToggleEventContext &)
                    {
                        if (m_on_change)
                            m_on_change();
                    });
                add_child(std::move(gpu_check));

                // 4. Spacer
                add_child(Spacer());
            }

            void from_json(const nlohmann::json &j) override
            {
                if (j.is_null())
                    return;

                if (j.contains("homepage"))
                {
                    std::string url = j["homepage"].get<std::string>();
                    m_url_box->set_text(url);
                    if (url == "about:blank")
                    {
                        m_url_box->set_enabled(false);
                    }
                    else
                    {
                        m_url_box->set_enabled(true);
                    }
                }

                if (j.contains("use_gpu"))
                {
                    m_gpu_check->set_checked(j["use_gpu"].get<bool>());
                }
            }

            nlohmann::json to_json() const override
            {
                nlohmann::json j = nlohmann::json::object();
                std::string url = m_url_box->text();
                if (url.empty())
                    url = "about:blank";
                j["homepage"] = url;
                j["use_gpu"] = m_gpu_check->is_checked();
                return j;
            }

        private:
            TextBox<> *m_url_box;
            Checkbox<AquaObject> *m_gpu_check;
            std::function<void()> m_on_change;
        };

    } // namespace nova
} // namespace horizon
