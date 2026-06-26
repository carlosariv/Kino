#pragma once

#include "base_types.h"

#include "vector.h"
#include "matrix.h"

namespace math {

const f32 PI = 3.14159265358979323846264338327950288f;

f32 cos(f32 f);
f64 cos(f64 f);
f32 sin(f32 f);
f64 sin(f64 f);
f32 tan(f32 f);
f64 tan(f64 f);

inline f32 radians(f32 deg) {
    return deg * PI / 180.0f;
}

inline f32 degrees(f32 rads) {
    return rads * 180.0f / PI;
}

f32 length(Vector2 v);
f32 length(Vector3 v);
f32 length(Vector4 v);
f32 length_squared(Vector2 v);
f32 length_squared(Vector3 v);
f32 length_squared(Vector4 v);

Vector2 normalize(Vector2 v);
Vector3 normalize(Vector3 v);
Vector4 normalize(Vector4 v);

f32 dot(Vector2 left, Vector2 right);
f32 dot(Vector3 left, Vector3 right);
f32 dot(Vector4 left, Vector4 right);

Vector3 cross(Vector3 left, Vector3 right);

Matrix4 transpose(Matrix4 m);


// Transformations
Vector4 linear_combine(Vector4 left, Matrix4 right);

Matrix4 translate(f32 x, f32 y, f32 z);
Matrix4 translate(Vector3 v);
Matrix4 scale(f32 x, f32 y, f32 z);
Matrix4 rotate_rh(float angle, Vector3 axis);

Matrix4 orthographic_rh_no(f32 left, f32 right, f32 bottom, f32 top, f32 z_near, f32 z_far);
Matrix4 ortho(f32 left, f32 top, f32 right, f32 bottom);

Matrix4 perspective_rh_no(float FOV, float AspectRatio, float Near, float Far);
Matrix4 perspective_rh_zo(float FOV, float AspectRatio, float Near, float Far);
Matrix4 perspective_lh_no(float FOV, float AspectRatio, float Near, float Far);
Matrix4 perspective_lh_zo(float FOV, float AspectRatio, float Near, float Far);
Matrix4 perspective(f32 fov, f32 aspect, f32 z_near, f32 z_far);

Matrix4 look_at_rh(Vector3 eye, Vector3 center, Vector3 up);
Matrix4 look_at_lh(Vector3 eye, Vector3 center, Vector3 up);

Vector3 forward_vector(f32 yaw, f32 pitch);
};

Matrix4 operator*(Matrix4 left, Matrix4 right);
