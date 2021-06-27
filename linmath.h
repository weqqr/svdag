#pragma once

#include <iosfwd>

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vec3() = default;

    constexpr Vec3(float x, float y, float z)
        : x(x)
        , y(y)
        , z(z)
    {
    }

    static Vec3 from_euler_angles(float pitch, float yaw);

    Vec3& operator+=(Vec3 rhs);
    Vec3& operator-=(Vec3 rhs);
    Vec3& operator*(float rhs);

    const float* ptr() const { return &x; }
};

static constexpr Vec3 UP = Vec3(0, 1, 0);

Vec3 cross(Vec3 lhs, Vec3 rhs);
float length(Vec3 lhs);
Vec3 normalize(Vec3 lhs);
std::ostream& operator<<(std::ostream& stream, const Vec3& vec);
