#include <horizon/capture/VideoRecorder.h>
#include <horizon/Logger.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <wayland-client.h>
#include <protocols/wlr-screencopy-unstable-v1-client-protocol.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <algorithm>
#include <poll.h>

namespace horizon::capture {

struct RawFrame {
    uint8_t* data;
    size_t size;
    uint32_t width, height, stride, format;
};

struct VideoRecorder::Impl {
    struct wl_display* display = nullptr;
    struct wl_registry* registry = nullptr;
    struct wl_shm* shm = nullptr;
    struct zwlr_screencopy_manager_v1* screencopy_manager = nullptr;
    struct wl_output* target_output = nullptr;

    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVStream* stream = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;
    struct SwsContext* sws_ctx = nullptr;
    int64_t next_pts = 0;

    std::atomic<bool> running{false};
    std::thread capture_thread;
    std::thread encode_thread;
    
    std::queue<RawFrame> frame_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cond;

    int x, y, width, height, fps;
    bool done = false;

    // Buffer info
    void* buffer_data = nullptr;
    size_t buffer_size = 0;
    uint32_t b_width, b_height, b_stride, b_format;

    ~Impl() {
        cleanup();
    }

    void cleanup() {
        if (display) {
            wl_display_disconnect(display);
            display = nullptr;
        }
    }

    static void handle_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
        auto* impl = static_cast<Impl*>(data);
        if (strcmp(interface, "wl_shm") == 0) {
            impl->shm = (wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, 1);
        } else if (strcmp(interface, "zwlr_screencopy_manager_v1") == 0) {
            impl->screencopy_manager = (zwlr_screencopy_manager_v1*)wl_registry_bind(registry, name, &zwlr_screencopy_manager_v1_interface, std::min(version, 3u));
        } else if (strcmp(interface, "wl_output") == 0) {
            if (!impl->target_output) {
                impl->target_output = (wl_output*)wl_registry_bind(registry, name, &wl_output_interface, 1);
            }
        }
    }

    static void handle_global_remove(void*, struct wl_registry*, uint32_t) {}

    static int create_shm_file(off_t size) {
        int fd = memfd_create("horizon-video-shm", MFD_CLOEXEC);
        if (fd < 0) return -1;
        if (ftruncate(fd, size) < 0) {
            close(fd);
            return -1;
        }
        return fd;
    }

    AVPixelFormat get_ffmpeg_format(uint32_t wl_format) {
        // DRM formats often used by screencopy
        switch (wl_format) {
            case 0: return AV_PIX_FMT_BGRA; // WL_SHM_FORMAT_XRGB8888
            case 1: return AV_PIX_FMT_BGRA; // WL_SHM_FORMAT_ARGB8888
            case 875709016: return AV_PIX_FMT_RGBA; // DRM_FORMAT_XRGB8888 (seems RGBA on this system)
            case 875708754: return AV_PIX_FMT_RGBA; // DRM_FORMAT_ARGB8888
            default: 
                LOG_INFO << "[VideoRecorder] Unknown format: " << wl_format << ", falling back to RGBA";
                return AV_PIX_FMT_RGBA;
        }
    }
};

VideoRecorder::VideoRecorder() : m_impl(std::make_unique<Impl>()) {}
VideoRecorder::~VideoRecorder() {
    stop();
}

bool VideoRecorder::is_recording() const {
    return m_impl->running;
}

