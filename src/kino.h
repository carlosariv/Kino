#pragma once

#include "array.h"
#include "string.h"
#include "buffer.h"
#include "enum_flags.hpp"
#include "os.h"
#include "keymap.h"

namespace kino {
struct Pane;
struct View;
struct Buffer;
struct ViewManager;
struct BufferManager;

struct FileSystem {
    String path;
    Array<os::File> files;
};

struct State {
    View *active_view;
    String self_insert;

    BufferManager *buffer_manager;
    ViewManager *view_manager;

    KeyMap *normal_map;
    KeyMap *insert_map;

    View *console_view;
    View *prev_view = nullptr;

    Pane *root_pane = nullptr;

    KeyNode *current_key_state = nullptr;
};

extern State *state;
extern FileSystem *g_file_system;

void init();
void update(f32 dt, Array<os::Event*> window_events);

void set_active_view(View *view);
View *get_active_view();

Buffer *find_buffer(i64 id);
View *find_view(i64 id);

};
