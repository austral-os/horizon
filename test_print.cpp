#include <horizon/Application.hpp>
#include "TextEditorWindow.hpp"
#include <horizon/core/WaylandWindow.hpp>
#include <iostream>

using namespace horizon;

class TestWindow : public WaylandWindow {
public:
    TestWindow() : WaylandWindow("test", 100, 100, false) {}
    bool test_detect(Widget* root) {
        return detect_print_support(root);
    }
};

int main() {
    auto window = std::make_unique<text_editor::TextEditorWindow>();
    window->new_file();
    
    TestWindow test_win;
    bool has_print = test_win.test_detect(window.get());
    std::cout << "Has print: " << (has_print ? "YES" : "NO") << std::endl;
    return 0;
}
