#pragma once

#include "ui.h"

namespace ui {

struct ScrollPt {
    i64 idx;
    f32 off;
};

struct LineEdit {
    u8 *buffer;
    int len;
    int capacity;
    int pos;
};


struct ScrollListParams {
    Axis axis;
    PrefSize size[2];
    f32 row_height;
    Vector4 border_color;
    Vector4 text_color;
    i64 item_count;
    ScrollPt *scroll_pt;
};


Signal spacer(PrefSize size);
Box *label(String string);

Signal button(String name);

void line_edit(u8 **buffer, int *pos, int *len, int *capacity, String string);
ScrollPt scroll_bar(String name, Axis axis, ScrollPt scroll_pt, i64 item_count, i64 view_indices);


void scrollable_list_begin(ScrollListParams params);
void scrollable_list_end(ScrollListParams params);
};
