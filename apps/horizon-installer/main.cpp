#include <horizon/Application.hpp>
#include <horizon/WaylandWindow.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <horizon/VPanel.hpp>
#include <horizon/Toolbar.hpp>
#include <horizon/ToolbarButton.hpp>
#include <horizon/I18n.hpp>
#include "Wizard.hpp"
#include "InstallerPages.hpp"
#include <horizon/Window.hpp>
#include <horizon-installer-utils/InstallerManager.hpp>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <linux/limits.h>
#include <horizon/Logger.hpp>

using namespace horizon;
using namespace horizon::installer;

class InstallerWindow : public Window
{
public:
    InstallerWindow(bool oobe_mode, const std::string& initial_view = "", bool working_mode = false) 
        : Window("Horizon Installer"), m_oobe_mode(oobe_mode), m_initial_view(initial_view), m_working_mode(working_mode)
    {
        set_size(1000, 800);
        
        m_manager = std::make_unique<InstallerManager>();

        auto root = std::make_unique<Widget>();
        root->set_layout_type(WIDGET_LAYOUT_VERTICAL);
        
        auto wizard = std::make_unique<Wizard>();
        m_wizard = wizard.get();
        root->add_child(std::move(wizard));

        // Start stage 1: Only Language Page
        setup_language_page();

        if (!m_initial_view.empty()) {
            // If a specific view is requested, we skip language selection and jump
            setup_remaining_pages();
            jump_to_view(m_initial_view);
        }

        add_child(std::move(root));
    }

private:
    void setup_language_page()
    {
        auto lang_page = std::make_unique<LanguagePage>();
        auto* ptr = lang_page.get();
        lang_page->when_continue.connect([this, ptr](auto&) {
            // Apply selected locale
            std::string code = ptr->selected_language_code();
            std::cout << "Setting locale to: " << code << std::endl;
            m_config.locale = code;
            horizon::i18n().set_locale(code);
            horizon::i18n().load_app_locales("horizon-installer");
            
            // Now that locale is set, create the rest of the pages
            setup_remaining_pages();
            m_wizard->next();
        });
        m_wizard->add_page(std::move(lang_page));
    }

    void setup_remaining_pages()
    {
        if (m_oobe_mode)
        {
            // OOBE Mode: Language -> User -> Progress -> Success
            
            // 2. User Page
            auto user_page = std::make_unique<UserPage>();
            auto* user_ptr = user_page.get();
            user_page->when_back.connect([this](auto&) { m_wizard->back(); });
            user_page->when_continue.connect([this, user_ptr](auto&) {
                m_config.fullname = user_ptr->fullname();
                m_config.username = user_ptr->username();
                m_config.password = user_ptr->password();
                m_config.hostname = "austral-pc"; // Default or prompt
                m_config.is_oobe = true;
                
                m_wizard->next();
                start_installation();
            });
            m_wizard->add_page(std::move(user_page));
        }
        else
        {
            // 2. Welcome Page
            auto welcome_page = std::make_unique<WelcomePage>();
            welcome_page->when_continue.connect([this](auto&) { m_wizard->next(); });
            m_wizard->add_page(std::move(welcome_page));

            // 3. License Page
            auto license_page = std::make_unique<LicensePage>();
            license_page->when_agree.connect([this](auto&) { m_wizard->next(); });
            license_page->when_disagree.connect([this](auto&) { 
                // Handle disagreement: for now just exit or go back
            });
            m_wizard->add_page(std::move(license_page));

            // 4. Region Page
            auto region_page = std::make_unique<RegionPage>();
            auto* region_ptr = region_page.get();
            region_page->when_back.connect([this](auto&) { m_wizard->back(); });
            region_page->when_continue.connect([this, region_ptr](auto&) {
                m_config.country = region_ptr->selected_country();
                m_config.timezone = region_ptr->selected_timezone();
                m_wizard->next();
            });
            m_wizard->add_page(std::move(region_page));

            // 5. Disk Selection Page
            auto disk_page = std::make_unique<DiskPage>();
            auto* disk_page_ptr = disk_page.get();
            disk_page->when_back.connect([this](auto&) { m_wizard->back(); });
            disk_page->when_install.connect([this, disk_page_ptr](auto&) {
                m_config.target_device = disk_page_ptr->selected_device_str;
                m_config.is_oobe = false;
                m_wizard->next();
                start_installation();
            });
            m_wizard->add_page(std::move(disk_page));
        }

        // 5. Progress Page
        auto install_page = std::make_unique<InstallPage>();
        m_install_page_ptr = install_page.get();
        install_page->when_cancel.connect([this](auto&) {
            // Handle cancellation
        });
        m_wizard->add_page(std::move(install_page));

        // 6. Success Page
        auto success_page = std::make_unique<SuccessPage>();
        success_page->when_finish.connect([this](auto&) {
            std::cout << "Rebooting system..." << std::endl;
            if (auto* app = application()) app->quit();
        });
        m_wizard->add_page(std::move(success_page));
    }

