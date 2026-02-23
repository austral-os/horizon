#pragma once

#include <cmath>
#include <string>

namespace horizon
{

    class Color
    {
    public:
        float r; // 0-1
        float g;
        float b;
        float a;

    public:
        // constructores
        Color();                                         // negro opaco
        Color(float r, float g, float b);                // rgb
        Color(float r, float g, float b, float a);       // rgba
        Color(const std::string &hex);                   // hex string #RRGGBB o #RRGGBBAA
        Color(float h, float s, float v, bool from_hsv); // hsv

        // conversiones
        std::string to_hex() const; // #RRGGBBAA
        void to_hsv(float &h, float &s, float &v) const;
        void to_rgb(float &out_r, float &out_g, float &out_b, float &out_a) const;

        // generacion de variantes
        Color lighter(float percent) const; // 0-100
        Color darker(float percent) const;

    private:
        static void hsv_to_rgb(float h, float s, float v, float &r, float &g, float &b);
        static void rgb_to_hsv(float r, float g, float b, float &h, float &s, float &v);
        static float clamp(float value);
    };
} // namespace horizon