bool VideoRecorder::start(const std::string& output_file, int x, int y, int width, int height, int fps) {
    if (m_impl->running) return false;

    m_impl->x = x;
    m_impl->y = y;
    m_impl->width = width;
    m_impl->height = height;
    m_impl->fps = fps;

    // 1. Initialize Wayland
    m_impl->display = wl_display_connect(nullptr);
    if (!m_impl->display) return false;

    m_impl->registry = wl_display_get_registry(m_impl->display);
    static const struct wl_registry_listener registry_listener = { Impl::handle_global, Impl::handle_global_remove };
    wl_registry_add_listener(m_impl->registry, &registry_listener, m_impl.get());
    wl_display_roundtrip(m_impl->display);
    wl_display_roundtrip(m_impl->display);

    if (!m_impl->screencopy_manager || !m_impl->target_output) return false;

    // 2. Initialize FFmpeg
    avformat_alloc_output_context2(&m_impl->fmt_ctx, nullptr, nullptr, output_file.c_str());
    if (!m_impl->fmt_ctx) return false;

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) return false;

    m_impl->stream = avformat_new_stream(m_impl->fmt_ctx, codec);
    m_impl->codec_ctx = avcodec_alloc_context3(codec);
    m_impl->codec_ctx->width = width;
    m_impl->codec_ctx->height = height;
    m_impl->codec_ctx->time_base = {1, fps};
    m_impl->codec_ctx->framerate = {fps, 1};
    m_impl->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_impl->codec_ctx->gop_size = 10;
    m_impl->codec_ctx->max_b_frames = 1;
    
    if (m_impl->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        m_impl->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    av_opt_set(m_impl->codec_ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(m_impl->codec_ctx->priv_data, "tune", "zerolatency", 0);

    if (avcodec_open2(m_impl->codec_ctx, codec, nullptr) < 0) return false;
    avcodec_parameters_from_context(m_impl->stream->codecpar, m_impl->codec_ctx);

    if (!(m_impl->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_impl->fmt_ctx->pb, output_file.c_str(), AVIO_FLAG_WRITE) < 0) return false;
    }

    if (avformat_write_header(m_impl->fmt_ctx, nullptr) < 0) return false;

    m_impl->frame = av_frame_alloc();
    m_impl->frame->format = m_impl->codec_ctx->pix_fmt;
    m_impl->frame->width = width;
    m_impl->frame->height = height;
    av_frame_get_buffer(m_impl->frame, 0);

    m_impl->pkt = av_packet_alloc();

    m_impl->running = true;

    // 3. Start Encoding Thread
    m_impl->encode_thread = std::thread([this]() {
        while (m_impl->running || !m_impl->frame_queue.empty()) {
            RawFrame raw;
            {
                std::unique_lock<std::mutex> lock(m_impl->queue_mutex);
                m_impl->queue_cond.wait_for(lock, std::chrono::milliseconds(100), [this]() { 
                    return !m_impl->frame_queue.empty() || !m_impl->running; 
                });
                if (m_impl->frame_queue.empty() && !m_impl->running) break;
                if (m_impl->frame_queue.empty()) continue;
                
                raw = m_impl->frame_queue.front();
                m_impl->frame_queue.pop();
            }

            // Convert to YUV
            if (!m_impl->sws_ctx) {
                AVPixelFormat src_fmt = m_impl->get_ffmpeg_format(raw.format);
                m_impl->sws_ctx = sws_getContext(raw.width, raw.height, src_fmt,
                                               m_impl->width, m_impl->height, AV_PIX_FMT_YUV420P,
                                               SWS_BILINEAR, nullptr, nullptr, nullptr);
            }

            const uint8_t* src_data[4] = {raw.data, nullptr, nullptr, nullptr};
            int src_linesize[4] = {(int)raw.stride, 0, 0, 0};
            sws_scale(m_impl->sws_ctx, src_data, src_linesize, 0, raw.height, m_impl->frame->data, m_impl->frame->linesize);

            m_impl->frame->pts = m_impl->next_pts++;

            // Encode
            if (avcodec_send_frame(m_impl->codec_ctx, m_impl->frame) >= 0) {
                while (avcodec_receive_packet(m_impl->codec_ctx, m_impl->pkt) >= 0) {
                    av_packet_rescale_ts(m_impl->pkt, m_impl->codec_ctx->time_base, m_impl->stream->time_base);
                    m_impl->pkt->stream_index = m_impl->stream->index;
                    av_interleaved_write_frame(m_impl->fmt_ctx, m_impl->pkt);
                    av_packet_unref(m_impl->pkt);
                }
            }
            delete[] raw.data;
        }
    });

    // 4. Start Capture Thread
    m_impl->capture_thread = std::thread([this]() {
        while (m_impl->running) {
            m_impl->done = false;
            struct zwlr_screencopy_frame_v1* frame_req = zwlr_screencopy_manager_v1_capture_output_region(
                m_impl->screencopy_manager, 1, m_impl->target_output, m_impl->x, m_impl->y, m_impl->width, m_impl->height);

            static const struct zwlr_screencopy_frame_v1_listener frame_listener = {
                .buffer = [](void* data, struct zwlr_screencopy_frame_v1*, uint32_t format, uint32_t width, uint32_t height, uint32_t stride) {
                    auto* impl = static_cast<Impl*>(data);
                    impl->b_format = format;
                    impl->b_width = width;
                    impl->b_height = height;
                    impl->b_stride = stride;
                },
                .flags = [](void*, struct zwlr_screencopy_frame_v1*, uint32_t) {},
                .ready = [](void* data, struct zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t) {
                    static_cast<Impl*>(data)->done = true;
                },
                .failed = [](void* data, struct zwlr_screencopy_frame_v1*) {
                    static_cast<Impl*>(data)->done = true;
                },
                .damage = [](void*, struct zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t, uint32_t) {},
                .linux_dmabuf = [](void*, struct zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t) {},
                .buffer_done = [](void* data, struct zwlr_screencopy_frame_v1* frame_req) {
                    auto* impl = static_cast<Impl*>(data);
                    impl->buffer_size = impl->b_stride * impl->b_height;
                    int fd = Impl::create_shm_file(impl->buffer_size);
                    impl->buffer_data = mmap(nullptr, impl->buffer_size, PROT_READ, MAP_SHARED, fd, 0);
                    
                    struct wl_shm_pool* pool = wl_shm_create_pool(impl->shm, fd, impl->buffer_size);
                    struct wl_buffer* buffer = wl_shm_pool_create_buffer(pool, 0, impl->b_width, impl->b_height, impl->b_stride, impl->b_format);
                    wl_shm_pool_destroy(pool);
                    close(fd);
                    zwlr_screencopy_frame_v1_copy(frame_req, buffer);
                }
            };

            zwlr_screencopy_frame_v1_add_listener(frame_req, &frame_listener, m_impl.get());

            while (m_impl->running && !m_impl->done) {
                struct pollfd pfd = { wl_display_get_fd(m_impl->display), POLLIN, 0 };
                wl_display_flush(m_impl->display);
                if (poll(&pfd, 1, 100) > 0) {
                    wl_display_dispatch(m_impl->display);
                }
            }

            if (m_impl->buffer_data) {
                RawFrame raw;
                raw.width = m_impl->b_width;
                raw.height = m_impl->b_height;
                raw.stride = m_impl->b_stride;
                raw.format = m_impl->b_format;
                raw.size = m_impl->buffer_size;
                raw.data = new uint8_t[raw.size];
                memcpy(raw.data, m_impl->buffer_data, raw.size);
                
                munmap(m_impl->buffer_data, m_impl->buffer_size);
                m_impl->buffer_data = nullptr;

                {
                    std::lock_guard<std::mutex> lock(m_impl->queue_mutex);
                    m_impl->frame_queue.push(raw);
                }
                m_impl->queue_cond.notify_one();
            }

            zwlr_screencopy_frame_v1_destroy(frame_req);
            usleep(1000000 / m_impl->fps);
        }
    });

    return true;
}

