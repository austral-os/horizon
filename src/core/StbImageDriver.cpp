#include "horizon/StbImageDriver.hpp"
#include "horizon/GraphicsContext.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "horizon/external/stb_image.h"

namespace horizon
{
    struct StbImageDriver::Impl
    {
        int channels{0};
        unsigned char *data{nullptr};

        ~Impl()
        {
            cleanup();
        }

        void cleanup()
        {
            if (data)
            {
                stbi_image_free(data);
                data = nullptr;
            }
        }
    };

    StbImageDriver::StbImageDriver() : m_impl(std::make_unique<Impl>()) {}

    StbImageDriver::~StbImageDriver() = default;

    bool StbImageDriver::load(const std::string &path)
    {
        m_impl->cleanup();

        m_impl->data = stbi_load(path.c_str(), &m_width, &m_height, &m_impl->channels, 4);
        if (!m_impl->data)
        {
            return false;
        }

        // Cairo CAIRO_FORMAT_ARGB32 on Little-Endian expects BGRA byte order.
        // STB loads as RGBA. We swap R and B.
        for (int i = 0; i < m_width * m_height * 4; i += 4)
        {
            std::swap(m_impl->data[i], m_impl->data[i + 2]);
        }

        return true;
    }

    void StbImageDriver::draw(GraphicsContext &ctx, int x, int y, int w, int h)
    {
        if (!m_impl->data)
            return;

        ctx.drawPixels(m_impl->data, m_width, m_height, x, y, w, h, m_impl->channels);
    }

} // namespace horizon
