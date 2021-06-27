#include <cmath>
#include <iostream>
#include "linmath.h"

Vec3 Vec3::from_euler_angles(float pitch, float yaw)
{
    float x = std::cos(yaw) * std::cos(pitch);
    float y = std::sin(pitch);
    float z = std::sin(yaw) * std::cos(pitch);

    return Vec3(x, y, z);
}

Vec3& Vec3::operator+=(Vec3 rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}

Vec3& Vec3::operator-=(Vec3 rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}

Vec3& Vec3::operator*(float rhs)
{
    x *= rhs;
    y *= rhs;
    z *= rhs;
    return *this;
}

Vec3 cross(Vec3 lhs, Vec3 rhs)
{
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x};
}

float length(Vec3 lhs)
{
    return sqrt(lhs.x * lhs.x + lhs.y * lhs.y + lhs.z * lhs.z);
}

Vec3 normalize(Vec3 lhs)
{
    auto reciprocal = 1.0f / length(lhs);
    return {lhs.x * reciprocal, lhs.y * reciprocal, lhs.z * reciprocal};
}

std::ostream& operator<<(std::ostream& stream, const Vec3& vec)
{
    stream << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return stream;
}
