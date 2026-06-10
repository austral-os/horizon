#pragma once

#include <horizon/Widget.hpp>
#include <horizon/Color.hpp>
#include <horizon/EventsManager.hpp>

namespace horizon {

class StarRating : public Widget {
public:
    StarRating();
    virtual ~StarRating() = default;

    float rating() const { return m_rating; }
    void set_rating(float rating);

    int max_stars() const { return m_max_stars; }
    void set_max_stars(int max_stars);

    int star_size() const { return m_star_size; }
    void set_star_size(int size);

    void set_readonly(bool ro) { m_readonly = ro; }
    bool is_readonly() const { return m_readonly; }

    EventsManager<float> when_change;

    void draw(GraphicsContext& gc) override;

private:
    float m_rating = 0.0f;
    int m_max_stars = 5;
    int m_star_size = 24;
    int m_spacing = 4;
    bool m_readonly = false;
    bool m_is_dragging = false;

    Color m_active_color{1.0f, 0.843f, 0.0f, 1.0f}; // Gold/Yellow
    Color m_inactive_color{1.0f, 0.843f, 0.0f, 0.3f}; // Apagado pero visible

    void update_rating_from_mouse(int x);
};

} // namespace horizon
