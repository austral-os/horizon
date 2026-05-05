<p align="center">
  <img src="images/horizon-lib.png" alt="Horizon Logo" width="128">
</p>

<h1 align="center">Horizon</h1>

<p align="center">
  <strong>A desktop framework for Linux built from scratch.</strong><br>
  No GTK. No Qt. Just full control over your UI.
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-LGPL_v3-blue.svg?style=flat-square" alt="License"></a>
  <img src="https://img.shields.io/badge/Status-Alpha-orange?style=flat-square" alt="Status">
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=c%2B%2B" alt="C++17">
  <img src="https://img.shields.io/badge/Platform-Wayland-red?style=flat-square" alt="Platform">
</p>

<p align="center">
  <img src="images/screenshot1.png" alt="Horizon Dashboard" width="700" style="border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.2);">
</p>

<p align="center">
  Horizon is a desktop framework built entirely without GTK or Qt, providing full control over the UI stack from rendering to widgets. The core technology behind <strong><a href="https://github.com/austral-os/austral-os">Austral OS</a></strong>, a fully integrated Linux system built around Horizon.
</p>

---

## ⚠️ Project Status: Alpha

Horizon is currently in **active alpha development**.

- **Expect Breaking Changes**: The API is not yet stable and may change frequently.
- **Incomplete Features**: Many planned components and features are still under construction.
- **Bugs & Stability**: As an early-stage project, you will encounter bugs and stability issues.

We are working toward a stable beta, but for now, Horizon is intended for developers, contributors, and those interested in experimental desktop technology.

Feedback and contributions are welcome. If you're interested in contributing, feedback and discussions are highly welcome.

---

## 🌟 Overview

Horizon is a high-performance graphical toolkit for Linux, independent from traditional libraries like GTK or Qt.

It gives developers full control over the UI stack — from low-level rendering to high-level widgets — enabling consistent and predictable interfaces.

Designed for Wayland-native environments, Horizon focuses on clarity, performance, and visual consistency.

Its visual design draws inspiration from classic macOS releases like Mavericks and Leopard, reimagined for a modern, Wayland-native Linux environment.

---

## 💡 Why Horizon?

Horizon exists because building modern Linux interfaces still depends on large, complex, and legacy-heavy toolkits.

In a landscape dominated by those toolkits, Horizon offers a different path:

- **Total Independence**: No GTK, no Qt. By building our own stack, we eliminate legacy overhead and deep dependency chains.
- **Full Control**: Every pixel and every event is handled directly, allowing for optimizations and UI patterns that are difficult to achieve in restricted toolkits.
- **Architectural Clarity**: A modern C++17 codebase that is modular, easy to audit, and free from decades of technical debt.
- **System Integration**: Deeply integrated with Wayland and the Austral OS ecosystem, providing a consistent identity for the entire desktop experience.

---

## 🖥️ Experience Horizon

Horizon is not just a toolkit — it powers an entire desktop environment.

The best way to experience it today is through **Austral OS**, where Horizon drives the full system UI.

