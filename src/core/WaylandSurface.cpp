#include <cstring>
#include <horizon/WaylandSurface.hpp>
#include <horizon/xdg-shell-client-protocol.h>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client.h>

namespace horizon
{
    static void registry_global(void *data, wl_registry *registry, uint32_t id,
                                const char *interface, uint32_t version)
    {
        WaylandSurface *ws = static_cast<WaylandSurface *>(data);

        if (strcmp(interface, "wl_compositor") == 0)
        {
            ws->set_wl_compositor(static_cast<wl_compositor *>(
                wl_registry_bind(registry, id, &wl_compositor_interface, 4)));
        }
        else if (strcmp(interface, "wl_shm") == 0)
        {
            ws->set_wl_shm(
                static_cast<wl_shm *>(wl_registry_bind(registry, id, &wl_shm_interface, 1)));
        }
        else if (std::strcmp(interface, "xdg_wm_base") == 0)
        {
            ws->set_xdg_wm_base(static_cast<xdg_wm_base *>(
                wl_registry_bind(registry, id, &xdg_wm_base_interface, 1)));
            // Listener para responder al PING del servidor
            static const xdg_wm_base_listener wm_list = {
                .ping = [](void *, xdg_wm_base *wm, uint32_t ser) { xdg_wm_base_pong(wm, ser); }};
            xdg_wm_base_add_listener(ws->xdg_wm_base(), &wm_list, nullptr);
        }
    }

    static void registry_global_remove(void *data, wl_registry *registry, uint32_t id)
    {
        // not implemented
    }

    WaylandSurface::WaylandSurface(int w, int h) : m_width(w), m_height(h) {}

    int WaylandSurface::width() const
    {
        return m_width;
    }
    int WaylandSurface::height() const
    {
        return m_height;
    }

    WaylandSurface::~WaylandSurface()
    {
        free();
    }

    void WaylandSurface::set_wl_compositor(struct wl_compositor *compositor)
    {
        m_compositor = compositor;
    }

    void WaylandSurface::set_wl_shm(struct wl_shm *shm)
    {
        m_shm = shm;
    }

    void WaylandSurface::set_xdg_wm_base(struct xdg_wm_base *xdg_wm_base)
    {
        m_xdg_wm_base = xdg_wm_base;
    }

    void WaylandSurface::init()
    {

        m_display = wl_display_connect(nullptr);
        if (!m_display)
        {
            throw std::runtime_error("No se pudo conectar al servidor Wayland.");
        }

        m_registry = wl_display_get_registry(m_display);
        if (!m_registry)
        {
            wl_display_disconnect(m_display);
            m_display = nullptr;
            throw std::runtime_error("No se pudo obtener el registro.");
        }

        // Listener del registry
        static const wl_registry_listener listener = {registry_global, registry_global_remove};
        wl_registry_add_listener(m_registry, &listener, this);

        // Roundtrip inicial para que los globals estén disponibles
        wl_display_roundtrip(m_display);

        // 1. Crear Superficie
        m_surface = wl_compositor_create_surface(m_compositor);

        // 2. Configurar XDG Surface (El "cascarón" de la ventana)
        xdg_surface *xdg_surf = xdg_wm_base_get_xdg_surface(m_xdg_wm_base, m_surface);
        static const xdg_surface_listener xdg_surf_ptr = {
            .configure = [](void *data, xdg_surface *xdg_s, uint32_t serial)
            { xdg_surface_ack_configure(xdg_s, serial); }};
        xdg_surface_add_listener(xdg_surf, &xdg_surf_ptr, nullptr);

        // 3. Configurar Toplevel (La ventana propiamente dicha)
        xdg_toplevel *toplevel = xdg_surface_get_toplevel(xdg_surf);
        xdg_toplevel_set_title(toplevel, "Cairo Wayland Corrected");

        // IMPORTANTE: Primer commit para que el compositor envíe el evento 'configure'
        wl_surface_commit(m_surface);
        wl_display_roundtrip(m_display);

        // 4. Crear Buffer SHM
        int stride = m_width * 4;
        int size = stride * m_height;
        int fd = memfd_create("buffer", MFD_CLOEXEC);
        ftruncate(fd, size);
        m_data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        wl_shm_pool *pool = wl_shm_create_pool(m_shm, fd, size);
        m_buffer =
            wl_shm_pool_create_buffer(pool, 0, m_width, m_height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);
    }

    void WaylandSurface::free()
    {

        if (m_data)
        {
            munmap(m_data, m_width * m_height * 4);
            m_data = nullptr;
        }

        if (m_xdg_wm_base)
        {
            xdg_wm_base_destroy(m_xdg_wm_base);
            m_xdg_wm_base = nullptr;
        }
        if (m_compositor)
        {
            wl_compositor_destroy(m_compositor);
            m_compositor = nullptr;
        }
        if (m_shm)
        {
            wl_shm_destroy(m_shm);
            m_shm = nullptr;
        }
        if (m_registry)
        {
            wl_registry_destroy(m_registry);
            m_registry = nullptr;
        }
        if (m_display)
        {
            wl_display_disconnect(m_display);
            m_display = nullptr;
        }
    }

    // Devuelve el puntero a xdg_wm_base
    struct xdg_wm_base *WaylandSurface::xdg_wm_base() const
    {
        return m_xdg_wm_base;
    }

    // Devuelve el puntero al buffer de memoria
    void *WaylandSurface::data() const
    {
        return m_data;
    }

    // Devuelve la superficie Wayland
    struct wl_surface *WaylandSurface::surface() const
    {
        return m_surface;
    }

    // Devuelve el buffer Wayland
    struct wl_buffer *WaylandSurface::buffer() const
    {
        return m_buffer;
    }

    struct wl_display *WaylandSurface::display() const
    {
        return m_display;
    }

} // namespace horizon