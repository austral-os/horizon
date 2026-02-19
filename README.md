# Horizon

Horizon is a minimal graphical toolkit written in C++ and designed to run on Wayland.  
It is part of the Austral ecosystem and serves as the foundational layer for building native, modern, and fully controlled graphical applications.

---

## Philosophy

- Minimalism over abstraction overload
- Explicit control over memory and resources
- No unnecessary dependencies
- Clear and extensible architecture
- Designed to integrate with Weston during development
- Compatible with the `.app` packaging model of Austral

Horizon aims to be small, auditable, and educational.

---

## Architecture

Horizon is structured in layers:

### 1. Core
- Application lifecycle management
- Main event loop
- Wayland integration
- Input event handling

### 2. Renderer
- Initially based on Cairo
- Surface abstraction
- Buffer management

### 3. Widgets
- Basic components: window, button, label, containers
- Simple layout system

### 4. Runtime
- Support for `.app` distribution format
- Resource and asset management

---

## Dependencies

To build Horizon on Debian (Wayland + Weston environment):

- C++ compiler (g++)
- CMake
- Ninja
- libwayland-dev
- libxkbcommon-dev
- libcairo2-dev
- wayland-protocols
- weston (for testing)

## Installing Development Dependencies (Debian)

```bash
sudo apt install build-essential g++ pkg-config \
                 cmake ninja-build \
                 git \
                 libwayland-dev \
                 wayland-protocols \
                 libxkbcommon-dev \
                 libcairo2-dev \
                 libpixman-1-dev \
                 libegl1-mesa-dev \
                 libgles2-mesa-dev \
                 weston \
                 wayland-utils
```
## Compiling

```bash
mkdir build
cd build
cmake -G Ninja ..
ninja
```
