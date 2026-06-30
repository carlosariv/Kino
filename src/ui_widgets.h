#pragma once

#include "ui.h"

namespace ui {

struct ScrollPt {
    i64 idx;
    f32 off;
};

Signal spacer(PrefSize size);
Box *label(String string);

Signal button(String name);

void line_edit(u8 **buffer, int *pos, int *len, int *capacity, String string);

ScrollPt scroll_bar(String name, Axis axis, ScrollPt scroll_pt, i64 view_min, i64 view_max, i64 view_indices);
};