void VideoRecorder::stop() {
    if (!m_impl->running) return;
    m_impl->running = false;
    
    m_impl->queue_cond.notify_all();
    if (m_impl->capture_thread.joinable()) m_impl->capture_thread.join();
    if (m_impl->encode_thread.joinable()) m_impl->encode_thread.join();

    if (m_impl->codec_ctx) {
        avcodec_send_frame(m_impl->codec_ctx, nullptr);
        while (avcodec_receive_packet(m_impl->codec_ctx, m_impl->pkt) >= 0) {
            av_interleaved_write_frame(m_impl->fmt_ctx, m_impl->pkt);
            av_packet_unref(m_impl->pkt);
        }
    }

    if (m_impl->fmt_ctx) av_write_trailer(m_impl->fmt_ctx);

    if (m_impl->codec_ctx) avcodec_free_context(&m_impl->codec_ctx);
    if (m_impl->fmt_ctx) {
        if (!(m_impl->fmt_ctx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&m_impl->fmt_ctx->pb);
        avformat_free_context(m_impl->fmt_ctx);
    }
    if (m_impl->frame) av_frame_free(&m_impl->frame);
    if (m_impl->pkt) av_packet_free(&m_impl->pkt);
    if (m_impl->sws_ctx) sws_freeContext(m_impl->sws_ctx);
    
    m_impl->fmt_ctx = nullptr;
    m_impl->codec_ctx = nullptr;
    m_impl->sws_ctx = nullptr;
    m_impl->frame = nullptr;
    m_impl->pkt = nullptr;
}

} // namespace horizon::capture
