#pragma once // Solo se incluye una vez.

namespace horizon
{

    class Application
    {
      public:
        explicit Application();
        ~Application();

        Application(const Application &) = delete;            // Sin soporte para copias.
        Application &operator=(const Application &) = delete; // Sin soporte para asignaciones.
    };
} // namespace horizon