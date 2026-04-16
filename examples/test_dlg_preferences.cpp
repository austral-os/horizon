#include <horizon/Application.hpp>
#include <horizon/AquaObject.hpp>
#include <horizon/Button.hpp>
#include <horizon/Checkbox.hpp>
#include <horizon/Icon.hpp>
#include <horizon/Label.hpp>
#include <horizon/Slider.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Window.hpp>
#include <horizon/dialogs/DialogPreferences.hpp>
#include <horizon/dialogs/PreferencesContent.hpp>
#include <iostream>

using namespace horizon;

// --- Section 1: General Settings ---
class GeneralSection : public Widget, public ConfigSection
{
public:
    GeneralSection(std::function<void()> on_change) : Widget(), m_on_change(on_change)
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(30);
        set_spacing(20);

        // Rate Slider
        add_child(std::make_unique<Label>("Velocidad (Rate):"));
        auto slider = std::make_unique<Slider>();
        slider->set_min(0.0f);
        slider->set_max(100.0f);
        slider->set_value(50.0f);
        slider->set_width(300);
        m_rate_slider = slider.get();

        m_rate_slider->when_value_changed.connect(
            [this](const EventContext &)
            {
                if (m_on_change)
                    m_on_change();
            });

        add_child(std::move(slider));

        // Start at boot Checkbox
        auto check = std::make_unique<Checkbox<AquaObject>>();
        check->set_text("Cargar al inicio");
        m_boot_check = check.get();

        m_boot_check->when_toggle.connect(
            [this](ToggleEventContext &)
            {
                if (m_on_change)
                    m_on_change();
            });

        add_child(std::move(check));
    }

    // ConfigSection implementation
    void from_json(const nlohmann::json &j) override
    {
        if (j.is_null())
            return;
        if (j.contains("rate"))
            m_rate_slider->set_value(j["rate"].get<float>());
        if (j.contains("cargar-inicio"))
            m_boot_check->set_checked(j["cargar-inicio"].get<bool>());
    }

    nlohmann::json to_json() const override
    {
        nlohmann::json j;
        j["rate"] = m_rate_slider->value();
        j["cargar-inicio"] = m_boot_check->is_checked();
        return j;
    }

private:
    Slider *m_rate_slider;
    Checkbox<AquaObject> *m_boot_check;
    std::function<void()> m_on_change;
};

// --- Section 2: Advanced Options ---
class AdvancedSection : public Widget, public ConfigSection
{
public:
    AdvancedSection(std::function<void()> on_change) : Widget(), m_on_change(on_change)
    {
        set_layout_type(WIDGET_LAYOUT_VERTICAL);
        set_margin(30);
        set_spacing(20);

        // File Path TextBox
        add_child(std::make_unique<Label>("Ruta de archivo:"));
        auto box = std::make_unique<TextBox<>>();
        box->set_placeholder("Ingrese ruta...");
        box->set_width(350);
        box->set_fixed_size(35);
        m_file_box = box.get();

        m_file_box->when_text_changed.connect(
            [this](const KeyEventContext &)
            {
                if (m_on_change)
                    m_on_change();
            });

        add_child(std::move(box));

        // Clean Checkbox
        auto check = std::make_unique<Checkbox<AquaObject>>();
        check->set_text("Limpiar al salir");
        m_clean_check = check.get();

        m_clean_check->when_toggle.connect(
            [this](ToggleEventContext &)
            {
                if (m_on_change)
                    m_on_change();
            });

        add_child(std::move(check));

        // Rate2 Slider
        add_child(std::make_unique<Label>("Prioridad (Rate 2):"));
        auto slider = std::make_unique<Slider>();
        slider->set_min(0.0f);
        slider->set_max(1.0f);
        slider->set_value(0.5f);
        slider->set_width(300);
        m_rate2_slider = slider.get();

        m_rate2_slider->when_value_changed.connect(
            [this](const EventContext &)
            {
                if (m_on_change)
                    m_on_change();
            });

        add_child(std::move(slider));
    }

    // ConfigSection implementation
    void from_json(const nlohmann::json &j) override
    {
        if (j.is_null())
            return;
        if (j.contains("file"))
            m_file_box->set_text(j["file"].get<std::string>());
        if (j.contains("clean"))
            m_clean_check->set_checked(j["clean"].get<bool>());
        if (j.contains("rate2"))
            m_rate2_slider->set_value(j["rate2"].get<float>());
    }

    nlohmann::json to_json() const override
    {
        nlohmann::json j;
        j["file"] = m_file_box->text();
        j["clean"] = m_clean_check->is_checked();
        j["rate2"] = m_rate2_slider->value();
        return j;
    }

private:
    TextBox<> *m_file_box;
    Checkbox<AquaObject> *m_clean_check;
    Slider *m_rate2_slider;
    std::function<void()> m_on_change;
};

int main()
{
    try
    {
        WaylandWindow app("horizon.test_dlg_preferences", 400, 300);
        app.set_name("Test DialogPreferences");

        auto wnd = std::make_unique<Window>("DialogPreferences Tester");
        wnd->set_margin(0);
        wnd->set_spacing(0);
        wnd->set_layout_type(WIDGET_LAYOUT_VERTICAL);

        auto lbl_info = std::make_unique<Label>("The 'Preferences' item in the global menu (App Menu) is now linked to the settings.");
        lbl_info->set_alignment(TextAlignment::Center);

        // Create PreferencesContent with an absolute path
        std::string home = std::getenv("HOME") ? std::getenv("HOME") : "";
        std::string absolute_path = home + "/" + "test_preferences_abs.json";

        // Set preferences content factory in the application
        // This lambda will be called every time show_preferences() is invoked.
        app.set_preferences_content([absolute_path]() {
            auto content = std::make_unique<PreferencesContent>(absolute_path);
            auto content_ptr = content.get();

            // Define auto-save callback for this instance
            auto save_callback = [content_ptr]()
            {
                content_ptr->save_config();
                std::cout << "[Test] Configuration auto-saved to disk." << std::endl;
            };

            // Add sections to this instance
            content->add_section("General Settings", "preferences-system",
                                        std::make_unique<GeneralSection>(save_callback));

            content->add_section("Advanced Options", "preferences-system-details",
                                        std::make_unique<AdvancedSection>(save_callback));

            return content;
        });

        auto btn_open = std::make_unique<Button<AquaObject>>();
        btn_open->set_text("Invoke show_preferences() manually");
        btn_open->set_fixed_size(40);
        btn_open->when_click.connect(
            [&app](MouseButtonEventContext &)
            {
                // Simple standardized call
                app.show_preferences();
            });

        wnd->add_child(std::move(lbl_info));
        wnd->add_child(std::move(btn_open));

        app.set_root(std::move(wnd));
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
