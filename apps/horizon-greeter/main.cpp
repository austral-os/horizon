#include "GreeterWindow.hpp"
#include "GreetdClient.hpp"
#include <horizon/About.hpp>
#include <horizon/Application.hpp>
#include <horizon/Logger.hpp>

namespace horizon::greeter
{
    /**
     * @class GreeterApplication
     * @brief Application class for the Greeter.
     */
    class GreeterApplication : public Application
    {
    public:
        GreeterApplication(bool debug_mode) : Application("horizon-greeter", 0, 0, false, true), m_debug_mode(debug_mode)
        {
            set_name("Horizon Greeter");

            // Setup About info
            auto &about = about_manager();
            about.set_app_title("Horizon Greeter");
            about.set_app_description("System login interface for Austral OS.");
            about.set_app_version("0.1.0");
            about.set_app_icon("avatar-default");
            about.set_app_git(horizon::ABOUT_HORIZON.git);
            about.add_app_author("Horacio Daniel Ros", "https://github.com/austral-os/horizon", "horaciodrs@gmail.com");

            m_greetd_client = std::make_unique<GreetdClient>();
            if (!m_greetd_client->connect())
            {
                LOG_WARNING << "Could not connect to greetd. Running in demo/test mode.";
            }

            auto window = std::make_unique<GreeterWindow>(*m_greetd_client, *this, m_debug_mode);
            // In initialize(), GreeterWindow will setup its Layer Shell properties.
            
            // Add it as the primary window
            m_managed_windows.push_back({std::move(window), nullptr, {}});
        }

        void run_greeter()
        {
            run();
        }

    private:
        std::unique_ptr<GreetdClient> m_greetd_client;
        bool m_debug_mode{false};
    };
} // namespace horizon::greeter

int main(int argc, char **argv)
{
    bool debug_mode = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--debug" || std::string(argv[i]) == "--console")
        {
            debug_mode = true;
        }
    }

    horizon::greeter::GreeterApplication app(debug_mode);
    app.run_greeter();
    return 0;
}
