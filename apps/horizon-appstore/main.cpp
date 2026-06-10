#include "AppStoreWindow.hpp"
#include <horizon/Application.hpp>
#include <horizon/I18n.hpp>
#include <iostream>
#include <horizon/dialogs/PreferencesContent.hpp>
#include <horizon/ConfigSection.hpp>
#include <horizon/TextBox.hpp>
#include <horizon/Label.hpp>
#include <horizon/Spacer.hpp>
#include <horizon/Button.hpp>
#include <horizon/apt/AppStoreClient.hpp>

class GeneralSettingsSection : public horizon::Widget, public horizon::ConfigSection {
public:
    GeneralSettingsSection(std::function<void()> on_change) : m_on_change(on_change) {
        set_layout_type(horizon::WIDGET_LAYOUT_VERTICAL);
        set_margin(30);
        set_spacing(15);
        
        auto lbl = std::make_unique<horizon::Label>(horizon::i18n().tr("appstore.preferences.api_url"));
        lbl->set_font_weight(horizon::FONT_WEIGHT_BOLD);
        lbl->set_fixed_size(25);
        add_child(std::move(lbl));
        
        auto txt = std::make_unique<horizon::TextBox<horizon::TextPolicy>>();
        m_url_box = txt.get();
        m_url_box->set_fixed_size(35); // height
        m_url_box->when_text_changed.connect([this](auto&) {
            if (m_on_change) m_on_change();
        });
        add_child(std::move(txt));
        
        auto btn = std::make_unique<horizon::Button<horizon::SolidObject>>();
        btn->set_text(horizon::i18n().tr("appstore.preferences.clear_cache"));
        btn->set_fixed_size(35);
        btn->when_click.connect([](auto&) {
            horizon::apt::AppStoreClient client;
            client.clear_cache();
        });
        add_child(std::move(btn));
        
        add_child(horizon::Spacer());
    }
    
    void from_json(const nlohmann::json &j) override {
        if (j.is_null()) return;
        if (j.contains("api_url")) {
            m_url_box->set_text(j["api_url"].get<std::string>());
        } else {
            m_url_box->set_text("");
        }
    }
    
    nlohmann::json to_json() const override {
        nlohmann::json j;
        j["api_url"] = m_url_box->text();
        return j;
    }
    
private:
    horizon::TextBoxBase* m_url_box;
    std::function<void()> m_on_change;
};

int main(int argc, char* argv[]) {
    auto app = std::make_unique<horizon::Application>("horizon-appstore", 1000, 700);
    
    // Load app-specific locales
    horizon::i18n().load_app_locales("horizon-appstore");

    char *home = std::getenv("HOME");
    std::string config_path = home ? std::string(home) + "/.config/horizon/appstore.json" : "appstore.json";

    app->set_preferences_content(
        [config_path]()
        {
            auto content = std::make_unique<horizon::PreferencesContent>(config_path);
            auto *content_ptr = content.get();
            auto on_change = [content_ptr]() { 
                content_ptr->save_config(); 
            };

            content->add_section(horizon::i18n().tr("appstore.preferences.general"),
                                 "preferences-system",
                                 std::make_unique<GeneralSettingsSection>(on_change), "appstore");

            return content;
        },
        600, 500);

    std::string initial_view = horizon::i18n().tr("appstore.views.featured");
    std::string initial_search = "";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << horizon::i18n().tr("appstore.help.usage") << std::endl;
            return 0;
        } else if (arg == "--explore" || arg == "explore") {
            initial_view = horizon::i18n().tr("appstore.views.explore");
        } else if (arg == "--updates" || arg == "updates") {
            initial_view = horizon::i18n().tr("appstore.views.updates");
        } else if (arg.find("--search=") == 0) {
            initial_search = arg.substr(9);
            initial_view = horizon::i18n().tr("appstore.views.explore");
        } else if (arg == "--search" && i + 1 < argc) {
            initial_search = argv[++i];
            initial_view = horizon::i18n().tr("appstore.views.explore");
        }
    }


    app->set_name("AppStore");
    app->set_icon_name("system-software-install");

    auto &about = app->about_manager();
    about.set_app_title("AppStore");
    about.set_app_description(horizon::i18n().tr("appstore.about.description"));
    about.set_app_version(APP_VERSION);
    about.set_app_icon("system-software-install");

    auto window = std::make_unique<horizon::appstore::AppStoreWindow>(initial_view, initial_search);
    app->set_root(std::move(window));
    
    app->run();
    
    return 0;
}
