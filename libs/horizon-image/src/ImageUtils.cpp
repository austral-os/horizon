#include <horizon/image/ImageUtils.hpp>
#include <horizon/GraphicsContext.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace horizon {
namespace image {

void draw_rotated(GraphicsContext &ctx, const std::string &path,
                  int cx, int cy, int size, float degrees, float alpha)
{
    if (path.empty() || size <= 0)
        return;

    ctx.save();
    ctx.translate((float)cx, (float)cy);
    ctx.rotate(degrees * (float)(M_PI / 180.0f));
    ctx.drawImage(path, -size / 2, -size / 2, size, size, alpha);
    ctx.restore();
}

} // namespace image
} // namespace horizon
