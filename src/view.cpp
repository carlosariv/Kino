#include "buffer.h"
#include "view.h"
#include "kino.h"

namespace kino {

void add_view(View *view) {
    ViewManager *manager = state->view_manager;
    manager->view_list.add(view);
    view->id = manager->running_id;
    manager->running_id++;
}


Panel *panel_create() {
    Panel *panel = new Panel;
    return panel;
}

View *view_create() {
    View *view = new View;
    view->bufid = -1;
    view->cursor = {};
    view->normal_map = state->normal_map;
    view->insert_map = state->insert_map;
    view->map = view->normal_map;
    add_view(view);
    return view;
}

void view_move_lines(View *view, int lines) {
    Buffer *buffer = find_buffer(view->bufid);
    Cursor c = view->cursor;

    i64 line_count = get_buffer_line_count(buffer);
    i64 line = c.line + lines;
    if (line < 0) {
        line = 0;
    } else if (line >= line_count) {
        line = line_count - 1;
    }

    c.pos = buffer->line_starts[line];
    c.line = line;
    c.col = 0;

    view->cursor = c;
}

void view_move_offset(View *view, int offset) {
    Buffer *buffer = find_buffer(view->bufid);
    i64 buffer_len = get_buffer_len(buffer);
    i64 new_pos = view->cursor.pos + offset;

    if (new_pos < 0) {
        new_pos = 0;
    }
    if (new_pos > buffer_len) {
        new_pos = buffer_len;
    }

    i64 line_count = get_buffer_line_count(buffer);
    i64 line;
    i64 col = 0;

    for (line = 0; line < line_count; line++) {
        BufferPos start = buffer->line_starts[line];
        BufferPos end = start + get_buffer_line_length(buffer, line);

        if (new_pos >= start && new_pos < end) {
            col = new_pos - start;
            break;
        }
    }

    view->cursor.pos = new_pos;
    view->cursor.line = line;
    view->cursor.col = col;
}

// void split_view(View *view, Axis axis) {
//     View *container = view_create();
//
//     if (parent) {
//         if (parent->tl == pane) {
//             parent->tl = container;
//         } else {
//             parent->br = container;
//         }
//     } else {
//         state->root_pane = container;
//     }
//
//     container->dim = parent ? parent->pct : pane->pct;
//     container->split_axis = axis;
//     container->tl = pane;
//     container->br = br;
//
//     pane->parent = container;
//     br->parent = container;
//
//     container->parent = parent;
//     container->tl->pct = 0.5;
//     container->br->pct = 0.5;
// }

};


