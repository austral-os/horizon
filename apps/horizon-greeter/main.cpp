#include "GreeterWindow.hpp"
#include "GreetdClient.hpp"
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
        GreeterApplication() : Application("horizon-greeter", 0, 0, false, true)
        {
            set_name("Horizon Greeter");

            m_greetd_client = std::make_unique<GreetdClient>();
            if (!m_greetd_client->connect())
            {
                LOG_WARNING << "Could not connect to greetd. Running in demo/test mode.";
            }

            auto window = std::make_unique<GreeterWindow>(*m_greetd_client, *this);
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
    };
} // namespace horizon::greeter

int main(int argc, char **argv)
{
    horizon::greeter::GreeterApplication app;
    app.run_greeter();
    return 0;
}
