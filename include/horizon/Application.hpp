#include <horizon/WaylandSurface.hpp>
#include <memory>

#pragma once // Solo se incluye una vez.

namespace horizon
{

    class Widget;

    class Application
    {
    public:
        explicit Application(int w, int h);
        ~Application();

        Application(const Application &) = delete;            // Sin soporte para copias.
        Application &operator=(const Application &) = delete; // Sin soporte para asignaciones.

        Application(Application &&) noexcept; // Puede mover sin gastar recursos extra copiando.
        Application &operator=(Application &&) noexcept;

        void set_root(std::unique_ptr<Widget> root);

        WaylandSurface *w_surface() const;

        void run();
        void quit();

    private:
        void dispatch_events();

    private:
        std::unique_ptr<WaylandSurface> m_surface;
        bool m_is_running = false;

        std::unique_ptr<Widget> m_root;
    };
} // namespace horizon