#include <horizon/capture/VideoRecorder.h>
#include <horizon/Logger.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>
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
#include <cmath>

// PipeWire
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/debug/types.h>

namespace horizon::capture {

struct RawFrame {
    uint8_t* data;
    size_t size;
    uint32_t width, height, stride, format;
};

struct RawAudio {
    float* data;
    size_t n_samples;
};

struct VideoRecorder::Impl {
    struct wl_display* display = nullptr;
    struct wl_registry* registry = nullptr;
    struct wl_shm* shm = nullptr;
    struct zwlr_screencopy_manager_v1* screencopy_manager = nullptr;
    struct wl_output* target_output = nullptr;

    // FFmpeg
    AVFormatContext* fmt_ctx = nullptr;
    AVCodecContext* video_codec_ctx = nullptr;
    AVCodecContext* audio_codec_ctx = nullptr;
    AVStream* video_stream = nullptr;
    AVStream* audio_stream = nullptr;
    AVFrame* video_frame = nullptr;
    AVFrame* audio_frame = nullptr;
    AVPacket* video_pkt = nullptr;
    AVPacket* audio_pkt = nullptr;
    struct SwsContext* sws_ctx = nullptr;
    struct SwrContext* swr_ctx = nullptr;
    AVAudioFifo* audio_fifo = nullptr;

    std::atomic<int64_t> next_video_pts{0};
    std::atomic<int64_t> next_audio_pts{0};

    // Threads
    std::atomic<bool> running{false};
    std::thread capture_thread;
    std::thread encode_thread;
    std::thread audio_thread;
    
    // Queues
    std::queue<RawFrame> video_queue;
    std::queue<RawAudio> audio_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cond;

    int x, y, width, height, fps;
    bool done = false;
    bool record_audio = false;

    // Wayland Buffer
    void* buffer_data = nullptr;
    size_t buffer_size = 0;
    uint32_t b_width, b_height, b_stride, b_format;

    // PipeWire
    struct pw_main_loop* pw_loop = nullptr;
    struct pw_stream* pw_stream = nullptr;
    std::atomic<size_t> total_audio_received{0};
    std::atomic<float> audio_peak{0.0f};

    void cleanup() {
        if (!running) return;
        running = false;
        queue_cond.notify_all();

        if (pw_loop) pw_main_loop_quit(pw_loop);
        
        if (capture_thread.joinable()) capture_thread.join();
        if (audio_thread.joinable()) audio_thread.join();
        if (encode_thread.joinable()) encode_thread.join();

        if (display) {
            wl_display_disconnect(display);
            display = nullptr;
        }

        if (fmt_ctx) {
            LOG_INFO << "[VideoRecorder] Flushing encoders...";
            if (video_codec_ctx) {
                avcodec_send_frame(video_codec_ctx, nullptr);
                while (avcodec_receive_packet(video_codec_ctx, video_pkt) >= 0) {
                    av_packet_rescale_ts(video_pkt, video_codec_ctx->time_base, video_stream->time_base);
                    video_pkt->stream_index = video_stream->index;
                    av_interleaved_write_frame(fmt_ctx, video_pkt);
                    av_packet_unref(video_pkt);
                }
            }
            if (audio_codec_ctx) {
                avcodec_send_frame(audio_codec_ctx, nullptr);
                while (avcodec_receive_packet(audio_codec_ctx, audio_pkt) >= 0) {
                    av_packet_rescale_ts(audio_pkt, audio_codec_ctx->time_base, audio_stream->time_base);
                    audio_pkt->stream_index = audio_stream->index;
                    av_interleaved_write_frame(fmt_ctx, audio_pkt);
                    av_packet_unref(audio_pkt);
                }
            }
            av_write_trailer(fmt_ctx);
        }

        if (video_codec_ctx) avcodec_free_context(&video_codec_ctx);
        if (audio_codec_ctx) avcodec_free_context(&audio_codec_ctx);
        
        if (fmt_ctx) {
            if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&fmt_ctx->pb);
            avformat_free_context(fmt_ctx);
            fmt_ctx = nullptr;
        }

