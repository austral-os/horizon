#pragma once

#include <cmath>
#include <cstring>

namespace horizon
{
    /**
     * @brief Simple 4x4 Matrix math utilities to avoid direct GL dependecies in widgets.
     */
    class Matrix
    {
    public:
        static void identity(float *m)
        {
            for (int i = 0; i < 16; i++)
                m[i] = 0;
            m[0] = m[5] = m[10] = m[15] = 1.0f;
        }

        static void multiply(float *out, const float *a, const float *b)
        {
            float res[16];
            for (int c = 0; c < 4; c++)
            {
                for (int r = 0; r < 4; r++)
                {
                    res[c * 4 + r] = a[0 * 4 + r] * b[c * 4 + 0] + a[1 * 4 + r] * b[c * 4 + 1] +
                                     a[2 * 4 + r] * b[c * 4 + 2] + a[3 * 4 + r] * b[c * 4 + 3];
                }
            }
            std::memcpy(out, res, 16 * sizeof(float));
        }

        static void perspective(float *m, float fov, float aspect, float near, float far)
        {
            float f = 1.0f / std::tan(fov / 2.0f);
            m[0] = f / aspect;
            m[1] = 0.0f;
            m[2] = 0.0f;
            m[3] = 0.0f;
            m[4] = 0.0f;
            m[5] = f;
            m[6] = 0.0f;
            m[7] = 0.0f;
            m[8] = 0.0f;
            m[9] = 0.0f;
            m[10] = (far + near) / (near - far);
            m[11] = -1.0f;
            m[12] = 0.0f;
            m[13] = 0.0f;
            m[14] = (2.0f * far * near) / (near - far);
            m[15] = 0.0f;
        }

        static void ortho(float *m, float left, float right, float bottom, float top, float near,
                          float far)
        {
            m[0] = 2.0f / (right - left);
            m[1] = 0.0f;
            m[2] = 0.0f;
            m[3] = 0.0f;
            m[4] = 0.0f;
            m[5] = 2.0f / (top - bottom);
            m[6] = 0.0f;
            m[7] = 0.0f;
            m[8] = 0.0f;
            m[9] = 0.0f;
            m[10] = -2.0f / (far - near);
            m[11] = 0.0f;
            m[12] = -(right + left) / (right - left);
            m[13] = -(top + bottom) / (top - bottom);
            m[14] = -(far + near) / (far - near);
            m[15] = 1.0f;
        }

        static void translate(float *m, float x, float y, float z)
        {
            float t[16];
            identity(t);
            t[12] = x;
            t[13] = y;
            t[14] = z;
            multiply(m, m, t);
        }

        static void rotate_y(float *m, float angle)
        {
            float r[16];
            identity(r);
            r[0] = std::cos(angle);
            r[2] = -std::sin(angle);
            r[8] = std::sin(angle);
            r[10] = std::cos(angle);
            multiply(m, m, r);
        }

        static void scale(float *m, float x, float y, float z)
        {
            float s[16];
            identity(s);
            s[0] = x;
            s[5] = y;
            s[10] = z;
            multiply(m, m, s);
        }
    };
} // namespace horizon
