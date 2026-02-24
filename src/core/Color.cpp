#include <algorithm>
#include <cmath>
#include <horizon/Color.hpp>
#include <iomanip>
#include <sstream>

namespace horizon
{

    // constructores
    Color::Color() : r(0.f), g(0.f), b(0.f), a(1.f) {}
    Color::Color(float r_, float g_, float b_) : r(r_), g(g_), b(b_), a(1.f) {}
    Color::Color(float r_, float g_, float b_, float a_) : r(r_), g(g_), b(b_), a(a_) {}

    // constructor hex
    Color::Color(const std::string &hex)
    {
        unsigned int ri = 0, gi = 0, bi = 0, ai = 255;
        if (hex.size() == 7)
        {
            std::sscanf(hex.c_str(), "#%02x%02x%02x", &ri, &gi, &bi);
            ai = 255;
        }
        else if (hex.size() == 9)
        {
            std::sscanf(hex.c_str(), "#%02x%02x%02x%02x", &ri, &gi, &bi, &ai);
        }
        r = ri / 255.f;
        g = gi / 255.f;
        b = bi / 255.f;
        a = ai / 255.f;
    }

    // constructor HSV
    Color::Color(float h, float s, float v, bool from_hsv)
    {
        if (from_hsv)
            hsv_to_rgb(h, s, v, r, g, b);
        a = 1.f;
    }

    // conversion a hex
    std::string Color::to_hex() const
    {
        int ri = static_cast<int>(clamp(r) * 255.f);
        int gi = static_cast<int>(clamp(g) * 255.f);
        int bi = static_cast<int>(clamp(b) * 255.f);
        int ai = static_cast<int>(clamp(a) * 255.f);

        std::ostringstream ss;
        ss << "#" << std::setw(2) << std::setfill('0') << std::hex << ri << std::setw(2)
           << std::setfill('0') << std::hex << gi << std::setw(2) << std::setfill('0') << std::hex
           << bi << std::setw(2) << std::setfill('0') << std::hex << ai;
        return ss.str();
    }

    // conversion a rgb
    void Color::to_rgb(float &out_r, float &out_g, float &out_b, float &out_a) const
    {
        out_r = r;
        out_g = g;
        out_b = b;
        out_a = a;
    }

    // conversion a hsv
    void Color::to_hsv(float &h, float &s, float &v) const
    {
        rgb_to_hsv(r, g, b, h, s, v);
    }

    // lighter
    Color Color::lighter(float percent) const
    {
        float h, s, v;
        to_hsv(h, s, v);
        v += v * (percent / 100.f);
        v = std::min(v, 1.f);
        float nr, ng, nb;
        hsv_to_rgb(h, s, v, nr, ng, nb);
        return Color(nr, ng, nb, a);
    }

    // darker
    Color Color::darker(float percent) const
    {
        float h, s, v;
        to_hsv(h, s, v);
        v -= v * (percent / 100.f);
        v = std::max(v, 0.f);
        float nr, ng, nb;
        hsv_to_rgb(h, s, v, nr, ng, nb);
        return Color(nr, ng, nb, a);
    }

    // with_alpha
    Color Color::with_alpha(float alpha) const
    {
        return Color(r, g, b, clamp(alpha));
    }

    // HSV -> RGB
    void Color::hsv_to_rgb(float h, float s, float v, float &out_r, float &out_g, float &out_b)
    {
        float c = v * s;
        float x = c * (1 - std::fabs(fmod(h / 60.f, 2.f) - 1));
        float m = v - c;
        float r1, g1, b1;
        if (h < 60.f)
        {
            r1 = c;
            g1 = x;
            b1 = 0;
        }
        else if (h < 120.f)
        {
            r1 = x;
            g1 = c;
            b1 = 0;
        }
        else if (h < 180.f)
        {
            r1 = 0;
            g1 = c;
            b1 = x;
        }
        else if (h < 240.f)
        {
            r1 = 0;
            g1 = x;
            b1 = c;
        }
        else if (h < 300.f)
        {
            r1 = x;
            g1 = 0;
            b1 = c;
        }
        else
        {
            r1 = c;
            g1 = 0;
            b1 = x;
        }
        out_r = r1 + m;
        out_g = g1 + m;
        out_b = b1 + m;
    }

    // RGB -> HSV
    void Color::rgb_to_hsv(float r, float g, float b, float &h, float &s, float &v)
    {
        float maxc = std::max({r, g, b});
        float minc = std::min({r, g, b});
        float delta = maxc - minc;

        v = maxc;
        if (delta == 0.f)
            h = 0.f;
        else if (maxc == r)
            h = 60.f * fmod(((g - b) / delta), 6.f);
        else if (maxc == g)
            h = 60.f * (((b - r) / delta) + 2.f);
        else
            h = 60.f * (((r - g) / delta) + 4.f);

        if (h < 0.f)
            h += 360.f;

        s = maxc == 0.f ? 0.f : delta / maxc;
    }

    // clamp [0,1]
    float Color::clamp(float value)
    {
        if (value < 0.f)
            return 0.f;
        if (value > 1.f)
            return 1.f;
        return value;
    }
} // namespace horizon
