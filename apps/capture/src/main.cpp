#include <horizon/capture/CaptureEngine.h>
#include <horizon/capture/SelectionWindow.h>
#include <horizon/WaylandSurface.hpp>
#include <horizon/Logger.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>
#include <wayland-client.h>

int main(int argc, char** argv) {
    std::string output_file = "screenshot.png";
    std::string monitor = "";
    bool help = false;
    bool selection_mode = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            help = true;
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) output_file = argv[++i];
        } else if (arg == "--monitor" || arg == "-m") {
            if (i + 1 < argc) monitor = argv[++i];
        } else if (arg == "--select" || arg == "-s") {
            selection_mode = true;
        }
    }

    if (help) {
        std::cout << "Usage: horizon-capture [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -o, --output <file>    Output file path (default: screenshot.png)" << std::endl;
        std::cout << "  -m, --monitor <name>   Monitor name to capture (full screen)" << std::endl;
        std::cout << "  -s, --select           Interactive selection mode" << std::endl;
        std::cout << "  -h, --help             Show this help" << std::endl;
        return 0;
    }

    if (monitor.empty()) {
        selection_mode = true;
    }

    if (selection_mode) {
        LOG_INFO << "[CaptureApp] Starting selection mode";
        auto selection_win = std::make_shared<horizon::capture::SelectionWindow>();
        
        selection_win->selection_widget()->when_selected().connect([selection_win, output_file](horizon::capture::SelectionRect rect) {
            int global_x = rect.x + selection_win->screen_x();
            int global_y = rect.y + selection_win->screen_y();
            int w = rect.width;
            int h = rect.height;
            
            LOG_INFO << "[CaptureApp] Selected Region: " << global_x << "," << global_y << " " << w << "x" << h;
            
            // 1. Hide the window immediately
            selection_win->set_visible(false);
            
            // 2. Use a timer to defer capture. 
            // This is CRITICAL because it allows the event loop to continue running,
            // process the window hiding request, repaint the screen, and ensure
            // the compositor has a clean frame BEFORE we start the capture process.
            selection_win->add_timer(300, [global_x, global_y, w, h, output_file]() {
                LOG_INFO << "[CaptureApp] Timer expired, performing capture...";
                
                horizon::capture::CaptureEngine engine;
                if (engine.init()) {
                    if (engine.capture_region("", global_x, global_y, w, h, output_file)) {
                        std::cout << "Screenshot saved to " << output_file << std::endl;
                    } else {
                        std::cerr << "Failed to capture selection" << std::endl;
                    }
                }
                exit(0);
            });
            
            // We return from this callback, allowing SelectionWindow::run() to continue the loop.
        });

        selection_win->selection_widget()->when_cancelled().connect([&](horizon::EventContext&) {
            LOG_INFO << "[CaptureApp] Selection cancelled";
            exit(0);
        });

        selection_win->initialize();
        selection_win->run();
    } else {
        horizon::capture::CaptureEngine engine;
        if (!engine.init()) {
            std::cerr << "Failed to initialize CaptureEngine" << std::endl;
            return 1;
        }

        if (!engine.capture_screenshot(monitor, output_file)) {
            std::cerr << "Failed to capture screenshot" << std::endl;
            return 1;
        }

        std::cout << "Screenshot saved to " << output_file << std::endl;
    }

    return 0;
}
