#pragma once
#include <horizon/AquaObject.hpp>
#include <vector>

namespace horizon
{
    class AquaPolygon : public AquaObject
    {
    public:
        AquaPolygon();
        virtual ~AquaPolygon() = default;

        void set_points(const std::vector<PolygonPoint> &points);
        const std::vector<PolygonPoint> &points() const;

        void set_has_border(bool has_border);
        bool has_border() const;

        void set_border_size(float size);
        float border_size() const;

        void set_rotation(float angle);
        float rotation() const;

        void set_orientation(WidgetOrientation orientation);

        void draw(GraphicsContext &gc) override;

    protected:
        std::vector<PolygonPoint> m_points;
        bool m_has_border{false};
        float m_border_size{1.5f};
        float m_rotation{0.0f};
    };
} // namespace horizon
