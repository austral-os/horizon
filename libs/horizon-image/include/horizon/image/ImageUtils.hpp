#pragma once

#include <string>

namespace horizon
{
class GraphicsContext;

namespace image
{

/**
 * @brief Draws an image centered at (cx, cy) with a rotation applied.
 *
 * The image is drawn at the given size and rotated around its center by
 * the specified angle. This is a convenience wrapper around GraphicsContext
 * save/translate/rotate/drawImage/restore.
 *
 * @param ctx      The graphics context to draw into.
 * @param path     Path to the image file (PNG, JPG, SVG, etc.).
 * @param cx       X-coordinate of the center of the drawn image.
 * @param cy       Y-coordinate of the center of the drawn image.
 * @param size     Width and height of the drawn image (square).
 * @param degrees  Rotation angle in degrees (clockwise).
 * @param alpha    Opacity (0.0 = fully transparent, 1.0 = fully opaque).
 */
void draw_rotated(GraphicsContext &ctx, const std::string &path,
                  int cx, int cy, int size, float degrees, float alpha = 1.0f);

} // namespace image
} // namespace horizon
