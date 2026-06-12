#include "horizon/StbImageDriver.hpp"
#include "horizon/GraphicsContext.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "horizon/external/stb_image.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace horizon
{
    struct SharedImageData
    {
        int width{0};
        int height{0};
        int channels{0};
        unsigned char *data{nullptr};

        ~SharedImageData()
        {
            if (data)
            {
                stbi_image_free(data);
            }
        }
    };

    static std::map<std::string, std::weak_ptr<SharedImageData>> g_image_cache;
    static std::mutex g_cache_mutex;

    struct StbImageDriver::Impl
    {
        std::shared_ptr<SharedImageData> shared_data;
    };

    StbImageDriver::StbImageDriver() : m_impl(std::make_unique<Impl>()) {}

    StbImageDriver::~StbImageDriver() = default;

    bool StbImageDriver::load(const std::string &path)
    {
        m_impl->shared_data.reset();

        std::lock_guard<std::mutex> lock(g_cache_mutex);
        
        m_path = path;
        auto it = g_image_cache.find(path);
        if (it != g_image_cache.end())
        {
            if (auto shared = it->second.lock())
            {
                m_impl->shared_data = shared;
                m_width = shared->width;
                m_height = shared->height;
                return true;
            }
        }

        auto shared = std::make_shared<SharedImageData>();
        shared->data = stbi_load(path.c_str(), &shared->width, &shared->height, &shared->channels, 4);
        if (!shared->data)
            return false;

        // Cairo CAIRO_FORMAT_ARGB32 on Little-Endian expects BGRA byte order.
        // STB loads as RGBA. We swap R and B.
        for (int i = 0; i < shared->width * shared->height * 4; i += 4)
        {
            std::swap(shared->data[i], shared->data[i + 2]);
        }

        g_image_cache[path] = shared;
        m_impl->shared_data = shared;
        m_width = shared->width;
        m_height = shared->height;
        return true;
    }

    void StbImageDriver::draw(GraphicsContext &ctx, int x, int y, int w, int h)
    {
        if (!m_impl->shared_data || !m_impl->shared_data->data)
            return;

        ctx.drawPixels(m_impl->shared_data->data, m_width, m_height, x, y, w, h, m_impl->shared_data->channels, m_path);
    }

} // namespace horizon
