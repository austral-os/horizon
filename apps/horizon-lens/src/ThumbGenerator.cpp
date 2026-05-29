#include "ThumbGenerator.hpp"
#include <horizon/Logger.hpp>

#include <cairo/cairo.h>
#include <librsvg/rsvg.h>
#include <poppler.h>
#include <glib.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <vector>

// stb_image for raster formats
#define STB_IMAGE_IMPLEMENTATION
#include <horizon/external/stb_image.h>

namespace fs = std::filesystem;

namespace horizon::lens
{

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::pair<int,int> ThumbGenerator_dims(ThumbnailSize size)
{
    switch (size) {
        case ThumbnailSize::Normal: return {128, 128};
        case ThumbnailSize::Large:  return {256, 256};
        case ThumbnailSize::XLarge: return {512, 512};
    }
    return {256, 256};
}

std::pair<int,int> ThumbGenerator::dimensions_for_size(ThumbnailSize size)
{
    return ThumbGenerator_dims(size);
}

// ---------------------------------------------------------------------------
// CRC32 (PNG-compatible, table-based) — file-scope helper
// ---------------------------------------------------------------------------
static uint32_t png_crc32(const uint8_t* data, size_t len)
{
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int j = 0; j < 8; j++)
                c = (c & 1) ? (0xedb88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        init = true;
    }
    uint32_t crc = 0xffffffff;
    for (size_t i = 0; i < len; i++)
        crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    return crc ^ 0xffffffff;
}

static std::vector<uint8_t> make_text_chunk(const std::string& key, const std::string& value)
{
    std::string kv = key + '\0' + value;
    uint32_t len   = (uint32_t)kv.size();
    std::vector<uint8_t> chunk;
    chunk.reserve(4 + 4 + kv.size() + 4);
    chunk.push_back((len >> 24) & 0xff); chunk.push_back((len >> 16) & 0xff);
    chunk.push_back((len >>  8) & 0xff); chunk.push_back( len        & 0xff);
    chunk.push_back('t'); chunk.push_back('E'); chunk.push_back('X'); chunk.push_back('t');
    for (char c : kv) chunk.push_back((uint8_t)c);
    uint32_t crc = png_crc32(chunk.data() + 4, chunk.size() - 4);
    chunk.push_back((crc >> 24) & 0xff); chunk.push_back((crc >> 16) & 0xff);
    chunk.push_back((crc >>  8) & 0xff); chunk.push_back( crc        & 0xff);
    return chunk;
}

// ---------------------------------------------------------------------------
// embed_png_metadata — streaming, O(1) extra RAM
// ---------------------------------------------------------------------------
/**
 * Appends XDG tEXt metadata chunks into an existing PNG by streaming the file
 * through a 4KB buffer into a temp file and renaming atomically.
 * Peak extra RAM: ~200 bytes for the chunk data (not O(PNG size)).
 */
bool ThumbGenerator::embed_png_metadata(const std::string& png_path,
                                        const std::string& uri,
                                        long long mtime)
{
    auto chunk_uri      = make_text_chunk("Thumb::URI",      uri);
    auto chunk_mtime    = make_text_chunk("Thumb::MTime",    std::to_string(mtime));
    auto chunk_software = make_text_chunk("Thumb::Software", "horizon-lens");

    std::string tmp_path = png_path + ".tmp";
    std::ifstream in(png_path,  std::ios::binary);
    std::ofstream out(tmp_path, std::ios::binary);
    if (!in || !out) return false;

    in.seekg(0, std::ios::end);
    std::streamoff file_size = in.tellg();
    in.seekg(0, std::ios::beg);
    if (file_size < 12) return false;

    // Stream all bytes except the last 12 (IEND chunk)
    constexpr size_t BUF = 4096;
    char buf[BUF];
    std::streamoff remaining = file_size - 12;
    while (remaining > 0) {
        std::streamsize to_read = (std::streamsize)std::min((std::streamoff)BUF, remaining);
        in.read(buf, to_read);
        std::streamsize got = in.gcount();
        if (got <= 0) break;
        out.write(buf, got);
        remaining -= got;
    }

    // Insert our tEXt chunks
    out.write((char*)chunk_uri.data(),      (std::streamsize)chunk_uri.size());
    out.write((char*)chunk_mtime.data(),    (std::streamsize)chunk_mtime.size());
    out.write((char*)chunk_software.data(), (std::streamsize)chunk_software.size());

    // Copy IEND
    char iend[12];
    in.read(iend, 12);
    out.write(iend, in.gcount());

    in.close();
    out.close();

    std::error_code ec;
    fs::rename(tmp_path, png_path, ec);
    if (ec) { fs::remove(tmp_path, ec); return false; }
    return true;
}