👉 [Explore Austral OS](https://github.com/austral-os/austral-os)

---

## 👥 Who is this for?

Horizon is designed for:

- Developers who want full control over their UI stack
- People interested in building desktop environments from scratch
- Those who prefer simplicity and clarity over large frameworks
- Experimenters exploring Wayland-native systems

---

## 🎨 Philosophy

- **Minimalism & Control**: Prioritizing explicit control over memory and resources over heavy abstraction.
- **Independence**: No unnecessary dependencies; built for the Austral ecosystem.
- **Modern Rendering**: A lightweight rendering engine based on Cairo with GLES support for fluid animations.
- **Developer-Centric**: A clear, extensible architecture that is easy to audit and learn from.

---

## 🏗️ Architecture

Horizon follows a modular, layered design to ensure maintainability and high performance.

### Core Layers

1.  **Core**: Manages the application lifecycle, main event loop, and Wayland seat/display integration.
2.  **Renderer**: Surface abstractions and buffer management (currently utilizing Cairo with Rsvg support).
3.  **Widgets**: A growing set of components including windows, buttons, labels, and flexible layout containers.
4.  **Runtime**: Integrated support for asset management and the Austral distribution format.

---

## 🧩 UI Components (Widgets)

Horizon provides a rich set of built-in widgets for building modern interfaces. Every widget inherits from the base [**`Widget`**](docs/widgets/Widget.md) class and follows the Horizon layout philosophy.

- [**`Advanced Views`**](docs/widgets/AdvancedViews.md): TableView, TreeView, IconView, and CoverFlow.
- [**`Button`**](docs/widgets/Button.md): Generic interactive button with glassmorphism support.
- [**`Controls`**](docs/widgets/Controls.md): Checkbox, RadioButton, Slider, ProgressBar, and Text inputs.
- [**`Icon`**](docs/widgets/Icon.md): Themed vector and raster icon rendering.
- [**`Label`**](docs/widgets/Label.md): Multi-line text display with automatic wrapping.
- [**`Menu System`**](docs/widgets/MenuSystem.md): Menu, MenuBar, and standard context menus.
- [**`Notebook`**](docs/widgets/Notebook.md): Tabbed container for organizing content.
- [**`ScrollArea`**](docs/widgets/ScrollArea.md): Viewport for scrolling large content areas.
- [**`Spacer`**](docs/widgets/Spacer.md): Layout helper for alignment and spacing.
- [**`Structure & Navigation`**](docs/widgets/StructureAndNavigation.md): Toolbar, Sidebar, Panels, and Statusbars.
- [**`Window`**](docs/widgets/Window.md): Standard application window frame and decorations.

---

## 📚 Documentation

Detailed guides on implementing core framework features:

- [**Clipboard System**](docs/clipboard.md): Standardized signal-based copy, cut, and paste implementation.
- [**Fullscreen System**](docs/fullscreen.md): Declarative fullscreen support and visual isolation protocol.
- [**File Operations Standard**](docs/file_operations_standard.md): Automated file menu injection and standardized operations.
- [**Localization (i18n)**](docs/localization.md): Native multi-language support with JSON backends.
- **Dialog System**: Unified interface for user interaction via modal dialogs.
    - [**MessageDialog System**](docs/message_dialogs.md): Standardized alert and confirmation dialogs.
    - [**FileDialog System**](docs/file_dialog.md): Usage and invocation of the standard file selection dialog.
    - [**Preferences Dialog System**](docs/preferences_dialog.md): Settings management system architecture.
    - [**Font Selection System**](docs/font_dialog.md): Integration of system font selection tools.
    - [**Color Selection System**](docs/color_selector.md): Integration of system color selection tools.
    - [**AboutUs Dialog System**](docs/aboutus_dialog.md): Application information dialog architecture.
- [**Configuration Management**](docs/configuration_management.md): Robust system for managing JSON-based configurations.
- [**File Watcher System**](docs/file_watcher.md): Real-time monitoring of filesystem changes.

---

## 🛠️ Build & Installation

Horizon is designed to be built on Linux environments using Wayland.

### 1. Prerequisites (Debian/Ubuntu)

Install the necessary development libraries and tools:

```bash
sudo apt update
sudo apt install build-essential g++ pkg-config \
                 cmake ninja-build git \
                 libwayland-dev wayland-protocols \
                 libxkbcommon-dev libcairo2-dev \
                 librsvg2-dev libpixman-1-dev \
                 libegl1-mesa-dev libgles2-mesa-dev \
                 libdbus-1-dev libvterm-dev \
                 libpipewire-0.3-dev libspa-0.2-dev \
                 libfontconfig1-dev libfreetype6-dev \
                 libharfbuzz-dev uuid-dev \
                 libwpe-1.0-dev libwpebackend-fdo-1.0-dev \
                 libwpewebkit-2.0-dev libpoppler-glib-dev \
                 libpango1.0-dev libmount-dev \
                 nlohmann-json3-dev wayland-utils \
                 wlr-randr xdg-utils shared-mime-info \
                 desktop-file-utils libmpv-dev
```

_Note: For applications like Nova (web browser), you may also need multimedia plugins:_

```bash
sudo apt install gstreamer1.0-plugins-bad gstreamer1.0-libav
```

### 2. Compiling the Project

Building is straightforward using CMake and Ninja:

```bash
git clone https://github.com/austral-os/horizon.git
cd horizon
mkdir build && cd build
cmake -G Ninja ..
ninja
```

### 3. Running the Demos

To test the installation, you can run the `minimal` demo:

```bash
./minimal
```

_Note: Ensure you are in a Wayland session or running a compositor like Wayfire or labwc._

---

## 📦 Packaging

Horizon supports generating `.deb` packages for Debian-based systems.

### 1. Using Ninja (Recommended)

Three targets are available for easy packaging from the `build` directory:

- **Monolithic Package** (All applications + libraries in one `.deb`):
    ```bash
    ninja package-monolithic
    ```
- **Individual Packages** (One `.deb` per application + library):
    ```bash
    ninja package-components
    ```
- **Standard Package** (Default configuration):
    ```bash
    ninja package
    ```

### 2. Runtime Requirements

To run Horizon-based applications, ensure these core runtime libraries are present:

|                           |                      |                  |                        |
| ------------------------- | -------------------- | ---------------- | ---------------------- |
| `desktop-file-utils`      | `libliftoff0`        | `libvterm0`      | `libwoff1`             |
| `wayfire`                 | `labwc`              | `libseat1`       | `libwf-config1`        |
| `libwpebackend-fdo-1.0-1` | `wlr-randr`          | `libaml0t64`     | `libsfdo0`             |
| `libwf-utils0t64`         | `libwpewebkit-2.0-1` | `xdg-dbus-proxy` | `libfreerdp-server3-3` |
| `libturbojpeg0`           | `libwinpr3-3`        | `libxcb-errors0` | `libfreerdp3-3`        |
| `liburiparser1`           | `libwlroots-0.18`    | `libxcb-ewmh2`   |                        |

---

## 🌈 Customization (Themes)

Horizon supports dynamic theming via JSON configuration. You can customize the look and feel by editing your theme at `~/.config/horizon/color-scheme.json`.

```json
{
    "variant": "light",
    "colors": {
        "light": {
            "background": "#F5F5F7",
            "foreground": "#1D1D1F",
            "primary": "#007AFF",
            "accent": "#FF9500"
        }
    },
    "fonts": {
        "window": { "family": "Inter", "size": 13, "weight": "normal" }
    }
}
```

---

<p align="center">
  Built with ❤️ for the <a href="https://github.com/austral-os">Austral Project</a>.
</p>
