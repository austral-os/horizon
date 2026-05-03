#include <horizon/capture/CaptureEngine.h>
#include <horizon/Logger.hpp>
#include <wayland-client.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cairo/cairo.h>
#include <algorithm>
#include <stdexcept>

namespace horizon::capture {

struct OutputInfo {
    struct wl_output* output;
    std::string name;
    uint32_t id;
};

struct CaptureEngine::Impl {
    struct wl_display* display = nullptr;
    struct wl_registry* registry = nullptr;
    struct wl_shm* shm = nullptr;
    struct zwlr_screencopy_manager_v1* screencopy_manager = nullptr;
    std::vector<OutputInfo> outputs;

    bool done = false;
    bool failed = false;

    // Buffer info for current capture
    void* buffer_data = nullptr;
    size_t buffer_size = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride = 0;
    uint32_t format = 0;
    bool y_invert = false;

    ~Impl() {
        if (screencopy_manager) zwlr_screencopy_manager_v1_destroy(screencopy_manager);
        if (shm) wl_shm_destroy(shm);
        if (registry) wl_registry_destroy(registry);
        if (display) wl_display_disconnect(display);
    }

    static void handle_output_name(void* data, struct wl_output* wl_output, const char* name) {
        auto* impl = static_cast<Impl*>(data);
        for (auto& o : impl->outputs) {
            if (o.output == wl_output) {
                o.name = name;
                break;
            }
        }
    }

    static void handle_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
        auto* impl = static_cast<Impl*>(data);
        if (strcmp(interface, "wl_shm") == 0) {
            impl->shm = (wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, 1);
        } else if (strcmp(interface, "zwlr_screencopy_manager_v1") == 0) {
            impl->screencopy_manager = (zwlr_screencopy_manager_v1*)wl_registry_bind(registry, name, &zwlr_screencopy_manager_v1_interface, std::min(version, 3u));
        } else if (strcmp(interface, "wl_output") == 0) {
            struct wl_output* output = (wl_output*)wl_registry_bind(registry, name, &wl_output_interface, 4);
            OutputInfo info;
            info.output = output;
            info.id = name;
            impl->outputs.push_back(info);
            
            static const struct wl_output_listener output_listener = {
                .geometry = [](void*, struct wl_output*, int32_t, int32_t, int32_t, int32_t, int32_t, const char*, const char*, int32_t) {},
                .mode = [](void*, struct wl_output*, uint32_t, int32_t, int32_t, int32_t) {},
                .done = [](void*, struct wl_output*) {},
                .scale = [](void*, struct wl_output*, int32_t) {},
                .name = handle_output_name,
                .description = [](void*, struct wl_output*, const char*) {}
            };
            wl_output_add_listener(output, &output_listener, impl);
        }
    }

    static void handle_global_remove(void*, struct wl_registry*, uint32_t) {}

    static int create_shm_file(off_t size) {
        int fd = memfd_create("horizon-capture-shm", MFD_CLOEXEC);
        if (fd < 0) return -1;
        if (ftruncate(fd, size) < 0) {
            close(fd);
            return -1;
        }
        return fd;
    }
};

CaptureEngine::CaptureEngine() : m_impl(std::make_unique<Impl>()) {}
CaptureEngine::~CaptureEngine() = default;

bool CaptureEngine::init() {
    m_impl->display = wl_display_connect(nullptr);
    if (!m_impl->display) {
        LOG_ERROR << "[CaptureEngine] Failed to connect to Wayland display";
        return false;
    }

    m_impl->registry = wl_display_get_registry(m_impl->display);
    static const struct wl_registry_listener registry_listener = {
        Impl::handle_global,
        Impl::handle_global_remove
    };
    wl_registry_add_listener(m_impl->registry, &registry_listener, m_impl.get());

    wl_display_roundtrip(m_impl->display);
    wl_display_roundtrip(m_impl->display);

    if (!m_impl->screencopy_manager) {
        LOG_ERROR << "[CaptureEngine] Compositor does not support zwlr_screencopy_manager_v1";
        return false;
    }

    return true;
}

bool CaptureEngine::capture_screenshot(const std::string& output_name, const std::string& file_path) {
    return capture_region(output_name, -1, -1, -1, -1, file_path);
}

