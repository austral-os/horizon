#include <horizon/apt/AppStoreClient.hpp>
#include <glib.h>
#include <iostream>

using namespace horizon::apt;

int main() {
    AppStoreClient client;
    GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

    std::cout << "Testing get_app_details_async for horizon-arkfm..." << std::endl;
    client.get_app_details_async("horizon-arkfm", "es", [&](std::optional<AppDetails> details) {
        if (details) {
            std::cout << "Successfully retrieved app details. Versions: " << details->versions.size() << std::endl;
            if (!details->versions.empty()) {
                std::string ver = details->versions[0].version_string;
                std::cout << "Latest version: " << ver << std::endl;
                client.get_app_version_details_async("horizon-arkfm", ver, "es", [&](std::optional<AppVersionDetails> v_details) {
                    if (v_details) {
                        std::cout << "Successfully retrieved version details." << std::endl;
                        std::cout << "Name: " << v_details->name << std::endl;
                        if (v_details->description) std::cout << "Desc: " << *v_details->description << std::endl;
                    } else {
                        std::cerr << "Failed to retrieve version details." << std::endl;
                    }
                    g_main_loop_quit(loop);
                });
            } else {
                g_main_loop_quit(loop);
            }
        } else {
            std::cerr << "Failed to retrieve app details." << std::endl;
            g_main_loop_quit(loop);
        }
    });

    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    return 0;
}
