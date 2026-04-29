#pragma once
#include <horizon/Widget.hpp>
#include <memory>

namespace horizon
{
    /**
     * @class UnderConstruction
     * @brief A widget intended to be used in sections that are not yet implemented.
     * It displays a centered "under construction" icon.
     */
    class UnderConstruction : public Widget
    {
    public:
        UnderConstruction();
        ~UnderConstruction() = default;

        void draw(GraphicsContext &gc) override;
    };
} // namespace horizon
