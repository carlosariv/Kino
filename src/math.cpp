
#include <cmath>
#include <assert.h>
#include "math.h"

inline const Vector2 Vector2::ZERO  = {0, 0};
inline const Vector3 Vector3::ZERO  = {0, 0, 0};
inline const Vector4 Vector4::ZERO  = {0, 0, 0, 0};

inline const Vector2 Vector2::ONE  = {1, 1};
inline const Vector3 Vector3::ONE  = {1, 1, 1};
inline const Vector4 Vector4::ONE  = {1, 1, 1, 1};

inline const Vector2 Vector2::RIGHT = {1, 0};
inline const Vector2 Vector2::UP    = {0, 1};
inline const Vector2 Vector2::DOWN  = {0, -1};

inline const Vector3 Vector3::LEFT  = {-1, 0, 0};
inline const Vector3 Vector3::RIGHT = {1, 0, 0};
inline const Vector3 Vector3::UP    = {0, 1, 0};
inline const Vector3 Vector3::DOWN  = {0, -1, 0};
inline const Vector3 Vector3::FORWARD  = {0, 0, 1};
inline const Vector3 Vector3::BACKWARD = {0, 0, -1};

inline const Vector4 Vector4::LEFT  = {-1, 0, 0, 0};
inline const Vector4 Vector4::RIGHT = {1, 0, 0, 0};
inline const Vector4 Vector4::UP    = {0, 1, 0, 0};
inline const Vector4 Vector4::DOWN  = {0, -1, 0, 0};
inline const Vector4 Vector4::FORWARD  = {0, 0, 1, 0};
inline const Vector4 Vector4::BACKWARD = {0, 0, -1, 0};

