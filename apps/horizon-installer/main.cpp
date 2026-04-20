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

using namespace horizon;
using namespace horizon::installer;

class InstallerWindow : public Window
{
public:
    InstallerWindow(bool oobe_mode) : Window("Horizon Installer"), m_oobe_mode(oobe_mode)
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

        // 4. Disk Selection Page
        auto disk_page = std::make_unique<DiskPage>();
        auto* disk_page_ptr = disk_page.get();
        disk_page->when_back.connect([this](auto&) { m_wizard->back(); });
        disk_page->when_install.connect([this, disk_page_ptr](auto&) {
            m_config.target_device = disk_page_ptr->selected_device_str;
            m_wizard->next();
            start_installation();
        });
        m_wizard->add_page(std::move(disk_page));

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

        // Run installation in a separate thread to keep UI responsive
        std::thread([this]() {
            auto res = m_manager->run_stage1(m_config);
            if (res.success) {
                // Post to UI thread to transition to success page
                if (auto* app = application()) {
                    app->post_task([this]() {
                        m_wizard->next();
                    });
                }
            } else {
                // Handle error
            }
        }).detach();
    }

    bool m_oobe_mode;
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
    
    bool oobe = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--oobe") oobe = true;
    }

    auto win = std::make_unique<InstallerWindow>(oobe);
    app.set_root(std::move(win));
    
    app.run();
    return 0;
}