        if (video_frame) av_frame_free(&video_frame);
        if (audio_frame) av_frame_free(&audio_frame);
        if (video_pkt) av_packet_free(&video_pkt);
        if (audio_pkt) av_packet_free(&audio_pkt);
        if (sws_ctx) sws_freeContext(sws_ctx);
        if (swr_ctx) swr_free(&swr_ctx);
        if (audio_fifo) av_audio_fifo_free(audio_fifo);
        if (pw_stream) pw_stream_destroy(pw_stream);
        if (pw_loop) pw_main_loop_destroy(pw_loop);
        
        pw_stream = nullptr; pw_loop = nullptr;
    }

    AVPixelFormat get_ffmpeg_format(uint32_t wl_format) {
        switch (wl_format) {
            case 0: return AV_PIX_FMT_BGRA;
            case 1: return AV_PIX_FMT_BGRA;
            case 875709016: return AV_PIX_FMT_RGBA;
            case 875708754: return AV_PIX_FMT_RGBA;
            default: return AV_PIX_FMT_RGBA;
        }
    }

    static int create_shm_file(off_t size) {
        int fd = memfd_create("horizon-video-shm", MFD_CLOEXEC);
        if (fd < 0) return -1;
        if (ftruncate(fd, size) < 0) { close(fd); return -1; }
        return fd;
    }

    static void on_pw_stream_process(void* data) {
        auto* impl = static_cast<Impl*>(data);
        struct pw_buffer* b;
        if ((b = pw_stream_dequeue_buffer(impl->pw_stream)) == nullptr) return;

        struct spa_buffer* buf = b->buffer;
        float* samples = (float*)buf->datas[0].data;
        if (samples) {
            size_t n_samples = buf->datas[0].chunk->size / sizeof(float);
            if (n_samples > 0) {
                impl->total_audio_received += n_samples;
                RawAudio audio; audio.n_samples = n_samples;
                audio.data = new float[n_samples];
                memcpy(audio.data, samples, n_samples * sizeof(float));
                
                float p = impl->audio_peak;
                for (size_t i=0; i<n_samples; ++i) {
                    float v = std::abs(audio.data[i]);
                    if (v > p) p = v;
                }
                impl->audio_peak = p;

                { std::lock_guard<std::mutex> lock(impl->queue_mutex); impl->audio_queue.push(audio); }
                impl->queue_cond.notify_one();
            }
        }
        pw_stream_queue_buffer(impl->pw_stream, b);
    }
};

VideoRecorder::VideoRecorder() : m_impl(std::make_unique<Impl>()) {
    pw_init(nullptr, nullptr);
}

VideoRecorder::~VideoRecorder() {
    stop();
    pw_deinit();
}

bool VideoRecorder::is_recording() const { return m_impl->running; }