namespace math {

f32 cos(f32 f) { return std::cosf(f); }
f64 cos(f64 f) { return std::cos(f); }
f32 sin(f32 f) { return std::sinf(f); }
f64 sin(f64 f) { return std::sin(f); }
f32 tan(f32 f) { return std::tanf(f); }
f64 tan(f64 f) { return std::tan(f); }

f32 length(Vector2 v) {
    f32 f = v.x * v.x + v.y * v.y;
    return sqrtf(f);
}

f32 length(Vector3 v) {
    f32 f = v.x * v.x + v.y * v.y + v.z * v.z;
    return sqrtf(f);
}

f32 length(Vector4 v) {
    f32 f = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
    return sqrtf(f);
}

f32 length_squared(Vector2 v) {
    f32 f = v.x * v.x + v.y * v.y;
    return f;
}

f32 length_squared(Vector3 v) {
    f32 f = v.x * v.x + v.y * v.y + v.z * v.z;
    return f;
}

f32 length_squared(Vector4 v) {
    f32 f = v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
    return f;
}


f32 dot(Vector2 left, Vector2 right) {
    return left.x * right.x + left.y * right.y;
}

f32 dot(Vector3 left, Vector3 right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

f32 dot(Vector4 left, Vector4 right) {
    return left.x * right.x + left.y * right.y + left.z * right.z + left.w * right.w;
}

Vector2 normalize(Vector2 v) {
    f32 len = length(v);
    assert(len != 0);
    Vector2 result = Vector2(v.x / len, v.y / len);
    return result;
}

Vector3 normalize(Vector3 v) {
    f32 len = length(v);
    assert(len != 0);
    Vector3 result = Vector3(v.x / len, v.y / len, v.z / len);
    return result;
}

Vector4 normalize(Vector4 v) {
    f32 len = length(v);
    assert(len != 0);
    Vector4 result = Vector4(v.x/len, v.y/len, v.z/len, v.w/len);
    return result;
}

Vector3 cross(Vector3 left, Vector3 right) {
    Vector3 result;
    result.x = (left.y * right.z) - (left.z * right.y);
    result.y = -((left.x * right.z) - (left.z * right.x));
    result.z = (left.x * right.y) - (left.y * right.x);
    return result;
}

Matrix4 transpose(Matrix4 m) {
    Matrix4 result = m;
    result[0][1] = m[1][0];
    result[0][2] = m[2][0];
    result[0][3] = m[3][0];
    result[1][0] = m[0][1];
    result[1][2] = m[2][1];
    result[1][3] = m[3][1];
    result[2][0] = m[0][2];
    result[2][1] = m[1][2];
    result[2][3] = m[3][2];
    result[3][0] = m[0][3];
    result[3][1] = m[1][3];
    result[3][2] = m[2][3];
    return result;
}

Vector4 linear_combine(Vector4 left, Matrix4 right) {
    Vector4 result;
    result.x = left.elems[0] * right.columns[0].x;
    result.y = left.elems[0] * right.columns[0].y;
    result.z = left.elems[0] * right.columns[0].z;
    result.w = left.elems[0] * right.columns[0].w;

    result.x += left.elems[1] * right.columns[1].x;
    result.y += left.elems[1] * right.columns[1].y;
    result.z += left.elems[1] * right.columns[1].z;
    result.w += left.elems[1] * right.columns[1].w;

    result.x += left.elems[2] * right.columns[2].x;
    result.y += left.elems[2] * right.columns[2].y;
    result.z += left.elems[2] * right.columns[2].z;
    result.w += left.elems[2] * right.columns[2].w;

    result.x += left.elems[3] * right.columns[3].x;
    result.y += left.elems[3] * right.columns[3].y;
    result.z += left.elems[3] * right.columns[3].z;
    result.w += left.elems[3] * right.columns[3].w;
    return result;
}

Matrix4 translate(f32 x, f32 y, f32 z) {
    Matrix4 result(1.0f);
    result[3][0] = x;
    result[3][1] = y;
    result[3][2] = z;
    return result;
}

Matrix4 translate(Vector3 v) {
    return translate(v.x, v.y, v.z);
}

Matrix4 scale(f32 x, f32 y, f32 z) {
    Matrix4 result(1.0f);
    result[0][0] = x;
    result[1][1] = y;
    result[2][2] = z;
    result[3][3] = 1.0f;
    return result;
}

Matrix4 rotate_rh(float angle, Vector3 axis) {
    Matrix4 result = Matrix4(1.0f);

    axis = normalize(axis);

    float sin_theta = sin(angle);
    float cos_theta = cos(angle);
    float cos_value = 1.0f - cos_theta;

    result.elems[0][0] = (axis.x * axis.x * cos_value) + cos_theta;
    result.elems[0][1] = (axis.x * axis.y * cos_value) + (axis.z * sin_theta);
    result.elems[0][2] = (axis.x * axis.z * cos_value) - (axis.y * sin_theta);

    result.elems[1][0] = (axis.y * axis.x * cos_value) - (axis.z * sin_theta);
    result.elems[1][1] = (axis.y * axis.y * cos_value) + cos_theta;
    result.elems[1][2] = (axis.y * axis.z * cos_value) + (axis.x * sin_theta);

    result.elems[2][0] = (axis.z * axis.x * cos_value) + (axis.y * sin_theta);
    result.elems[2][1] = (axis.z * axis.y * cos_value) - (axis.x * sin_theta);
    result.elems[2][2] = (axis.z * axis.z * cos_value) + cos_theta;

    return result;
}

Matrix4 orthographic_rh_no(f32 left, f32 right, f32 bottom, f32 top, f32 z_near, f32 z_far) {
    Matrix4 result;
    result[0][0] = 2.0f / (right - left);
    result[1][1] = 2.0f / (top - bottom);
    result[2][2] = 2.0f / (z_near - z_far);
    result[3][3] = 1.0f;

    result[3][0] = (left + right) / (left - right);
    result[3][1] = (bottom + top) / (bottom - top);
    result[3][2] = (z_near + z_far) / (z_near - z_far);
    return result;
}

Matrix4 ortho(f32 left, f32 top, f32 right, f32 bottom) {
    Matrix4 result;
    result.elems[0][0] = 2.0f / (right - left);
    result.elems[1][1] = 2.0f / (top - bottom);
    result.elems[2][2] = -1.0f;
    result.elems[3][0] = -(right + left) / (right - left);
    result.elems[3][1] = -(top + bottom) / (top - bottom);
    return result;
}


Matrix4 perspective_rh_zo(float FOV, float AspectRatio, float Near, float Far) {
    Matrix4 Result = {0};
    // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

    float Cotangent = 1.0f / std::tanf(FOV / 2.0f);
    Result[0][0] = Cotangent / AspectRatio;
    Result[1][1] = Cotangent;
    Result[2][3] = -1.0f;

    Result[2][2] = (Far) / (Near - Far);
    Result[3][2] = (Near * Far) / (Near - Far);

    return Result;
}

Matrix4 perspective_rh_no(float FOV, float AspectRatio, float Near, float Far) {
    Matrix4 result = {};

    // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluperspective.xml
    float Cotangent = 1.0f / std::tanf(FOV / 2.0f);
    result[0][0] = Cotangent / AspectRatio;
    result[1][1] = Cotangent;
    result[2][3] = -1.0f;

    result[2][2] = (Near + Far) / (Near - Far);
    result[3][2] = (2.0f * Near * Far) / (Near - Far);

    return result;
}

Matrix4 perspective_lh_no(float FOV, float AspectRatio, float Near, float Far) {
    Matrix4 result = perspective_rh_no(FOV, AspectRatio, Near, Far);
    result[2][2] = -result[2][2];
    result[2][3] = -result[2][3];
    return result;
}

Matrix4 HMM_Perspective_LH_ZO(float FOV, float AspectRatio, float Near, float Far) {

    Matrix4 Result = perspective_rh_zo(FOV, AspectRatio, Near, Far);
    Result[2][2] = -Result[2][2];
    Result[2][3] = -Result[2][3];
    return Result;
}


Matrix4 perspective(f32 fov, f32 aspect, f32 z_near, f32 z_far) {
    assert(z_near != z_far);
    f32 rad = fov;
    f32 tan_half_fov = std::tan(rad / 2.0f);
    Matrix4 result(0.0f);
    result[0][0] = 1.0f / (aspect * tan_half_fov);
    result[1][1] = 1.0f / (tan_half_fov);
    result[2][2] = -(z_far + z_near) / (z_far - z_near);
    result[2][3] = -1.0f;
    result[3][2] = -(2.0f * z_far * z_near) / (z_far - z_near);
    return result;
}

Matrix4 _look_at(Vector3 F, Vector3 S, Vector3 U, Vector3 Eye)
{
    Matrix4 result;

    result.elems[0][0] = S.x;
    result.elems[0][1] = U.x;
    result.elems[0][2] = -F.x;
    result.elems[0][3] = 0.0f;

    result.elems[1][0] = S.y;
    result.elems[1][1] = U.y;
    result.elems[1][2] = -F.y;
    result.elems[1][3] = 0.0f;

    result.elems[2][0] = S.z;
    result.elems[2][1] = U.z;
    result.elems[2][2] = -F.z;
    result.elems[2][3] = 0.0f;

    result.elems[3][0] = -dot(S, Eye);
    result.elems[3][1] = -dot(U, Eye);
    result.elems[3][2] = dot(F, Eye);
    result.elems[3][3] = 1.0f;

    return result;
}

Matrix4 look_at_rh(Vector3 eye, Vector3 center, Vector3 up) {
    Vector3 F = normalize(eye - center);
    Vector3 S = normalize(cross(F, up));
    Vector3 U = cross(S, F);
    return _look_at(F, S, U, eye);
}

Matrix4 look_at_lh(Vector3 eye, Vector3 center, Vector3 up) {
    Vector3 F = normalize(eye - center);
    Vector3 S = normalize(cross(F, up));
    Vector3 U = cross(S, F);
    return _look_at(F, S, U, eye);
}

Vector3 forward_vector(f32 yaw, f32 pitch) {
    Vector3 F;
    F.x = cos(yaw) * cos(pitch);
    F.y = sinf(pitch);
    F.z = sinf(yaw) * cos(pitch);
    return normalize(F);
}

};

Vector2 operator-(Vector2 v) {
    return Vector2(-v.x, -v.y);
}
Vector3 operator-(Vector3 v) {
    return Vector3(-v.x, -v.y, -v.z);
}
Vector4 operator-(Vector4 v) {
    return Vector4(-v.x, -v.y, -v.z, -v.w);
}

Vector2 operator+(Vector2 v) {
    return v;
}
Vector3 operator+(Vector3 v) {
    return v;
}
Vector4 operator+(Vector4 v) {
    return v;
}

Vector2 operator*(f32 s, Vector2 v) {
    return Vector2(v.x * s, v.y * s);
}

Vector3 operator*(f32 s, Vector3 v) {
    return Vector3(v.x * s, v.y * s, v.z * s);
}

Vector4 operator*(f32 s, Vector4 v) {
    return Vector4(v.x * s, v.y * s, v.z * s, v.w * s);
}

Vector2 operator*(Vector2 v, f32 s) {
    return Vector2(v.x * s, v.y * s);
}

Vector3 operator*(Vector3 v, f32 s) {
    return Vector3(v.x * s, v.y * s, v.z * s);
}

Vector4 operator*(Vector4 v, f32 s) {
    return Vector4(v.x * s, v.y * s, v.z * s, v.w * s);
}

Vector2 operator+(Vector2 left, Vector2 right) {
    return Vector2(left.x + right.x, left.y + right.y);
}

Vector3 operator+(Vector3 left, Vector3 right) {
    return Vector3(left.x + right.x, left.y + right.y, left.z + right.z);
}

Vector4 operator+(Vector4 left, Vector4 right) {
    return Vector4(left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w);
}

Vector2 operator-(Vector2 left, Vector2 right) {
    return Vector2(left.x - right.x, left.y - right.y);
}

Vector3 operator-(Vector3 left, Vector3 right) {
    return Vector3(left.x - right.x, left.y - right.y, left.z - right.z);
}

Vector4 operator-(Vector4 left, Vector4 right) {
    return Vector4(left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w);
}

Vector2Int operator-(Vector2Int v) {
    return Vector2Int(-v.x, -v.y);
}
Vector3Int operator-(Vector3Int v) {
    return Vector3Int(-v.x, -v.y, -v.z);
}
Vector4Int operator-(Vector4Int v) {
    return Vector4Int(-v.x, -v.y, -v.z, -v.w);
}

Vector2Int operator+(Vector2Int v) {
    return v;
}
Vector3Int operator+(Vector3Int v) {
    return v;
}
Vector4Int operator+(Vector4Int v) {
    return v;
}

Vector2Int operator*(f32 s, Vector2Int v) {
    return Vector2Int(v.x * s, v.y * s);
}

Vector3Int operator*(f32 s, Vector3Int v) {
    return Vector3Int(v.x * s, v.y * s, v.z * s);
}

Vector4Int operator*(f32 s, Vector4Int v) {
    return Vector4Int(v.x * s, v.y * s, v.z * s, v.w * s);
}

Vector2Int operator*(Vector2Int v, f32 s) {
    return Vector2Int(v.x * s, v.y * s);
}

Vector3Int operator*(Vector3Int v, f32 s) {
    return Vector3Int(v.x * s, v.y * s, v.z * s);
}

Vector4Int operator*(Vector4Int v, f32 s) {
    return Vector4Int(v.x * s, v.y * s, v.z * s, v.w * s);
}

Vector2Int operator+(Vector2Int left, Vector2Int right) {
    return Vector2Int(left.x + right.x, left.y + right.y);
}

Vector3Int operator+(Vector3Int left, Vector3Int right) {
    return Vector3Int(left.x + right.x, left.y + right.y, left.z + right.z);
}

Vector4Int operator+(Vector4Int left, Vector4Int right) {
    return Vector4Int(left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w);
}

Vector2Int operator-(Vector2Int left, Vector2Int right) {
    return Vector2Int(left.x - right.x, left.y - right.y);
}

Vector3Int operator-(Vector3Int left, Vector3Int right) {
    return Vector3Int(left.x - right.x, left.y - right.y, left.z - right.z);
}

Vector4Int operator-(Vector4Int left, Vector4Int right) {
    return Vector4Int(left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w);
}

Matrix4 operator*(Matrix4 left, Matrix4 right) {
    Matrix4 result;
    result[0] = math::linear_combine(right.columns[0], left);
    result[1] = math::linear_combine(right.columns[1], left);
    result[2] = math::linear_combine(right.columns[2], left);
    result[3] = math::linear_combine(right.columns[3], left);
    return result;
}