// ---------------------------------------------------------------------------
// Raster image thumbnail (JPEG, PNG, BMP, GIF, WEBP, TIFF)
// ---------------------------------------------------------------------------
bool ThumbGenerator::generate_image(const std::string& path, const std::string& out_png,
                                    int width, int height, long long mtime)
{
    int img_w, img_h, channels;
    unsigned char* data = stbi_load(path.c_str(), &img_w, &img_h, &channels, 4);
    if (!data) {
        LOG_WARNING << "horizon-lens: stbi_load failed for " << path;
        return false;
    }

    // RGBA → BGRA (Cairo ARGB32 on little-endian)
    for (int i = 0; i < img_w * img_h * 4; i += 4)
        std::swap(data[i], data[i + 2]);

    double scale_x = (double)width  / img_w;
    double scale_y = (double)height / img_h;
    double scale   = std::min(scale_x, scale_y);
    int    dst_w   = (int)(img_w * scale);
    int    dst_h   = (int)(img_h * scale);

    cairo_surface_t* src  = cairo_image_surface_create_for_data(
        data, CAIRO_FORMAT_ARGB32, img_w, img_h, img_w * 4);
    cairo_surface_t* dest = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr           = cairo_create(dest);

    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);

    double tx = (width  - dst_w) / 2.0;
    double ty = (height - dst_h) / 2.0;
    cairo_translate(cr, tx, ty);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, src, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BILINEAR);
    cairo_paint(cr);

    bool ok = (cairo_surface_write_to_png(dest, out_png.c_str()) == CAIRO_STATUS_SUCCESS);

    cairo_destroy(cr);
    cairo_surface_destroy(dest);
    cairo_surface_destroy(src);
    stbi_image_free(data);

    if (ok) embed_png_metadata(out_png, "file://" + path, mtime);
    return ok;
}

// ---------------------------------------------------------------------------
// SVG thumbnail
// ---------------------------------------------------------------------------
bool ThumbGenerator::generate_svg(const std::string& path, const std::string& out_png,
                                  int width, int height, long long mtime)
{
    GError* error = nullptr;
    RsvgHandle* handle = rsvg_handle_new_from_file(path.c_str(), &error);
    if (!handle) {
        if (error) g_error_free(error);
        return false;
    }

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);

    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);

    RsvgRectangle viewport = {0, 0, (double)width, (double)height};
    rsvg_handle_render_document(handle, cr, &viewport, nullptr);

    bool ok = (cairo_surface_write_to_png(surface, out_png.c_str()) == CAIRO_STATUS_SUCCESS);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(handle);

    if (ok) embed_png_metadata(out_png, "file://" + path, mtime);
    return ok;
}

// ---------------------------------------------------------------------------
// PDF thumbnail (first page via Poppler)
// ---------------------------------------------------------------------------
bool ThumbGenerator::generate_pdf(const std::string& path, const std::string& out_png,
                                  int width, int height, long long mtime)
{
    std::string file_uri = "file://" + path;
    GError* error = nullptr;
    PopplerDocument* doc = poppler_document_new_from_file(file_uri.c_str(), nullptr, &error);
    if (!doc) {
        if (error) g_error_free(error);
        return false;
    }

    if (poppler_document_get_n_pages(doc) == 0) {
        g_object_unref(doc);
        return false;
    }

    PopplerPage* page = poppler_document_get_page(doc, 0);
    if (!page) {
        g_object_unref(doc);
        return false;
    }

    double page_w, page_h;
    poppler_page_get_size(page, &page_w, &page_h);

    double scale = std::min(width / page_w, height / page_h);
    int render_w = (int)(page_w * scale);
    int render_h = (int)(page_h * scale);

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    cairo_translate(cr, (width - render_w) / 2.0, (height - render_h) / 2.0);
    cairo_scale(cr, scale, scale);
    poppler_page_render(page, cr);

    bool ok = (cairo_surface_write_to_png(surface, out_png.c_str()) == CAIRO_STATUS_SUCCESS);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(page);
    g_object_unref(doc);

    if (ok) embed_png_metadata(out_png, file_uri, mtime);
    return ok;
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------
bool ThumbGenerator::generate(const std::string& file_path, ThumbnailSize size)
{
    if (!fs::exists(file_path)) return false;

    struct stat st;
    if (stat(file_path.c_str(), &st) != 0) return false;
    long long mtime = (long long)st.st_mtime;

    // Skip if already cached and valid
    if (!ThumbnailCache::get_thumbnail(file_path, size).empty()) return true;

    auto [width, height] = dimensions_for_size(size);
    std::string out_png  = ThumbnailCache::thumbnail_path_for(file_path, size);

    std::error_code ec;
    fs::create_directories(fs::path(out_png).parent_path(), ec);

    std::string ext = fs::path(file_path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    bool ok = false;
    if (ext == ".svg")
        ok = generate_svg(file_path, out_png, width, height, mtime);
    else if (ext == ".pdf")
        ok = generate_pdf(file_path, out_png, width, height, mtime);
    else
        ok = generate_image(file_path, out_png, width, height, mtime);

    if (!ok) {
        fs::remove(out_png, ec);
        std::string fail_path = out_png.substr(0, out_png.find_last_of('.')) + ".failed";
        std::ofstream f(fail_path);
        f << mtime;
    } else {
        std::string fail_path = out_png.substr(0, out_png.find_last_of('.')) + ".failed";
        fs::remove(fail_path, ec);
    }
    
    return ok;
}

} // namespace horizon::lens