bool VideoRecorder::start(const std::string& output_file, int x, int y, int width, int height, int fps, bool record_audio) {
    if (m_impl->running) return false;

    m_impl->x = x; m_impl->y = y; m_impl->width = width; m_impl->height = height; m_impl->fps = fps;
    m_impl->record_audio = record_audio; m_impl->next_video_pts = 0; m_impl->next_audio_pts = 0;
    m_impl->total_audio_received = 0; m_impl->audio_peak = 0.0f;

    // Wayland
    m_impl->display = wl_display_connect(nullptr);
    if (!m_impl->display) return false;
    m_impl->registry = wl_display_get_registry(m_impl->display);
    static const struct wl_registry_listener reg_l = {
        [](void* data, struct wl_registry* reg, uint32_t name, const char* interface, uint32_t version) {
            auto* impl = (Impl*)data;
            if (strcmp(interface, "wl_shm") == 0) impl->shm = (wl_shm*)wl_registry_bind(reg, name, &wl_shm_interface, 1);
            else if (strcmp(interface, "zwlr_screencopy_manager_v1") == 0) impl->screencopy_manager = (zwlr_screencopy_manager_v1*)wl_registry_bind(reg, name, &zwlr_screencopy_manager_v1_interface, 3);
            else if (strcmp(interface, "wl_output") == 0 && !impl->target_output) impl->target_output = (wl_output*)wl_registry_bind(reg, name, &wl_output_interface, 1);
        },
        [](void*, struct wl_registry*, uint32_t) {}
    };
    wl_registry_add_listener(m_impl->registry, &reg_l, m_impl.get());
    wl_display_roundtrip(m_impl->display); wl_display_roundtrip(m_impl->display);

    // FFmpeg
    avformat_alloc_output_context2(&m_impl->fmt_ctx, nullptr, nullptr, output_file.c_str());
    
    // Video
    const AVCodec* v_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    m_impl->video_stream = avformat_new_stream(m_impl->fmt_ctx, v_codec);
    m_impl->video_codec_ctx = avcodec_alloc_context3(v_codec);
    m_impl->video_codec_ctx->width = width; m_impl->video_codec_ctx->height = height;
    m_impl->video_codec_ctx->time_base = {1, fps}; m_impl->video_codec_ctx->framerate = {fps, 1};
    m_impl->video_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_impl->video_codec_ctx->gop_size = 30; m_impl->video_codec_ctx->max_b_frames = 0;
    av_opt_set(m_impl->video_codec_ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(m_impl->video_codec_ctx->priv_data, "tune", "zerolatency", 0);
    if (m_impl->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) m_impl->video_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    avcodec_open2(m_impl->video_codec_ctx, v_codec, nullptr);
    avcodec_parameters_from_context(m_impl->video_stream->codecpar, m_impl->video_codec_ctx);
    m_impl->video_stream->time_base = m_impl->video_codec_ctx->time_base;

    // Audio
    if (m_impl->record_audio) {
        const AVCodec* a_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
        m_impl->audio_stream = avformat_new_stream(m_impl->fmt_ctx, a_codec);
        m_impl->audio_codec_ctx = avcodec_alloc_context3(a_codec);
        m_impl->audio_codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
        m_impl->audio_codec_ctx->sample_rate = 44100;
        AVChannelLayout out_layout = AV_CHANNEL_LAYOUT_STEREO;
        av_channel_layout_copy(&m_impl->audio_codec_ctx->ch_layout, &out_layout);
        m_impl->audio_codec_ctx->time_base = {1, 44100};
        m_impl->audio_codec_ctx->bit_rate = 128000;
        if (m_impl->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) m_impl->audio_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        avcodec_open2(m_impl->audio_codec_ctx, a_codec, nullptr);
        avcodec_parameters_from_context(m_impl->audio_stream->codecpar, m_impl->audio_codec_ctx);
        m_impl->audio_stream->time_base = m_impl->audio_codec_ctx->time_base;
        
        m_impl->audio_fifo = av_audio_fifo_alloc(m_impl->audio_codec_ctx->sample_fmt, 2, 44100);
        m_impl->audio_frame = av_frame_alloc();
        m_impl->audio_frame->nb_samples = m_impl->audio_codec_ctx->frame_size;
        m_impl->audio_frame->format = m_impl->audio_codec_ctx->sample_fmt;
        m_impl->audio_frame->sample_rate = 44100;
        av_channel_layout_copy(&m_impl->audio_frame->ch_layout, &m_impl->audio_codec_ctx->ch_layout);
        av_frame_get_buffer(m_impl->audio_frame, 0);
        m_impl->audio_pkt = av_packet_alloc();

        // Swr Setup
        AVChannelLayout in_layout = AV_CHANNEL_LAYOUT_STEREO;
        swr_alloc_set_opts2(&m_impl->swr_ctx, &out_layout, AV_SAMPLE_FMT_FLTP, 44100,
                                              &in_layout, AV_SAMPLE_FMT_FLT, 44100, 0, nullptr);
        swr_init(m_impl->swr_ctx);
    }

    if (!(m_impl->fmt_ctx->oformat->flags & AVFMT_NOFILE)) avio_open(&m_impl->fmt_ctx->pb, output_file.c_str(), AVIO_FLAG_WRITE);
    avformat_write_header(m_impl->fmt_ctx, nullptr);

    m_impl->video_frame = av_frame_alloc();
    m_impl->video_frame->format = AV_PIX_FMT_YUV420P;
    m_impl->video_frame->width = width; m_impl->video_frame->height = height;
    av_frame_get_buffer(m_impl->video_frame, 0);
    m_impl->video_pkt = av_packet_alloc();

    m_impl->running = true;

    // PipeWire
    if (m_impl->record_audio) {
        m_impl->pw_loop = pw_main_loop_new(nullptr);
        static const struct pw_stream_events st_e = { 
            .version = PW_VERSION_STREAM_EVENTS, 
            .state_changed = [](void*, enum pw_stream_state old, enum pw_stream_state state, const char* err) {
                LOG_INFO << "[PipeWire] State: " << pw_stream_state_as_string(old) << " -> " << pw_stream_state_as_string(state);
            },
            .process = Impl::on_pw_stream_process 
        };
        struct pw_properties* props = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Capture", PW_KEY_MEDIA_ROLE, "ScreenRecording", "stream.capture.sink", "true", nullptr);
        m_impl->pw_stream = pw_stream_new_simple(pw_main_loop_get_loop(m_impl->pw_loop), "horizon-capture", props, &st_e, m_impl.get());
        uint8_t buffer[1024]; struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
        struct spa_audio_info_raw info = SPA_AUDIO_INFO_RAW_INIT(.format = SPA_AUDIO_FORMAT_F32, .rate = 44100, .channels = 2);
        const struct spa_pod* params[1] = { spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info) };
        pw_stream_connect(m_impl->pw_stream, PW_DIRECTION_INPUT, PW_ID_ANY, (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_RT_PROCESS), params, 1);
        m_impl->audio_thread = std::thread([this]() { pw_main_loop_run(m_impl->pw_loop); });
    }

    // Encoder
    m_impl->encode_thread = std::thread([this]() {
        while (m_impl->running || !m_impl->video_queue.empty() || !m_impl->audio_queue.empty()) {
            bool worked = false;
            RawFrame rf; bool has_v = false;
            { std::lock_guard<std::mutex> l(m_impl->queue_mutex); if (!m_impl->video_queue.empty()) { rf = m_impl->video_queue.front(); m_impl->video_queue.pop(); has_v = true; } }
            if (has_v) {
                if (!m_impl->sws_ctx) m_impl->sws_ctx = sws_getContext(rf.width, rf.height, m_impl->get_ffmpeg_format(rf.format), m_impl->width, m_impl->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);
                const uint8_t* src_d[4] = {rf.data, 0, 0, 0}; int src_s[4] = {(int)rf.stride, 0, 0, 0};
                sws_scale(m_impl->sws_ctx, src_d, src_s, 0, rf.height, m_impl->video_frame->data, m_impl->video_frame->linesize);
                m_impl->video_frame->pts = m_impl->next_video_pts++;
                if (avcodec_send_frame(m_impl->video_codec_ctx, m_impl->video_frame) >= 0) {
                    while (avcodec_receive_packet(m_impl->video_codec_ctx, m_impl->video_pkt) >= 0) {
                        av_packet_rescale_ts(m_impl->video_pkt, m_impl->video_codec_ctx->time_base, m_impl->video_stream->time_base);
                        m_impl->video_pkt->stream_index = m_impl->video_stream->index;
                        av_interleaved_write_frame(m_impl->fmt_ctx, m_impl->video_pkt);
                        av_packet_unref(m_impl->video_pkt);
                    }
                }
                delete[] rf.data; worked = true;
            }
            RawAudio ra; bool has_a = false;
            { std::lock_guard<std::mutex> l(m_impl->queue_mutex); if (!m_impl->audio_queue.empty()) { ra = m_impl->audio_queue.front(); m_impl->audio_queue.pop(); has_a = true; } }
            if (has_a) {
                int spc = ra.n_samples / 2;
                // Boost for testing
                for (size_t i=0; i<ra.n_samples; ++i) ra.data[i] *= 2.0f;

                uint8_t* out[2];
                int out_samples = swr_get_out_samples(m_impl->swr_ctx, spc);
                av_samples_alloc(out, nullptr, 2, out_samples, AV_SAMPLE_FMT_FLTP, 0);
                int converted = swr_convert(m_impl->swr_ctx, out, out_samples, (const uint8_t**)&ra.data, spc);
                
                av_audio_fifo_write(m_impl->audio_fifo, (void**)out, converted);
                av_freep(&out[0]);
                
                while (av_audio_fifo_size(m_impl->audio_fifo) >= m_impl->audio_codec_ctx->frame_size) {
                    av_audio_fifo_read(m_impl->audio_fifo, (void**)m_impl->audio_frame->data, m_impl->audio_codec_ctx->frame_size);
                    m_impl->audio_frame->pts = m_impl->next_audio_pts; m_impl->next_audio_pts += m_impl->audio_frame->nb_samples;
                    if (avcodec_send_frame(m_impl->audio_codec_ctx, m_impl->audio_frame) >= 0) {
                        while (avcodec_receive_packet(m_impl->audio_codec_ctx, m_impl->audio_pkt) >= 0) {
                            av_packet_rescale_ts(m_impl->audio_pkt, m_impl->audio_codec_ctx->time_base, m_impl->audio_stream->time_base);
                            m_impl->audio_pkt->stream_index = m_impl->audio_stream->index;
                            av_interleaved_write_frame(m_impl->fmt_ctx, m_impl->audio_pkt);
                            av_packet_unref(m_impl->audio_pkt);
                        }
                    }
                }
                delete[] ra.data; worked = true;
            }
            if (!worked) { std::unique_lock<std::mutex> l(m_impl->queue_mutex); m_impl->queue_cond.wait_for(l, std::chrono::milliseconds(10)); }
        }
    });

    // Capture
    m_impl->capture_thread = std::thread([this]() {
        while (m_impl->running) {
            m_impl->done = false;
            auto* fr = zwlr_screencopy_manager_v1_capture_output_region(m_impl->screencopy_manager, 1, m_impl->target_output, m_impl->x, m_impl->y, m_impl->width, m_impl->height);
            static const struct zwlr_screencopy_frame_v1_listener fl = {
                .buffer = [](void* d, struct zwlr_screencopy_frame_v1*, uint32_t f, uint32_t w, uint32_t h, uint32_t s) { auto* i=(Impl*)d; i->b_format=f; i->b_width=w; i->b_height=h; i->b_stride=s; },
                .flags = [](void*, struct zwlr_screencopy_frame_v1*, uint32_t) {},
                .ready = [](void* d, struct zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t) { ((Impl*)d)->done=true; },
                .failed = [](void* d, struct zwlr_screencopy_frame_v1*) { ((Impl*)d)->done=true; },
                .damage = [](void*, struct zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t, uint32_t) {},
                .linux_dmabuf = [](void*, struct zwlr_screencopy_frame_v1*, uint32_t, uint32_t, uint32_t) {},
                .buffer_done = [](void* d, struct zwlr_screencopy_frame_v1* fr) {
                    auto* i=(Impl*)d; i->buffer_size = i->b_stride * i->b_height;
                    int fd = Impl::create_shm_file(i->buffer_size);
                    i->buffer_data = mmap(0, i->buffer_size, PROT_READ, MAP_SHARED, fd, 0);
                    auto* pool = wl_shm_create_pool(i->shm, fd, i->buffer_size);
                    auto* buf = wl_shm_pool_create_buffer(pool, 0, i->b_width, i->b_height, i->b_stride, i->b_format);
                    wl_shm_pool_destroy(pool); close(fd);
                    zwlr_screencopy_frame_v1_copy(fr, buf);
                }
            };
            zwlr_screencopy_frame_v1_add_listener(fr, &fl, m_impl.get());
            while (m_impl->running && !m_impl->done) {
                struct pollfd pfd = { wl_display_get_fd(m_impl->display), POLLIN, 0 };
                wl_display_flush(m_impl->display);
                if (poll(&pfd, 1, 100) > 0) wl_display_dispatch(m_impl->display);
            }
            if (m_impl->buffer_data) {
                RawFrame f; f.width=m_impl->b_width; f.height=m_impl->b_height; f.stride=m_impl->b_stride; f.format=m_impl->b_format; f.size=m_impl->buffer_size;
                f.data = new uint8_t[f.size]; memcpy(f.data, m_impl->buffer_data, f.size);
                munmap(m_impl->buffer_data, m_impl->buffer_size); m_impl->buffer_data=nullptr;
                { std::lock_guard<std::mutex> l(m_impl->queue_mutex); m_impl->video_queue.push(f); }
                m_impl->queue_cond.notify_one();
            }
            zwlr_screencopy_frame_v1_destroy(fr);
            usleep(1000000 / m_impl->fps);
        }
    });

    return true;
}

void VideoRecorder::stop() { if (!m_impl->running) return; m_impl->cleanup(); }

} // namespace horizon::capture
