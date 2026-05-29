#include <horizon/lens/ThumbnailCache.hpp>
#include <horizon/Logger.hpp>
#include <horizon/dbusutils/DbusHelper.hpp>
#include <dbus/dbus.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

// PNG chunk reading (minimal, no libpng dependency)
#include <cstdint>
#include <sys/stat.h>

namespace fs = std::filesystem;

namespace horizon::lens
{

// ---------------------------------------------------------------------------
// MD5 implementation (public domain, RFC 1321 based)
// We avoid OpenSSL/libssl to keep the dependency minimal.
// ---------------------------------------------------------------------------
namespace detail
{
    struct MD5Context {
        uint32_t lo, hi;
        uint32_t a, b, c, d;
        uint8_t  buffer[64];
        uint32_t block[16];
    };

    static const uint8_t* md5_body(MD5Context& ctx, const uint8_t* data, size_t size);

    static void md5_init(MD5Context& ctx)
    {
        ctx.a = 0x67452301; ctx.b = 0xefcdab89;
        ctx.c = 0x98badcfe; ctx.d = 0x10325476;
        ctx.lo = ctx.hi = 0;
    }

    static void md5_update(MD5Context& ctx, const void* data, size_t size)
    {
        uint32_t saved_lo = ctx.lo;
        if ((ctx.lo = (saved_lo + size) & 0x1fffffff) < saved_lo) ctx.hi++;
        ctx.hi += (uint32_t)(size >> 29);

        size_t used = saved_lo & 0x3f;
        if (used) {
            size_t available = 64 - used;
            if (size < available) {
                std::memcpy(&ctx.buffer[used], data, size);
                return;
            }
            std::memcpy(&ctx.buffer[used], data, available);
            data = (const uint8_t*)data + available;
            size -= available;
            md5_body(ctx, ctx.buffer, 64);
        }
        if (size >= 64) {
            data = md5_body(ctx, (const uint8_t*)data, size & ~(size_t)0x3f);
            size &= 0x3f;
        }
        std::memcpy(ctx.buffer, data, size);
    }

