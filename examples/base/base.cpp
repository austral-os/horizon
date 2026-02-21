#include <cairo/cairo.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include <horizon/xdg-shell-client-protocol.h>

// ----------------------------
// Widget base
class Widget
{
public:
    virtual ~Widget() = default;
    virtual void render(cairo_t *cr) = 0;
};

class RootWidget : public Widget
{
public:
    void render(cairo_t *cr) override
    {
        // Fondo azul claro
        cairo_set_source_rgb(cr, 0.2, 0.6, 0.8);
        cairo_paint(cr);

        // Rectángulo decorativo
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_rectangle(cr, 100, 100, 600, 400);
        cairo_set_line_width(cr, 10.0);
        cairo_stroke(cr);

        std::cout << "Renderizado completado en el buffer." << std::endl;
    }
};

// ----------------------------
class Application
{
public:
    Application()
    {
        init_wayland();
    }

    ~Application()
    {
        if (m_xdg_wm_base)
            xdg_wm_base_destroy(m_xdg_wm_base);
        if (m_compositor)
            wl_compositor_destroy(m_compositor);
        if (m_shm)
            wl_shm_destroy(m_shm);
        if (m_registry)
            wl_registry_destroy(m_registry);
        if (m_display)
            wl_display_disconnect(m_display);
    }

    void set_root(std::unique_ptr<Widget> root)
    {
        m_root = std::move(root);
    }

    void run()
    {
        const int width = 800;
        const int height = 600;

        // 1. Crear Superficie
        wl_surface *surface = wl_compositor_create_surface(m_compositor);

        // 2. Configurar XDG Surface (El "cascarón" de la ventana)
        xdg_surface *xdg_surf = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, surface);
        static const xdg_surface_listener xdg_surf_ptr = {
            .configure = [](void *data, xdg_surface *xdg_s, uint32_t serial)
            { xdg_surface_ack_configure(xdg_s, serial); }};
        xdg_surface_add_listener(xdg_surf, &xdg_surf_ptr, nullptr);

        // 3. Configurar Toplevel (La ventana propiamente dicha)
        xdg_toplevel *toplevel = xdg_surface_get_toplevel(xdg_surf);
        xdg_toplevel_set_title(toplevel, "Cairo Wayland Corrected");

        // IMPORTANTE: Primer commit para que el compositor envíe el evento 'configure'
        wl_surface_commit(surface);
        wl_display_roundtrip(m_display);

        // 4. Crear Buffer SHM
        int stride = width * 4;
        int size = stride * height;
        int fd = memfd_create("buffer", MFD_CLOEXEC);
        ftruncate(fd, size);
        void *data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        wl_shm_pool *pool = wl_shm_create_pool(m_shm, fd, size);
        wl_buffer *buffer =
            wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);

        // 5. Renderizar con Cairo
        cairo_surface_t *cairo_s = cairo_image_surface_create_for_data(
            (unsigned char *)data, CAIRO_FORMAT_ARGB32, width, height, stride);
        cairo_t *cr = cairo_create(cairo_s);

        if (m_root)
            m_root->render(cr);

        cairo_destroy(cr);
        cairo_surface_destroy(cairo_s);

        // 6. Atar buffer y mostrar
        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_damage(surface, 0, 0, width, height);
        wl_surface_commit(surface);

        std::cout << "Ventana mapeada. Bucle de eventos iniciado..." << std::endl;

        while (wl_display_dispatch(m_display) != -1)
        {
            // El dispatch maneja los eventos del sistema
        }
    }

private:
    wl_display *m_display = nullptr;
    wl_registry *m_registry = nullptr;
    wl_compositor *m_compositor = nullptr;
    wl_shm *m_shm = nullptr;
    xdg_wm_base *m_xdg_wm_base = nullptr;
    std::unique_ptr<Widget> m_root;

    void init_wayland()
    {
        m_display = wl_display_connect(nullptr);
        if (!m_display)
            throw std::runtime_error("No Wayland display");

        m_registry = wl_display_get_registry(m_display);
        static const wl_registry_listener reg_list = {
            .global =
                [](void *data, wl_registry *reg, uint32_t id, const char *intf, uint32_t ver)
            {
                auto app = static_cast<Application *>(data);
                if (std::strcmp(intf, "wl_compositor") == 0)
                    app->m_compositor =
                        (wl_compositor *)wl_registry_bind(reg, id, &wl_compositor_interface, 4);
                else if (std::strcmp(intf, "wl_shm") == 0)
                    app->m_shm = (wl_shm *)wl_registry_bind(reg, id, &wl_shm_interface, 1);
                else if (std::strcmp(intf, "xdg_wm_base") == 0)
                {
                    app->m_xdg_wm_base =
                        (xdg_wm_base *)wl_registry_bind(reg, id, &xdg_wm_base_interface, 1);
                    // Listener para responder al PING del servidor
                    static const xdg_wm_base_listener wm_list = {
                        .ping = [](void *, xdg_wm_base *wm, uint32_t ser)
                        { xdg_wm_base_pong(wm, ser); }};
                    xdg_wm_base_add_listener(app->m_xdg_wm_base, &wm_list, nullptr);
                }
            },
            .global_remove = [](void *, wl_registry *, uint32_t) {}};
        wl_registry_add_listener(m_registry, &reg_list, this);
        wl_display_roundtrip(m_display);
    }
};

int main()
{
    try
    {
        Application app;
        app.set_root(std::make_unique<RootWidget>());
        app.run();
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}