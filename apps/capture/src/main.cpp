#include <horizon/capture/CaptureEngine.h>
#include <horizon/Logger.hpp>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::string output_file = "screenshot.png";
    std::string monitor = "";
    bool help = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            help = true;
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) output_file = argv[++i];
        } else if (arg == "--monitor" || arg == "-m") {
            if (i + 1 < argc) monitor = argv[++i];
        }
    }

    if (help) {
        std::cout << "Usage: horizon-capture [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -o, --output <file>    Output file path (default: screenshot.png)" << std::endl;
        std::cout << "  -m, --monitor <name>   Monitor name to capture" << std::endl;
        std::cout << "  -h, --help             Show this help" << std::endl;
        return 0;
    }

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
    return 0;
}