bool CaptureEngine::capture_region(const std::string& output_name, int x, int y, int width, int height, const std::string& file_path) {
    struct wl_output* target_output = nullptr;
    if (output_name.empty()) {
        if (!m_impl->outputs.empty()) target_output = m_impl->outputs[0].output;
    } else {
        for (const auto& o : m_impl->outputs) {
            if (o.name == output_name) {
                target_output = o.output;
                break;
            }
        }
    }

    if (!target_output) {
        LOG_ERROR << "[CaptureEngine] Target output not found: " << output_name;
        return false;
    }

    struct zwlr_screencopy_frame_v1* frame;
    if (x == -1) {
        LOG_INFO << "[CaptureEngine] Capturing full output";
        frame = zwlr_screencopy_manager_v1_capture_output(m_impl->screencopy_manager, 1, target_output);
    } else {
        if (width <= 0 || height <= 0) {
            LOG_ERROR << "[CaptureEngine] Invalid region size: " << width << "x" << height;
            return false;
        }
        LOG_INFO << "[CaptureEngine] Capturing region: " << x << "," << y << " " << width << "x" << height;
        frame = zwlr_screencopy_manager_v1_capture_output_region(m_impl->screencopy_manager, 1, target_output, x, y, width, height);
    }
    
    m_impl->done = false;
    m_impl->failed = false;
    m_impl->buffer_data = nullptr;

    static const struct zwlr_screencopy_frame_v1_listener frame_listener = {
        .buffer = [](void* data, struct zwlr_screencopy_frame_v1*, uint32_t format, uint32_t width, uint32_t height, uint32_t stride) {
            auto* impl = static_cast<CaptureEngine::Impl*>(data);
            impl->format = format;
            impl->width = width;
            impl->height = height;
            impl->stride = stride;
            LOG_INFO << "[CaptureEngine] Buffer format: " << format << ", size: " << width << "x" << height << ", stride: " << stride;
        },
        .flags = [](void* data, struct zwlr_screencopy_frame_v1*, uint32_t flags) {
            auto* impl = static_cast<CaptureEngine::Impl*>(data);
            impl->y_invert = flags & ZWLR_SCREENCOPY_FRAME_V1_FLAGS_Y_INVERT;
        },
        .ready = [](void* data, struct zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t) {
            auto* impl = static_cast<CaptureEngine::Impl*>(data);
            impl->done = true;
        },
        .failed = [](void* data, struct zwlr_screencopy_frame_v1*) {
            auto* impl = static_cast<CaptureEngine::Impl*>(data);
            impl->failed = true;
            impl->done = true;
        },
        .damage = [](void*, struct zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t, uint32_t) {},
        .linux_dmabuf = [](void*, struct zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t) {},
        .buffer_done = [](void* data, struct zwlr_screencopy_frame_v1* frame) {
            auto* impl = static_cast<CaptureEngine::Impl*>(data);
            impl->buffer_size = impl->stride * impl->height;
            int fd = Impl::create_shm_file(impl->buffer_size);
            if (fd < 0) {
                impl->failed = true;
                impl->done = true;
                return;
            }

            impl->buffer_data = mmap(nullptr, impl->buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (impl->buffer_data == MAP_FAILED) {
                close(fd);
                impl->failed = true;
                impl->done = true;
                return;
            }

            struct wl_shm_pool* pool = wl_shm_create_pool(impl->shm, fd, impl->buffer_size);
            struct wl_buffer* buffer = wl_shm_pool_create_buffer(pool, 0, impl->width, impl->height, impl->stride, impl->format);
            wl_shm_pool_destroy(pool);
            close(fd);

            zwlr_screencopy_frame_v1_copy(frame, buffer);
        }
    };

    zwlr_screencopy_frame_v1_add_listener(frame, &frame_listener, m_impl.get());

    while (!m_impl->done && wl_display_dispatch(m_impl->display) != -1);

    if (m_impl->failed || !m_impl->buffer_data) {
        LOG_ERROR << "[CaptureEngine] Screen capture protocol failed";
        zwlr_screencopy_frame_v1_destroy(frame);
        return false;
    }

    // Handle color format conversion
    // 0 = WL_SHM_FORMAT_XRGB8888, 1 = WL_SHM_FORMAT_ARGB8888
    // If it's something else like BGRX, we might need a swap.
    if (m_impl->format != 0 && m_impl->format != 1) {
        LOG_INFO << "[CaptureEngine] Applying channel swap for format " << m_impl->format;
        unsigned char* data = (unsigned char*)m_impl->buffer_data;
        for (uint32_t j = 0; j < m_impl->height; j++) {
            for (uint32_t i = 0; i < m_impl->width; i++) {
                unsigned char* pixel = data + (j * m_impl->stride) + (i * 4);
                std::swap(pixel[0], pixel[2]);
            }
        }
    }

    cairo_surface_t* surface = cairo_image_surface_create_for_data(
        (unsigned char*)m_impl->buffer_data,
        CAIRO_FORMAT_ARGB32,
        m_impl->width,
        m_impl->height,
        m_impl->stride
    );

    cairo_status_t status = cairo_surface_write_to_png(surface, file_path.c_str());
    cairo_surface_destroy(surface);
    munmap(m_impl->buffer_data, m_impl->buffer_size);
    zwlr_screencopy_frame_v1_destroy(frame);

    if (status != CAIRO_STATUS_SUCCESS) {
        LOG_ERROR << "[CaptureEngine] Failed to write PNG: " << cairo_status_to_string(status);
        return false;
    }

    return true;
}

} // namespace horizon::capture
