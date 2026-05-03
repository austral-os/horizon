#include <horizon/capture/CaptureEngine.h>
#include <horizon/capture/VideoRecorder.h>
#include <horizon/capture/SelectionWindow.h>
#include <horizon/WaylandSurface.hpp>
#include <horizon/Logger.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>
#include <wayland-client.h>
#include <signal.h>

static std::shared_ptr<horizon::capture::VideoRecorder> g_recorder = nullptr;
static std::atomic<bool> g_keep_running{true};

void signal_handler(int) {
    LOG_INFO << "[CaptureApp] Signal received, stopping...";
    g_keep_running = false;
}

int main(int argc, char** argv) {
    std::string output_file = "";
    std::string monitor_name = "";
    bool help = false;
    bool selection_mode = false;
    bool record_video = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            help = true;
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) output_file = argv[++i];
        } else if (arg == "--monitor" || arg == "-m") {
            if (i + 1 < argc) monitor_name = argv[++i];
        } else if (arg == "--select" || arg == "-s") {
            selection_mode = true;
        } else if (arg == "--record" || arg == "-r") {
            record_video = true;
        }
    }

    if (help) {
        std::cout << "Usage: horizon-capture [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -o, --output <file>    Output file path" << std::endl;
        std::cout << "  -m, --monitor <name>   Monitor name to capture (default: all/primary)" << std::endl;
        std::cout << "  -s, --select           Interactive selection mode" << std::endl;
        std::cout << "  -r, --record           Record video instead of screenshot" << std::endl;
        std::cout << "  -h, --help             Show this help" << std::endl;
        return 0;
    }

    if (output_file.empty()) {
        output_file = record_video ? "recording.mp4" : "screenshot.png";
    }

    // Default behavior: if no selection mode and no monitor specified, 
    // we could either default to selection OR full screen.
    // User expects full screen by default if not told otherwise.
    
    int final_x = 0, final_y = 0, final_w = 0, final_h = 0;
    bool capture_ready = false;

    if (selection_mode) {
        LOG_INFO << "[CaptureApp] Starting selection mode";
        auto selection_win = std::make_shared<horizon::capture::SelectionWindow>();
        
        bool selection_done = false;
        selection_win->selection_widget()->when_selected().connect([&](horizon::capture::SelectionRect rect) {
            final_x = rect.x + selection_win->screen_x();
            final_y = rect.y + selection_win->screen_y();
            final_w = rect.width;
            final_h = rect.height;
            
            if (final_w % 2 != 0) final_w--;
            if (final_h % 2 != 0) final_h--;

            selection_win->set_visible(false);
            selection_win->add_timer(300, [&]() {
                selection_done = true;
                selection_win->quit();
            });
        });

        selection_win->selection_widget()->when_cancelled().connect([&](horizon::EventContext&) {
            exit(0);
        });

        selection_win->initialize();
        selection_win->run();
        
        if (!selection_done) return 0;
        capture_ready = true;
    } else {
        // Full screen mode
        LOG_INFO << "[CaptureApp] Initializing full screen capture...";
        horizon::capture::CaptureEngine engine;
        if (engine.init()) {
            // We'll use the CaptureEngine to get dimensions of the target output
            // (or the first one by default)
            // For now, let's assume we capture the first output.
            // In a real app, we'd iterate and sum or choose.
            // But let's keep it simple for MVP.
            
            // To get dimensions, we can't easily query them from CaptureEngine without capturing.
            // Let's use a small trick: use SelectionWindow's initialization logic 
            // but without showing it, to get monitor info.
            // Or just hardcode/query Wayland directly.
            
            // Better: use the SelectionWindow which already does the work.
            auto dummy_win = std::make_shared<horizon::capture::SelectionWindow>();
            dummy_win->initialize();
            
            final_x = 0;
            final_y = 0;
            final_w = dummy_win->width();
            final_h = dummy_win->height();
            
            if (final_w % 2 != 0) final_w--;
            if (final_h % 2 != 0) final_h--;
            
            capture_ready = true;
        }
    }

    if (!capture_ready) return 1;

    // --- Execution Phase ---
    if (record_video) {
        LOG_INFO << "[CaptureApp] Starting video recording: " << final_w << "x" << final_h;
        g_recorder = std::make_shared<horizon::capture::VideoRecorder>();
        signal(SIGINT, signal_handler);
        
        if (g_recorder->start(output_file, final_x, final_y, final_w, final_h)) {
            std::cout << "Recording... Press Ctrl+C to stop." << std::endl;
            while (g_keep_running && g_recorder->is_recording()) {
                usleep(100000);
            }
            LOG_INFO << "[CaptureApp] Stopping recording...";
            g_recorder->stop();
            std::cout << "Video saved to " << output_file << std::endl;
        } else {
            std::cerr << "Failed to start recording" << std::endl;
            return 1;
        }
    } else {
        horizon::capture::CaptureEngine engine;
        if (engine.init()) {
            if (engine.capture_region(monitor_name, final_x, final_y, final_w, final_h, output_file)) {
                std::cout << "Screenshot saved to " << output_file << std::endl;
            } else {
                std::cerr << "Failed to capture screenshot" << std::endl;
                return 1;
            }
        }
    }

    return 0;
}
