#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <math.h>

namespace vian {
    namespace geom {

        class matrix4_t;

        class vector3d
        {
        public:
            float x;
            float y;
            float z;

            vector3d() : x(0), y(0), z(0) {}
            vector3d(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
            vector3d(const vector3d &v) : x(v.x), y(v.y), z(v.z) {}

            vector3d& operator = (const vector3d &v) { x = v.x; y = v.y; z = v.z; return *this; }
            vector3d operator + () const { return *this; }
            vector3d operator - () const { return vector3d(-x, -y, -z); }
            vector3d& operator += (const vector3d &v) { x += v.x; y += v.y; z += v.z; return *this; }
            vector3d& operator -= (const vector3d &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
            vector3d& operator *= (float a) { x *= a; y *= a; z *= a; return *this; }

            float length() const { return sqrtf(x * x + y * y + z * z); }
            // FIXME: shit return result
            void normalize() { *this *= 1.0f / length(); }

            static vector3d cross(const vector3d &u, const vector3d &v) { return vector3d(u.y * v.z - u.z * v.y,
                                                                                          u.z * v.x - u.x * v.z,
                                                                                          u.x * v.y - u.y * v.x); }
            vector3d transform(const matrix4_t &m);
        };

        inline vector3d operator + (const vector3d &u, const vector3d &v) { return vector3d(u.x + v.x, u.y + v.y, u.z + v.z); }
        inline vector3d operator - (const vector3d &u, const vector3d &v) { return vector3d(u.x - v.x, u.y - v.y, u.z - v.z); }
        inline vector3d operator * (float a, const vector3d &v) { return vector3d(a * v.x, a * v.y, a * v.z); }
        inline vector3d operator * (const vector3d &v, float a) { return vector3d(a * v.x, a * v.y, a * v.z); }
        inline float operator * (const vector3d &u, const vector3d &v) { return u.x * v.x + u.y * v.y + u.z * v.z; }
    }
}
#endif // VECTOR3D_H
