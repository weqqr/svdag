#version 450

in vec2 v_uv;
out vec4 color;

uniform vec3 u_position;
uniform vec3 u_look_dir;

const float PI = 3.1415926;
const vec3  UP = vec3(0, 1, 0);
const float FOV = 90.0 * PI / 180.0;
const float WIDTH = 1280;
const float HEIGHT = 720;
const vec3  SUN = normalize(vec3(0.3, 0.5, 0.7));

struct Ray {
    vec3 origin;
    vec3 dir;
};

struct Box {
    vec3 center;
    vec3 radius;
    vec3 inv_radius;
};

float maxf(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

// Majercik, Journal of Computer Graphics Techniques (JCGT), vol. 7, no. 3
// http://jcgt.org/published/0007/03/04/
bool intersect(Box box, Ray ray, out float distance, out vec3 normal, in vec3 inv_ray_dir)
{
    ray.origin = ray.origin - box.center;
    float winding = (maxf(abs(ray.origin) * box.inv_radius) < 1.0) ? -1 : 1;
    vec3 sgn = -sign(ray.dir);
    vec3 d = box.radius * winding * sgn - ray.origin;
    d *= inv_ray_dir;
#define TEST(U, VW) (d.U >= 0.0) && \
    all(lessThan(abs(ray.origin.VW + ray.dir.VW * d.U), box.radius.VW))
    bvec3 test = bvec3(TEST(x, yz), TEST(y, zx), TEST(z, xy));
    sgn = test.x ? vec3(sgn.x, 0, 0) : (test.y ? vec3(0, sgn.y, 0) :
        vec3(0, 0, test.z ? sgn.z : 0));
#undef TEST
    distance = (sgn.x != 0) ? d.x : ((sgn.y != 0) ? d.y : d.z);
    normal = sgn;
    return (sgn.x != 0) || (sgn.y != 0) || (sgn.z != 0);
}

void main() {
    vec3 origin = u_position; //vec3(-5, -5, -5);
    vec3 look_dir = u_look_dir; // normalize(vec3(1, 1, 1));
    vec3 U = normalize(cross(UP, -look_dir));
    vec3 V = normalize(cross(-look_dir, U));

    U *= 2.0 * 16.0 / 9.0;
    V *= 2.0;

    vec3 viewport_origin = look_dir - U/2 - V/2;

    vec3 dir = normalize(viewport_origin + v_uv.x * U + v_uv.y * V);

    vec3 center = vec3(0, 0, 0);
    const vec3 radius = vec3(0.5, 0.5, 0.5);
    const vec3 inv_radius = 1.0 / radius;

    Ray ray = {origin, dir};
    Box box = {center, radius, inv_radius};

    float distance;
    vec3 normal;
    vec3 inv_ray_dir = 1.0 / ray.dir;
    bool intersects = intersect(box, ray, distance, normal, inv_ray_dir);

    if (intersects)
	    color = vec4(0.3 + vec3(0.7) * clamp(dot(normal, SUN), 0, 1), 1);
	else
	    color = vec4(0, 0, 0, 1);
}