    void start_installation()
    {
        m_manager->set_progress_callback([this](float progress, const std::string& msg) {
            if (m_install_page_ptr)
                m_install_page_ptr->update_progress(progress, msg);
        });

        if (m_working_mode) {
            LOG_INFO << "[DEV] DRY-RUN MODE: Skipping real installation actions.";
            LOG_INFO << "[DEV] Target: " << m_config.target_device << ", OOBE: " << (m_oobe_mode ? "YES" : "NO");
            
            // Just simulate success after a short delay
            std::thread([this]() {
                for (int i = 0; i <= 100; i += 10) {
                    usleep(200000);
                    if (auto* app = application()) {
                        app->post_task([this, i]() {
                            if (m_install_page_ptr) 
                                m_install_page_ptr->update_progress(i / 100.0f, "Simulating installation... " + std::to_string(i) + "%");
                        });
                    }
                }
                if (auto* app = application()) {
                    app->post_task([this]() {
                        m_wizard->next();
                    });
                }
            }).detach();
            return;
        }
        std::thread([this]() {
            StepResult res = m_oobe_mode ? m_manager->run_stage2(m_config) : m_manager->run_stage1(m_config);
            
            if (res.success) {
                // Post to UI thread to transition to success page
                if (auto* app = application()) {
                    app->post_task([this]() {
                        m_wizard->next();
                    });
                }
            } else {
                std::cerr << "Installation error: " << res.message << std::endl;
            }
        }).detach();
    }

    void jump_to_view(const std::string& view_name)
    {
        size_t index = 0;
        if (m_oobe_mode) {
            if (view_name == "new-user") index = 1;
            else if (view_name == "progress") index = 2;
            else if (view_name == "success") index = 3;
        } else {
            if (view_name == "welcome") index = 1;
            else if (view_name == "license") index = 2;
            else if (view_name == "region") index = 3;
            else if (view_name == "disk") index = 4;
            else if (view_name == "progress") index = 5;
            else if (view_name == "success") index = 6;
        }

        if (index > 0) {
            m_wizard->show_page(index);
        }
    }

    bool m_oobe_mode;
    std::string m_initial_view;
    bool m_working_mode;
    Wizard* m_wizard;
    std::unique_ptr<InstallerManager> m_manager;
    InstallationConfig m_config;
    InstallPage* m_install_page_ptr{nullptr};
};

int main(int argc, char** argv)
{
    Application app("org.austral.horizon.installer", 1000, 800);
    
    // Robust I18n discovery
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        std::string path(result, count);
        std::string dir = path.substr(0, path.find_last_of('/'));
        I18n::add_search_path(dir);
        I18n::add_search_path(dir + "/..");
    }
    
    I18n::add_search_path(".");
    horizon::i18n().load_app_locales("horizon-installer");
    
    app.set_name(horizon::i18n().tr("installer.welcome.title"));
    
    bool oobe = InstallerManager::is_oobe_pending();
    bool working = false;
    std::string view = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--oobe") oobe = true;
        else if (arg == "--working") working = true;
        else if (arg.find("--view-") == 0) {
            view = arg.substr(7);
        }
    }

    auto win = std::make_unique<InstallerWindow>(oobe, view, working);
    app.set_root(std::move(win));
    
    app.run();
    return 0;
}
