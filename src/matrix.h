#pragma once

#include "vector.h"

union Matrix4 {
    Vector4 columns[4];
    f32 elems[4][4] = {};

    Matrix4() {}

    Matrix4(f32 d) {
        elems[0][0] = d;
        elems[1][1] = d;
        elems[2][2] = d;
        elems[3][3] = d;
    }

    Vector4& operator[](int index) {
        return columns[index];
    }
};
