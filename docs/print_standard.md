# Horizon Print System

This document explains how to implement and utilize standardized printing support in your Horizon applications and widgets.

## 1. Overview

Horizon's printing system follows a declarative and automated model, similar to the clipboard and undo/redo infrastructure, but with centralized UI management for printer selection:
- **Declarative**: Widgets optionally declare their ability to be printed and provide the generated document when requested.
- **Centralized UI**: Horizon core manages a unified `PrintDialog` that interfaces with the system's CUPS backend to discover printers and submit print jobs.
- **Automated Menu Management**: The Window system automatically manages the "File" menu. If any widget supports printing, the "Print" option is automatically injected at the bottom of the "File" menu.
- **Automatic Shortcuts**: Horizon automatically intercepts `Ctrl+P` and routes it to the correct printable widget.

## 2. Enabling Print Support in a Widget

To make your widget eligible for printing, you must override two methods from the base `Widget` class.

### 2.1. Declaration

Override `supports_printing()` to indicate that the widget knows how to generate a printable document:

```cpp
bool supports_printing() const override { 
    return true; 
}
```

### 2.2. Document Generation

When the user confirms the print job in the dialog, the framework will call `generate_print_document()`. This function executes in a background thread and should return a `PrintDocument` containing a valid PDF stream.

```cpp
#include <horizon/print/Models.h>

horizon::print::PrintDocument generate_print_document(const horizon::print::PrintConfig& config) override {
    horizon::print::PrintDocument doc;
    doc.format = "application/pdf";
    doc.title = "My Document Title";
    
    // Generate your PDF using Cairo or any other backend, taking into account
    // the page width/height provided in `config`
    
    // doc.data = std::move(pdf_buffer);
    
    return doc;
}
```

## 3. The Print Standard

### 3.1. Global Signals
The system listens to the `"print"` signal. Emitting this signal from anywhere in the application will automatically route the request to the best candidate and display the system `PrintDialog`.

### 3.2. Menu Generation
Horizon enforces a consistent UX. If `supports_printing()` is detected anywhere in the active widget tree:
1. The system ensures a "File" menu exists.
2. An **Imprimir** (Print) option is added at the bottom, preceded by a separator if there are other file actions.

### 3.3. Inversion of Control
The application (or widget) never creates the Print Dialog directly. The widget simply responds to the request to generate a PDF, while the `PrintDialog` manages the CUPS backend, asynchronous job submission, and error handling.

## 4. Minimal Implementation Example

```cpp
#include <horizon/Application.hpp>
#include <horizon/Widget.hpp>
#include <horizon/print/Models.h>

class MyPrintableWidget : public horizon::Widget {
public:
    MyPrintableWidget() {
        set_focusable(true);
    }

    bool supports_printing() const override { return true; }

    horizon::print::PrintDocument generate_print_document(const horizon::print::PrintConfig& config) override {
        horizon::print::PrintDocument doc;
        doc.title = "Test Print";
        doc.format = "application/pdf";
        
        // Populate doc.data with raw PDF bytes here
        
        return doc;
    }
};

int main(int argc, char **argv) {
    horizon::Application app("com.example.printer", 800, 600);
    app.set_name("Print Standard Example");
    
    auto my_widget = std::make_unique<MyPrintableWidget>();
    app.set_root(std::move(my_widget));
    
    app.run();
    return 0;
}
```

## 5. Manual Triggers via Signals

You can trigger the print dialog programmatically by emitting the `"print"` signal. The framework will automatically find the focused printable widget.

```cpp
// Example: Custom toolbar button to trigger Printing
my_print_button->when_click.connect([window = application()](auto&) {
    window->signal_manager.emit("print");
});
```

## 6. Target Discovery Logic

When a print request is initiated (either via `Ctrl+P`, a menu click, or a manual signal):
1. **Target Selection**: The system searches bottom-up starting from the **currently focused widget**.
2. **Fallback**: If the focused widget does not support printing, the system performs a top-down search from the root widget to find the first candidate that returns `true` for `supports_printing()`.
3. **Dialog Presentation**: The `PrintDialog` is spawned in an asynchronous thread, bound to the discovered target.
4. **Document Generation**: Only when the user selects a printer and clicks "Print" does the dialog invoke `generate_print_document()` on the target.

---
*Horizon Toolkit - Standardized User Interface Management*
