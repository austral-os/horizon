#include "horizon/SvgImageDriver.hpp"
#include "horizon/GraphicsContext.hpp"

namespace horizon
{
    struct SvgImageDriver::Impl
    {
        std::string path;
    };

    SvgImageDriver::SvgImageDriver() : m_impl(std::make_unique<Impl>()) {}

    SvgImageDriver::~SvgImageDriver() = default;

    bool SvgImageDriver::load(const std::string &path)
    {
        m_impl->path = path;
        // SVG size is now retrieved via the GraphicsContext or on demand.
        // But since load() currently doesn't have the context,
        // we'll rely on the factory to have set the size or we'll get it during first draw.
        // Actually, the factory in CairoGraphicsContext already calls getSvgSize.
        return true;
    }

    void SvgImageDriver::draw(GraphicsContext &ctx, int x, int y, int w, int h)
    {
        if (m_impl->path.empty())
            return;

        // If width/height were not set yet, ask the context
        if (m_width <= 0 || m_height <= 0)
        {
            ctx.getSvgSize(m_impl->path, m_width, m_height);
        }

        ctx.drawSvg(m_impl->path, x, y, w, h);
    }
} // namespace horizon
