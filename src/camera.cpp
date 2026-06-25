#include "math.h"
#include "camera.h"

void Camera::set_aspect_ratio(f32 aspect_ratio) {
    this->aspect_ratio = aspect_ratio;
    update_projection_matrix();
}

void Camera::set_perspective_projection(f32 fov, f32 aspect_ratio, f32 near_z, f32 far_z) {
    // this->projection_type = ProjectionType::Perspective;
    this->fov = fov;
    this->aspect_ratio = aspect_ratio;
    this->near_z = near_z;
    this->far_z = far_z;

    update_projection_matrix();
}

void Camera::set_position(Vector3 position) {
    transform.position = position;

    update_view_matrix();
}

void Camera::rotate(f32 yaw, f32 pitch) {
    set_rotator(Vector3(transform.rotator.x + yaw, transform.rotator.y + pitch, 0));
}

void Camera::set_rotator(Vector3 rotator) {
    f32 yaw = rotator.x;
    f32 pitch = rotator.y;
    if (pitch < -0.45*math::PI) {
        pitch = -0.45*math::PI;
    } else if (pitch > 0.45*math::PI) {
        pitch = 0.45*math::PI;
    }

    transform.rotator.x = yaw;
    transform.rotator.y = pitch;

    forward = math::forward_vector(yaw, pitch);
    right = math::normalize(math::cross(forward, Vector3::UP));
    up = math::normalize(math::cross(right, forward));

    update_view_matrix();
}

void Camera::update_view_matrix() {
    view_matrix = math::look_at_rh(transform.position, transform.position - forward, Vector3::UP);
}

void Camera::update_projection_matrix() {
    projection_matrix = math::perspective_rh_zo(fov, aspect_ratio, near_z, far_z);
}

bool point_behind_plane(Vector3 p, Vector4 plane) {
    f32 dp = math::dot(Vector3(plane.x, plane.y, plane.z), p);
    return dp < -plane.w;
}

bool aabb_in_frustum(Frustum frustum, AABB box) {
    for (int i = 0; i < 6; i++) {
        Vector4 plane = frustum.planes[i];
        int out = 0;
        out += point_behind_plane(Vector3(box.x0, box.y0, box.z0), plane);
        out += point_behind_plane(Vector3(box.x1, box.y0, box.z0), plane);
        out += point_behind_plane(Vector3(box.x1, box.y1, box.z0), plane);
        out += point_behind_plane(Vector3(box.x0, box.y1, box.z0), plane);
        out += point_behind_plane(Vector3(box.x0, box.y0, box.z1), plane);
        out += point_behind_plane(Vector3(box.x1, box.y0, box.z1), plane);
        out += point_behind_plane(Vector3(box.x1, box.y1, box.z1), plane);
        out += point_behind_plane(Vector3(box.x0, box.y1, box.z1), plane);
        if (out == 8) return false;
    }
    return true;
}

Frustum make_frustum(Camera camera) {
    Matrix4 vp = camera.projection_matrix * camera.view_matrix;
    vp = math::transpose(vp);

    Frustum frustum = {};
    frustum.planes[0] = vp.columns[0] + vp.columns[3];
    frustum.planes[1] = -vp.columns[0] + vp.columns[3];
    frustum.planes[2] = vp.columns[1] + vp.columns[3];
    frustum.planes[3] = -vp.columns[1] + vp.columns[3];
    frustum.planes[4] = vp.columns[2] + vp.columns[3];
    frustum.planes[5] = -vp.columns[2] + vp.columns[3];
    for (int i = 0; i < 6; i++) {
        frustum.planes[i] = math::normalize(frustum.planes[i]);
    }
    return frustum;
}

// Frustum frustum_from_camera(Camera camera) {
    // f32 h_len = camera.far_z * math::tan(camera.fov * 0.5f);
    // f32 v_len = h_len * camera.aspect_ratio;
    // Vector3 far_point = camera.forward * camera.far_z;
    //
    // Vector3 left_normal   = math::normalize(math::cross(far_point - camera.right * h_len, camera.up));
    // Vector3 right_normal  = math::normalize(math::cross(far_point + camera.right * h_len, camera.up));
    // Vector3 top_normal    = math::normalize(math::cross(camera.right, far_point + camera.up * v_len));
    // Vector3 bottom_normal = math::normalize(math::cross(-camera.right, far_point - camera.up * v_len));
    //
    // Frustum f;
    // f.near_ = { camera.forward * camera.near_z, camera.forward };
    // f.far_ = { camera.forward * camera.far_z, -camera.forward };
    // f.left = { camera.transform.position, left_normal };
    // f.right = {camera.transform.position, right_normal };
    // f.top = { camera.transform.position, top_normal };
    // f.bottom = { camera.transform.position, bottom_normal };
//     return f;
// }

