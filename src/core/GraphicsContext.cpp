#include <horizon/GraphicsContext.hpp>
#include <iostream>

namespace horizon
{

    void GraphicsContext::drawRect(int x, int y, int width, int height)
    {
        std::cout << "Drawing rect at (" << x << "," << y << ") size " << width << "x" << height
                  << "\n";
    }

} // namespace horizon