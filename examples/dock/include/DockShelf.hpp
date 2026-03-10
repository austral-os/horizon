#pragma once

#include <horizon/Widget.hpp>

namespace horizon
{

    /**
     * @brief Custom widget mimicking the Mac OS X Mountain Lion 3D Dock shelf.
     */
    class DockShelf : public Widget
    {
    public:
        DockShelf();

        void calculate_layout() override;
        void draw(GraphicsContext &gc) override;
    };

} // namespace horizon
