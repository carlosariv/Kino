#pragma once

#include "base_types.h"
#include "transform.h"

struct AABB {
    f32 x0, y0, z0;
    f32 x1, y1, z1;
};

enum ProjectionType {
    Orthographic,
    Perspective,
};

struct Camera2D {
    Vector2 position;
    f32 rotation;
    f32 scale;
};

struct Camera {
    f32 fov;
    f32 aspect_ratio;
    f32 near_z;
    f32 far_z;

    Vector3 rotation;
    Vector3 forward;
    Vector3 right;
    Vector3 up;

    Transform transform;
    Matrix4 view_matrix;
    Matrix4 projection_matrix;

    void set_aspect_ratio(f32 aspect_ratio);

    void set_perspective_projection(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z);
    // void set_orthographic_projection(f32 width, f32 height);

    void set_position(Vector3 position);
    void rotate(f32 yaw, f32 pitch);
    void set_rotator(Vector3 rotator);

    void update_view_matrix();
    void update_projection_matrix();
};

struct Plane {
    Vector3 position = Vector3::ZERO;
    Vector3 normal = Vector3::ZERO;
};

struct Frustum {
    Vector4 planes[6];
};

bool point_behind_plane(Vector3 p, Vector4 plane);
bool aabb_in_frustum(Frustum frustum, AABB box);
Frustum make_frustum(Camera camera);