    static void md5_final(MD5Context& ctx, uint8_t result[16])
    {
        size_t used = ctx.lo & 0x3f;
        ctx.buffer[used++] = 0x80;
        size_t available = 64 - used;
        if (available < 8) {
            std::memset(&ctx.buffer[used], 0, available);
            md5_body(ctx, ctx.buffer, 64);
            used = 0;
            available = 64;
        }
        std::memset(&ctx.buffer[used], 0, available - 8);
        ctx.lo <<= 3;
        ctx.buffer[56] = (uint8_t)ctx.lo;
        ctx.buffer[57] = (uint8_t)(ctx.lo >> 8);
        ctx.buffer[58] = (uint8_t)(ctx.lo >> 16);
        ctx.buffer[59] = (uint8_t)(ctx.lo >> 24);
        ctx.buffer[60] = (uint8_t)ctx.hi;
        ctx.buffer[61] = (uint8_t)(ctx.hi >> 8);
        ctx.buffer[62] = (uint8_t)(ctx.hi >> 16);
        ctx.buffer[63] = (uint8_t)(ctx.hi >> 24);
        md5_body(ctx, ctx.buffer, 64);
        result[0]  = (uint8_t)ctx.a; result[1]  = (uint8_t)(ctx.a >> 8);
        result[2]  = (uint8_t)(ctx.a >> 16); result[3]  = (uint8_t)(ctx.a >> 24);
        result[4]  = (uint8_t)ctx.b; result[5]  = (uint8_t)(ctx.b >> 8);
        result[6]  = (uint8_t)(ctx.b >> 16); result[7]  = (uint8_t)(ctx.b >> 24);
        result[8]  = (uint8_t)ctx.c; result[9]  = (uint8_t)(ctx.c >> 8);
        result[10] = (uint8_t)(ctx.c >> 16); result[11] = (uint8_t)(ctx.c >> 24);
        result[12] = (uint8_t)ctx.d; result[13] = (uint8_t)(ctx.d >> 8);
        result[14] = (uint8_t)(ctx.d >> 16); result[15] = (uint8_t)(ctx.d >> 24);
    }

#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) ((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))
#define STEP(f, a, b, c, d, x, t, s) \
    (a) += f((b), (c), (d)) + (x) + (t); \
    (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
    (a) += (b);

    static const uint8_t* md5_body(MD5Context& ctx, const uint8_t* data, size_t size)
    {
        uint32_t a, b, c, d;
        do {
            uint32_t saved_a = a = ctx.a;
            uint32_t saved_b = b = ctx.b;
            uint32_t saved_c = c = ctx.c;
            uint32_t saved_d = d = ctx.d;
            uint32_t* block = ctx.block;
            for (int i = 0; i < 16; i++) {
                block[i] = (uint32_t)data[i*4] | ((uint32_t)data[i*4+1] << 8) |
                           ((uint32_t)data[i*4+2] << 16) | ((uint32_t)data[i*4+3] << 24);
            }
            STEP(F, a, b, c, d, block[0],  0xd76aa478,  7)
            STEP(F, d, a, b, c, block[1],  0xe8c7b756, 12)
            STEP(F, c, d, a, b, block[2],  0x242070db, 17)
            STEP(F, b, c, d, a, block[3],  0xc1bdceee, 22)
            STEP(F, a, b, c, d, block[4],  0xf57c0faf,  7)
            STEP(F, d, a, b, c, block[5],  0x4787c62a, 12)
            STEP(F, c, d, a, b, block[6],  0xa8304613, 17)
            STEP(F, b, c, d, a, block[7],  0xfd469501, 22)
            STEP(F, a, b, c, d, block[8],  0x698098d8,  7)
            STEP(F, d, a, b, c, block[9],  0x8b44f7af, 12)
            STEP(F, c, d, a, b, block[10], 0xffff5bb1, 17)
            STEP(F, b, c, d, a, block[11], 0x895cd7be, 22)
            STEP(F, a, b, c, d, block[12], 0x6b901122,  7)
            STEP(F, d, a, b, c, block[13], 0xfd987193, 12)
            STEP(F, c, d, a, b, block[14], 0xa679438e, 17)
            STEP(F, b, c, d, a, block[15], 0x49b40821, 22)
            STEP(G, a, b, c, d, block[1],  0xf61e2562,  5)
            STEP(G, d, a, b, c, block[6],  0xc040b340,  9)
            STEP(G, c, d, a, b, block[11], 0x265e5a51, 14)
            STEP(G, b, c, d, a, block[0],  0xe9b6c7aa, 20)
            STEP(G, a, b, c, d, block[5],  0xd62f105d,  5)
            STEP(G, d, a, b, c, block[10], 0x02441453,  9)
            STEP(G, c, d, a, b, block[15], 0xd8a1e681, 14)
            STEP(G, b, c, d, a, block[4],  0xe7d3fbc8, 20)
            STEP(G, a, b, c, d, block[9],  0x21e1cde6,  5)
            STEP(G, d, a, b, c, block[14], 0xc33707d6,  9)
            STEP(G, c, d, a, b, block[3],  0xf4d50d87, 14)
            STEP(G, b, c, d, a, block[8],  0x455a14ed, 20)
            STEP(G, a, b, c, d, block[13], 0xa9e3e905,  5)
            STEP(G, d, a, b, c, block[2],  0xfcefa3f8,  9)
            STEP(G, c, d, a, b, block[7],  0x676f02d9, 14)
            STEP(G, b, c, d, a, block[12], 0x8d2a4c8a, 20)
            STEP(H, a, b, c, d, block[5],  0xfffa3942,  4)
            STEP(H, d, a, b, c, block[8],  0x8771f681, 11)
            STEP(H, c, d, a, b, block[11], 0x6d9d6122, 16)
            STEP(H, b, c, d, a, block[14], 0xfde5380c, 23)
            STEP(H, a, b, c, d, block[1],  0xa4beea44,  4)
            STEP(H, d, a, b, c, block[4],  0x4bdecfa9, 11)
            STEP(H, c, d, a, b, block[7],  0xf6bb4b60, 16)
            STEP(H, b, c, d, a, block[10], 0xbebfbc70, 23)
            STEP(H, a, b, c, d, block[13], 0x289b7ec6,  4)
            STEP(H, d, a, b, c, block[0],  0xeaa127fa, 11)
            STEP(H, c, d, a, b, block[3],  0xd4ef3085, 16)
            STEP(H, b, c, d, a, block[6],  0x04881d05, 23)
            STEP(H, a, b, c, d, block[9],  0xd9d4d039,  4)
            STEP(H, d, a, b, c, block[12], 0xe6db99e5, 11)
            STEP(H, c, d, a, b, block[15], 0x1fa27cf8, 16)
            STEP(H, b, c, d, a, block[2],  0xc4ac5665, 23)
            STEP(I, a, b, c, d, block[0],  0xf4292244,  6)
            STEP(I, d, a, b, c, block[7],  0x432aff97, 10)
            STEP(I, c, d, a, b, block[14], 0xab9423a7, 15)
            STEP(I, b, c, d, a, block[5],  0xfc93a039, 21)
            STEP(I, a, b, c, d, block[12], 0x655b59c3,  6)
            STEP(I, d, a, b, c, block[3],  0x8f0ccc92, 10)
            STEP(I, c, d, a, b, block[10], 0xffeff47d, 15)
            STEP(I, b, c, d, a, block[1],  0x85845dd1, 21)
            STEP(I, a, b, c, d, block[8],  0x6fa87e4f,  6)
            STEP(I, d, a, b, c, block[15], 0xfe2ce6e0, 10)
            STEP(I, c, d, a, b, block[6],  0xa3014314, 15)
            STEP(I, b, c, d, a, block[13], 0x4e0811a1, 21)
            STEP(I, a, b, c, d, block[4],  0xf7537e82,  6)
            STEP(I, d, a, b, c, block[11], 0xbd3af235, 10)
            STEP(I, c, d, a, b, block[2],  0x2ad7d2bb, 15)
            STEP(I, b, c, d, a, block[9],  0xeb86d391, 21)
            ctx.a += a; ctx.b += b; ctx.c += c; ctx.d += d;
            data += 64;
        } while (size -= 64);
        return data;
    }
#undef F
#undef G
#undef H
#undef I
#undef STEP

    static std::string md5_hex(const std::string& input)
    {
        MD5Context ctx;
        md5_init(ctx);
        md5_update(ctx, input.data(), input.size());
        uint8_t result[16];
        md5_final(ctx, result);
        std::ostringstream ss;
        for (int i = 0; i < 16; i++)
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
        return ss.str();
    }

    // -----------------------------------------------------------------------
    // Minimal PNG tEXt chunk reader — reads Thumb::MTime without libpng
    // -----------------------------------------------------------------------
    static std::string read_png_text_chunk(const std::string& png_path, const std::string& key)
    {
        std::ifstream f(png_path, std::ios::binary);
        if (!f) return "";

        // Skip PNG signature (8 bytes)
        f.seekg(8);

        while (f) {
            uint8_t len_buf[4];
            if (!f.read((char*)len_buf, 4)) break;
            uint32_t chunk_len = ((uint32_t)len_buf[0] << 24) | ((uint32_t)len_buf[1] << 16) |
                                 ((uint32_t)len_buf[2] << 8)  |  (uint32_t)len_buf[3];

            char type[5] = {};
            if (!f.read(type, 4)) break;

            if (std::string(type) == "tEXt" && chunk_len > 0 && chunk_len < 8192) {
                std::vector<char> data(chunk_len);
                if (!f.read(data.data(), chunk_len)) break;
                // tEXt: keyword \0 text
                std::string kw(data.data());
                if (kw == key) {
                    std::string value(data.data() + kw.size() + 1,
                                     chunk_len - kw.size() - 1);
                    return value;
                }
                f.seekg(4, std::ios::cur); // skip CRC
            } else {
                // Skip data + CRC
                f.seekg((std::streamoff)chunk_len + 4, std::ios::cur);
            }

            if (std::string(type) == "IEND") break;
        }
        return "";
    }

} // namespace detail

// ---------------------------------------------------------------------------
// ThumbnailCache implementation
// ---------------------------------------------------------------------------

static std::string size_subdir(ThumbnailSize size)
{
    switch (size) {
        case ThumbnailSize::Normal:  return "normal";
        case ThumbnailSize::Large:   return "large";
        case ThumbnailSize::XLarge:  return "x-large";
    }
    return "large";
}

static std::string home_dir()
{
    const char* h = std::getenv("HOME");
    return h ? h : "/tmp";
}

// static
std::string ThumbnailCache::compute_uri_md5(const std::string& file_path)
{
    std::string uri = "file://" + file_path;
    return detail::md5_hex(uri);
}

// static
std::string ThumbnailCache::cache_dir(ThumbnailSize size)
{
    fs::path base = fs::path(home_dir()) / ".cache" / "horizon" / "thumbnails" / size_subdir(size);
    return base.string();
}

// static
std::string ThumbnailCache::thumbnail_path_for(const std::string& file_path, ThumbnailSize size)
{
    std::string md5 = compute_uri_md5(file_path);
    fs::path thumb = fs::path(cache_dir(size)) / (md5 + ".png");
    return thumb.string();
}

// static
bool ThumbnailCache::is_supported(const std::string& file_path)
{
    std::string ext = fs::path(file_path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    static const std::array<std::string, 10> supported = {
        ".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp", ".svg", ".pdf", ".tiff", ".tif"
    };
    for (const auto& s : supported)
        if (ext == s) return true;
    return false;
}

// static
std::string ThumbnailCache::get_thumbnail(const std::string& file_path, ThumbnailSize size)
{
    if (file_path.empty() || !fs::exists(file_path))
        return "";

    std::string thumb_path = thumbnail_path_for(file_path, size);

    if (!fs::exists(thumb_path))
        return "";

    // Validate freshness via Thumb::MTime metadata embedded in the PNG
    std::string mtime_str = detail::read_png_text_chunk(thumb_path, "Thumb::MTime");
    if (!mtime_str.empty()) {
        try {
            long long thumb_mtime = std::stoll(mtime_str);
            struct stat st;
            if (stat(file_path.c_str(), &st) == 0) {
                if (st.st_mtime != thumb_mtime) {
                    // File changed — thumbnail is stale
                    fs::remove(thumb_path);
                    return "";
                }
            }
        } catch (...) {}
    }

    return thumb_path;
}

// static
void ThumbnailCache::request_thumbnail(const std::string& file_path, ThumbnailSize size)
{
    if (file_path.empty()) return;

    try {
        std::string md5 = compute_uri_md5(file_path);
        fs::path fail_path = fs::path(cache_dir(size)) / (md5 + ".failed");
        
        struct stat st_file;
        if (stat(file_path.c_str(), &st_file) == 0) {
            struct stat st_fail;
            if (stat(fail_path.string().c_str(), &st_fail) == 0) {
                if (st_fail.st_mtime >= st_file.st_mtime) {
                    // Ya intentamos y fallamos para esta versión del archivo, no reintentar
                    return;
                }
            }
        }

        // Enviar la petición por D-Bus a horizon-lens de forma asíncrona
        dbusutils::DbusHelper dbus(DBUS_BUS_SESSION);
        DBusMessage* msg = dbus_message_new_method_call(
            "org.horizon.Lens",
            "/org/horizon/Lens",
            "org.horizon.Lens.Thumbnailer",
            "RequestThumbnail"
        );
        
        if (msg) {
            const char* path_cstr = file_path.c_str();
            dbus_message_append_args(msg, DBUS_TYPE_STRING, &path_cstr, DBUS_TYPE_INVALID);
            
            // Enviar sin esperar respuesta (fire and forget)
            dbus_connection_send(dbus.get_connection(), msg, nullptr);
            dbus_connection_flush(dbus.get_connection());
            dbus_message_unref(msg);
        }
    } catch (const std::exception& e) {
        LOG_WARNING << "ThumbnailCache::request_thumbnail D-Bus failed: " << e.what();
    }
}

// static
void ThumbnailCache::invalidate(const std::string& file_path, ThumbnailSize size)
{
    std::string thumb_path = thumbnail_path_for(file_path, size);
    try {
        if (fs::exists(thumb_path))
            fs::remove(thumb_path);
    } catch (...) {}
}

} // namespace horizon::lens
