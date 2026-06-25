#pragma once

#include "base_types.h"

union Vector2 {
    struct {
        f32 x, y;
    };
    f32 elems[2];

    Vector2() {}
    Vector2(f32 x, f32 y) : x(x), y(y) {}
    f32& operator[](int index) {
        return elems[index];
    }

    static const Vector2 ZERO;
    static const Vector2 ONE;
    static const Vector2 LEFT;
    static const Vector2 RIGHT;
    static const Vector2 UP;
    static const Vector2 DOWN;
};

union Vector3 {
    struct {
        f32 x, y, z;
    };
    f32 elems[3];

    Vector3() {}
    Vector3(f32 f) : x(f), y(f), z(f) {}
    Vector3(Vector2 xy, f32 z) : x(xy.x), y(xy.y), z(z) {}
    Vector3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
    f32& operator[](int index) {
        return elems[index];
    }

    static const Vector3 ZERO;
    static const Vector3 ONE;
    static const Vector3 LEFT;
    static const Vector3 RIGHT;
    static const Vector3 UP;
    static const Vector3 DOWN;
    static const Vector3 FORWARD;
    static const Vector3 BACKWARD;
};

union Vector4 {
    struct {
        f32 x, y, z, w;
    };
    f32 elems[4];

    Vector4() {}
    Vector4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
    f32& operator[](int index) {
        return elems[index];
    }

    static const Vector4 ZERO;
    static const Vector4 ONE;
    static const Vector4 LEFT;
    static const Vector4 RIGHT;
    static const Vector4 UP;
    static const Vector4 DOWN;
    static const Vector4 FORWARD;
    static const Vector4 BACKWARD;
};

union Vector2Int {
    struct {
        int x, y;
    };
    int elems[2];

    Vector2Int() {}
    Vector2Int(int x, int y) : x(x), y(y) {}
    int& operator[](int index) {
        return elems[index];
    }

    static const Vector2Int ZERO;
    static const Vector2Int ONE;
    static const Vector2Int LEFT;
    static const Vector2Int RIGHT;
    static const Vector2Int UP;
    static const Vector2Int DOWN;
};

union Vector3Int {
    struct {
        int x, y, z;
    };
    int elems[3];

    Vector3Int() {}
    Vector3Int(int f) : x(f), y(f), z(f) {}
    Vector3Int(Vector2Int xy, int z) : x(xy.x), y(xy.y), z(z) {}
    Vector3Int(int x, int y, int z) : x(x), y(y), z(z) {}
    int& operator[](int index) {
        return elems[index];
    }

    static const Vector3Int ZERO;
    static const Vector3Int ONE;
    static const Vector3Int LEFT;
    static const Vector3Int RIGHT;
    static const Vector3Int UP;
    static const Vector3Int DOWN;
    static const Vector3Int FORWARD;
    static const Vector3Int BACKWARD;
};

union Vector4Int {
    struct {
        int x, y, z, w;
    };
    int elems[4];

    Vector4Int() {}
    Vector4Int(int x, int y, int z, int w) : x(x), y(y), z(z), w(w) {}
    int& operator[](int index) {
        return elems[index];
    }

    static const Vector4Int ZERO;
    static const Vector4Int ONE;
    static const Vector4Int LEFT;
    static const Vector4Int RIGHT;
    static const Vector4Int UP;
    static const Vector4Int DOWN;
    static const Vector4Int FORWARD;
    static const Vector4Int BACKWARD;
};

Vector2Int operator-(Vector2Int v);
Vector3Int operator-(Vector3Int v);
Vector4Int operator-(Vector4Int v);

Vector2Int operator+(Vector2Int v);
Vector3Int operator+(Vector3Int v);
Vector4Int operator+(Vector4Int v);

Vector2Int operator*(int s, Vector2Int v);
Vector3Int operator*(int s, Vector3Int v);
Vector4Int operator*(int s, Vector4Int v);

Vector2Int operator*(Vector2Int v, int s);
Vector3Int operator*(Vector3Int v, int s);
Vector4Int operator*(Vector4Int v, int s);

Vector2Int operator+(Vector2Int left, Vector2Int right);
Vector3Int operator+(Vector3Int left, Vector3Int right);
Vector4Int operator+(Vector4Int left, Vector4Int right);

Vector2Int operator-(Vector2Int left, Vector2Int right);
Vector3Int operator-(Vector3Int left, Vector3Int right);
Vector4Int operator-(Vector4Int left, Vector4Int right);

Vector2 operator-(Vector2 v);
Vector3 operator-(Vector3 v);
Vector4 operator-(Vector4 v);

Vector2 operator+(Vector2 v);
Vector3 operator+(Vector3 v);
Vector4 operator+(Vector4 v);

Vector2 operator*(f32 s, Vector2 v);
Vector3 operator*(f32 s, Vector3 v);
Vector4 operator*(f32 s, Vector4 v);

Vector2 operator*(Vector2 v, f32 s);
Vector3 operator*(Vector3 v, f32 s);
Vector4 operator*(Vector4 v, f32 s);

Vector2 operator+(Vector2 left, Vector2 right);
Vector3 operator+(Vector3 left, Vector3 right);
Vector4 operator+(Vector4 left, Vector4 right);

Vector2 operator-(Vector2 left, Vector2 right);
Vector3 operator-(Vector3 left, Vector3 right);
Vector4 operator-(Vector4 left, Vector4 right);
