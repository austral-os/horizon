#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <cairo/cairo.h>
#include <linux/input-event-codes.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

struct App {
    wl_display* display{};
    wl_registry* registry{};
    wl_compositor* compositor{};
    wl_shm* shm{};
    wl_seat* seat{};
    wl_pointer* pointer{};
    xdg_wm_base* wm_base{};

    wl_surface* surface{};
    xdg_surface* xdg_surface_obj{};
    xdg_toplevel* toplevel{};

    wl_surface* popup_surface{};
    xdg_surface* popup_xdg_surface{};
    xdg_popup* popup{};

    int mouse_x{};
    int mouse_y{};
    uint32_t serial{};
} app;

int width = 400;
int height = 300;

void* shm_data{};
wl_buffer* buffer{};

int create_shm_file(size_t size)
{
    char name[]="/tmp/wayland-shm-XXXXXX";
    int fd = mkstemp(name);
    unlink(name);
    ftruncate(fd,size);
    return fd;
}

wl_buffer* create_buffer(int w,int h,void** data_out)
{
    int stride = w*4;
    int size = stride*h;

    int fd = create_shm_file(size);

    void* data = mmap(nullptr,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    *data_out = data;

    wl_shm_pool* pool = wl_shm_create_pool(app.shm,fd,size);

    wl_buffer* buf =
    wl_shm_pool_create_buffer(pool,0,w,h,stride,WL_SHM_FORMAT_ARGB8888);

    wl_shm_pool_destroy(pool);
    close(fd);

    return buf;
}

void draw_color(void* data,int w,int h,double r,double g,double b)
{
    cairo_surface_t* surface =
    cairo_image_surface_create_for_data(
        (unsigned char*)data,
                                        CAIRO_FORMAT_ARGB32,
                                        w,h,w*4);

    cairo_t* cr = cairo_create(surface);

    cairo_set_source_rgb(cr,r,g,b);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

void redraw()
{
    buffer = create_buffer(width,height,&shm_data);

    draw_color(shm_data,width,height,0.5,0.5,0.5);

    wl_surface_attach(app.surface,buffer,0,0);
    wl_surface_commit(app.surface);
}

static void xdg_surface_configure(
    void*, xdg_surface* surf, uint32_t serial)
{
    xdg_surface_ack_configure(surf,serial);
    redraw();
}

static const xdg_surface_listener surface_listener{
    xdg_surface_configure
};

void draw_popup()
{
    int pw = 120;
    int ph = 80;

    void* data{};
    wl_buffer* buf = create_buffer(pw,ph,&data);

    draw_color(data,pw,ph,0.2,0.2,0.2);

    wl_surface_attach(app.popup_surface,buf,0,0);
    wl_surface_commit(app.popup_surface);
}

static void popup_configure(
    void*, xdg_surface* surf, uint32_t serial)
{
    xdg_surface_ack_configure(surf,serial);
    draw_popup();
}

static const xdg_surface_listener popup_surface_listener{
    popup_configure
};

void open_popup()
{
    auto positioner =
    xdg_wm_base_create_positioner(app.wm_base);

    xdg_positioner_set_size(positioner,120,80);

    xdg_positioner_set_anchor_rect(
        positioner,
        app.mouse_x,
        app.mouse_y,
        1,1);

    xdg_positioner_set_anchor(
        positioner,
        XDG_POSITIONER_ANCHOR_TOP_LEFT);

    xdg_positioner_set_gravity(
        positioner,
        XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);

    app.popup_surface =
    wl_compositor_create_surface(app.compositor);

    app.popup_xdg_surface =
    xdg_wm_base_get_xdg_surface(
        app.wm_base,
        app.popup_surface);

    xdg_surface_add_listener(
        app.popup_xdg_surface,
        &popup_surface_listener,
        nullptr);

    app.popup =
    xdg_surface_get_popup(
        app.popup_xdg_surface,
        app.xdg_surface_obj,
        positioner);

    xdg_popup_grab(app.popup,app.seat,app.serial);

    wl_surface_commit(app.popup_surface);
}

static void pointer_enter(
    void*,wl_pointer*,uint32_t,
    wl_surface*,wl_fixed_t,wl_fixed_t){}

    static void pointer_leave(
        void*,wl_pointer*,uint32_t,wl_surface*){}

        static void pointer_motion(
            void*,wl_pointer*,uint32_t,
            wl_fixed_t x,wl_fixed_t y)
        {
            app.mouse_x = wl_fixed_to_int(x);
            app.mouse_y = wl_fixed_to_int(y);
        }

        static void pointer_button(
            void*,wl_pointer*,uint32_t serial,
            uint32_t,uint32_t button,uint32_t state)
        {
            if(state == WL_POINTER_BUTTON_STATE_PRESSED &&
                button == BTN_RIGHT)
            {
                app.serial = serial;
                open_popup();
            }
        }

        static void pointer_axis(void*,wl_pointer*,uint32_t,uint32_t,wl_fixed_t){}
        static void pointer_frame(void*,wl_pointer*){}
        static void pointer_axis_source(void*,wl_pointer*,uint32_t){}
        static void pointer_axis_stop(void*,wl_pointer*,uint32_t,uint32_t){}
        static void pointer_axis_discrete(void*,wl_pointer*,uint32_t,int32_t){}
        static void pointer_axis_value120(void*,wl_pointer*,uint32_t,int32_t){}

        static const wl_pointer_listener pointer_listener{
            pointer_enter,
            pointer_leave,
            pointer_motion,
            pointer_button,
            pointer_axis,
            pointer_frame,
            pointer_axis_source,
            pointer_axis_stop,
            pointer_axis_discrete,
            pointer_axis_value120
        };

        static void seat_capabilities(
            void*,wl_seat* seat,uint32_t caps)
        {
            if(caps & WL_SEAT_CAPABILITY_POINTER)
            {
                app.pointer = wl_seat_get_pointer(seat);

                wl_pointer_add_listener(
                    app.pointer,
                    &pointer_listener,
                    nullptr);
            }
        }

        static void seat_name(void*,wl_seat*,const char*){}

        static const wl_seat_listener seat_listener{
            seat_capabilities,
            seat_name
        };

        static void wm_base_ping(
            void*,xdg_wm_base* wm,uint32_t serial)
        {
            xdg_wm_base_pong(wm,serial);
        }

        static const xdg_wm_base_listener wm_listener{
            wm_base_ping
        };

        static void registry_add(
            void*,wl_registry* reg,
            uint32_t name,
            const char* interface,
            uint32_t)
        {
            if(strcmp(interface,"wl_compositor")==0)
                app.compositor=(wl_compositor*)
                wl_registry_bind(reg,name,
                                 &wl_compositor_interface,4);

                else if(strcmp(interface,"wl_shm")==0)
                    app.shm=(wl_shm*)
                    wl_registry_bind(reg,name,
                                     &wl_shm_interface,1);

                    else if(strcmp(interface,"wl_seat")==0)
                    {
                        app.seat=(wl_seat*)
                        wl_registry_bind(reg,name,
                                         &wl_seat_interface,5);

                        wl_seat_add_listener(
                            app.seat,
                            &seat_listener,
                            nullptr);
                    }

                    else if(strcmp(interface,"xdg_wm_base")==0)
                    {
                        app.wm_base=(xdg_wm_base*)
                        wl_registry_bind(reg,name,
                                         &xdg_wm_base_interface,1);

                        xdg_wm_base_add_listener(
                            app.wm_base,
                            &wm_listener,
                            nullptr);
                    }
        }

        static void registry_remove(
            void*,wl_registry*,uint32_t){}

            static const wl_registry_listener registry_listener{
                registry_add,
                registry_remove
            };

            void create_window()
            {
                app.surface =
                wl_compositor_create_surface(
                    app.compositor);

                app.xdg_surface_obj =
                xdg_wm_base_get_xdg_surface(
                    app.wm_base,
                    app.surface);

                xdg_surface_add_listener(
                    app.xdg_surface_obj,
                    &surface_listener,
                    nullptr);

                app.toplevel =
                xdg_surface_get_toplevel(
                    app.xdg_surface_obj);

                wl_surface_commit(app.surface);
            }

            int main()
            {
                app.display = wl_display_connect(nullptr);

                app.registry =
                wl_display_get_registry(app.display);

                wl_registry_add_listener(
                    app.registry,
                    &registry_listener,
                    nullptr);

                wl_display_roundtrip(app.display);

                create_window();

                while(wl_display_dispatch(app.display)) {}

                return 0;
            }
