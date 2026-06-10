#include <horizon/StarRating.hpp>
#include <horizon/GraphicsContext.hpp>
#include <horizon/ApplicationWindow.hpp>
#include <cmath>
#include <algorithm>

namespace horizon {

StarRating::StarRating() {
    set_size(m_max_stars * m_star_size + (m_max_stars - 1) * m_spacing, m_star_size);
    set_cursor_type(CursorType::Pointer);

    when_mouse_press.connect([this](MouseButtonEventContext& ctx) {
        if (m_readonly || ctx.button != 272) return;
        m_is_dragging = true;
        update_rating_from_mouse(ctx.x);
    });

    when_mouse_release.connect([this](MouseButtonEventContext& ctx) {
        if (ctx.button == 272) {
            m_is_dragging = false;
        }
    });

    when_mouse_move.connect([this](MouseMoveEventContext& ctx) {
        if (m_is_dragging) {
            update_rating_from_mouse(ctx.x);
        }
    });
}

void StarRating::set_rating(float rating) {
    float new_rating = std::clamp(rating, 0.0f, static_cast<float>(m_max_stars));
    if (m_rating != new_rating) {
        m_rating = new_rating;
        if (application()) invalidate();
    }
}

void StarRating::set_max_stars(int max_stars) {
    if (max_stars > 0 && m_max_stars != max_stars) {
        m_max_stars = max_stars;
        set_size(m_max_stars * m_star_size + (m_max_stars - 1) * m_spacing, m_star_size);
        if (application()) invalidate();
    }
}

void StarRating::set_star_size(int size) {
    if (size > 0 && m_star_size != size) {
        m_star_size = size;
        set_size(m_max_stars * m_star_size + (m_max_stars - 1) * m_spacing, m_star_size);
        if (application()) invalidate();
    }
}

void StarRating::draw(GraphicsContext& gc) {
    Widget::draw(gc);

    int cx = m_x;
    int cy = m_y;

    // Star generation based on 10 points
    auto generate_star = [](int cx, int cy, int outer_radius, int inner_radius) {
        std::vector<PolygonPoint> points;
        int num_points = 5;
        float angle = -M_PI / 2.0f;
        float step = M_PI / num_points;

        for (int i = 0; i < 2 * num_points; ++i) {
            float r = (i % 2 == 0) ? outer_radius : inner_radius;
            points.push_back({
                static_cast<int>(cx + r * std::cos(angle)),
                static_cast<int>(cy + r * std::sin(angle)),
                0
            });
            angle += step;
        }
        return points;
    };

    int outer_r = m_star_size / 2;
    int inner_r = outer_r / 2;

    for (int i = 0; i < m_max_stars; ++i) {
        int star_x = cx + i * (m_star_size + m_spacing);
        int center_x = star_x + outer_r;
        int center_y = cy + outer_r;

        auto star_points = generate_star(center_x, center_y, outer_r, inner_r);

        // Draw inactive star (background)
        gc.setColor(m_inactive_color);
        gc.fillPolygon(star_points);

        // Draw active star (foreground) based on rating
        float fill_amount = std::clamp(m_rating - i, 0.0f, 1.0f);
        if (fill_amount > 0.0f) {
            gc.save();
            gc.clip(star_x, cy, static_cast<int>(m_star_size * fill_amount), m_star_size);
            gc.setColor(m_active_color);
            gc.fillPolygon(star_points);
            gc.restore();
        }

        // Draw border for the star
        gc.setColor(m_active_color.darker(0.2f));
        gc.drawPolygon(star_points, 1.5f);
    }
}

void StarRating::update_rating_from_mouse(int x) {
    if (m_readonly) return;
    
    int relative_x = x - m_x;
    float new_rating = 0.0f;
    
    for (int i = 0; i < m_max_stars; ++i) {
        int star_start = i * (m_star_size + m_spacing);
        int star_end = star_start + m_star_size;
        
        if (relative_x >= star_start && relative_x <= star_end) {
            float fraction = static_cast<float>(relative_x - star_start) / m_star_size;
            new_rating = i + fraction;
            break;
        } else if (relative_x > star_end && i == m_max_stars - 1) {
            new_rating = m_max_stars;
        } else if (relative_x > star_end) {
            new_rating = i + 1;
        }
    }
    
    // Optional step: snap to nearest 0.5 or leave precise? The prompt asks for fractional.
    // We'll leave it precise or rounded to 1 decimal. Let's round to nearest 0.1
    new_rating = std::round(new_rating * 10.0f) / 10.0f;
    new_rating = std::clamp(new_rating, 0.0f, static_cast<float>(m_max_stars));
    
    if (m_rating != new_rating) {
        m_rating = new_rating;
        when_change.run(m_rating);
        if (application()) invalidate();
    }
}

} // namespace horizon
