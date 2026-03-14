#include "horizon/CairoGraphicsContext.hpp"
#include "horizon/Widget.hpp"
#include <cstdio>
#include <cstring>
#include <horizon/Application.hpp>
#include <horizon/ClientMenu.hpp>
#include <horizon/IpcClient.hpp>
#include <horizon/LabwcCompositorContext.hpp>
#include <horizon/Logger.hpp>
#include <horizon/Menu.hpp>
#include <horizon/WayfireCompositorContext.hpp>
#include <horizon/Window.hpp>
#include <horizon/xdg-shell-client-protocol.h>
#include <librsvg/rsvg.h>
#include <linux/input-event-codes.h>
#include <nlohmann/json.hpp>
#include <poll.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

namespace horizon
{

    Application::Application(const std::string &app_id, int w, int h)
        : Application(app_id, w, h, false)
    {
    }

    Application::Application(const std::string &app_id, int w, int h, bool defer_init)
        : HznSurface(app_id)
    {
        // Global safeguard: ignore SIGPIPE to prevent crash when writing to broken sockets
        signal(SIGPIPE, SIG_IGN);
    }

    Application::~Application() {}

    void Application::dispatch_events() {}

} // namespace horizon
