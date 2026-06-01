#include "ArkfmApplication.hpp"

using horizon::arkfm::ArkfmApplication;

int main(int argc, char **argv)
{
    std::string initial_path = "";
    if (argc > 1) {
        initial_path = argv[1];
    }
    ArkfmApplication app(initial_path);
    app.run();
    return 0;
}