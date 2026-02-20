#include <memory>

#pragma once // Solo se incluye una vez.

struct wl_display;
struct wl_registry;
struct wl_compositor;
struct wl_shm;

namespace horizon
{

    class Window;

    class Application
    {
    public:
        explicit Application();
        ~Application();

        Application(const Application &) = delete;            // Sin soporte para copias.
        Application &operator=(const Application &) = delete; // Sin soporte para asignaciones.

        Application(Application &&) noexcept; // Puede mover sin gastar recursos extra copiando.
        Application &operator=(Application &&) noexcept;

        void run();
        void quit();

        void set_wl_compositor(struct wl_compositor *compositor);
        void set_wl_shm(struct wl_shm *shm);

    private:
        void init_wayland();
        void close_wayland();
        void dispatch_events();

    private:
        struct wl_display *m_display = nullptr;
        struct wl_registry *m_registry = nullptr;
        struct wl_compositor *m_compositor = nullptr;
        struct wl_shm *m_shm = nullptr;

        bool m_is_running = false;

        std::unique_ptr<Window> m_window;
    };
} // namespace horizon