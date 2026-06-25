#pragma once

#include "buffer.h"
#include "ui.h"
#include "keymap.h"

namespace kino {

struct Map;
struct View;

typedef i64 ViewID;

struct View;

struct Panel {
    Panel *tl = nullptr;
    Panel *br = nullptr;
    View *view = nullptr;
    ui::Box *box = nullptr;

    Axis split_axis;
    Vector2 pct_dim = Vector2::ZERO;
    Vector2 dim = Vector2::ZERO;
};

struct View {
    ViewID id = -1;
    View *next = nullptr;
    View *prev = nullptr;

    i64 bufid = -1;
    KeyMap *map;
    KeyMap *normal_map;
    KeyMap *insert_map;
    Cursor cursor;
    ui::Box *box;

    Panel *panel;
};

struct ViewManager {
    i64 running_id = 1;
    Array<View*> view_list;
};

Panel *panel_create();

View *view_create();

void view_move_lines(View *view, int lines);
void view_move_offset(View *view, int offset);
void split_view(View *view, Axis axis);

};

