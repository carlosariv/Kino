#include <stdio.h>

#include "buffer.h"
#include "kino.h"
#include "view.h"

KINO_COMMAND(insert_mode) {
    kino::View *view = kino::get_active_view();
    view->map = view->insert_map;
}

KINO_COMMAND(normal_mode) {
    kino::View *view = kino::get_active_view();
    view->map = view->normal_map;
}

KINO_COMMAND(self_insert) {
    kino::View *view = kino::get_active_view();
    kino::Buffer *buffer = kino::buffer_find(view->bufid);

    String text = kino::state->self_insert;

    kino::buffer_insert_text(buffer, text, view->cursor.pos);

    view_move_offset(view, text.len);
}

KINO_COMMAND(newline) {
    kino::View *view = kino::get_active_view();
    kino::Buffer *buffer = kino::buffer_find(view->bufid);
    kino::buffer_insert_text(buffer, STRZ("\n"), view->cursor.pos);
    view_move_offset(view, 1);
}

KINO_COMMAND(newline_and_insert) {
    kino::View *view = kino::get_active_view();
    kino::Buffer *buffer = kino::buffer_find(view->bufid);
    int line_len = get_buffer_line_length(buffer, view->cursor.line);
    kino::buffer_insert_text(buffer, STRZ("\n"), view->cursor.pos + line_len - view->cursor.col - 1);
    view_move_offset(view, line_len - view->cursor.col);
    view->map = view->insert_map;
}

KINO_COMMAND(backspace) {
    kino::View *view = kino::get_active_view();
    kino::Buffer *buffer = kino::buffer_find(view->bufid);

    kino::buffer_delete_region(buffer, view->cursor.pos - 1, view->cursor.pos);
    view_move_offset(view, -1);
}

KINO_COMMAND(move_left) {
    kino::View *view = kino::get_active_view();
    view_move_offset(view, -1);
}

KINO_COMMAND(move_right) {
    kino::View *view = kino::get_active_view();
    view_move_offset(view, 1);
}

KINO_COMMAND(move_up) {
    kino::View *view = kino::get_active_view();
    view_move_lines(view, -1);
}

KINO_COMMAND(move_down) {
    kino::View *view = kino::get_active_view();
    view_move_lines(view, 1);
}

KINO_COMMAND(console_prompt) {
    kino::View *console = kino::state->console_view;
    kino::state->prev_view = kino::get_active_view();
    set_active_view(console);
}

KINO_COMMAND(console_complete_and_exit) {
    kino::View *console = kino::state->console_view;
    kino::Buffer *buffer = kino::buffer_find(console->bufid);

    String buffer_text = string_concat(String{buffer->text, buffer->gb}, String{buffer->text + buffer->ge, buffer->len - buffer->ge});

    String file_name = buffer_text;

    kino::View *view = kino::state->prev_view;
    set_active_view(view);

    kino::Buffer *new_buffer = kino::load_buffer_from_file(file_name);
    view->bufid = new_buffer->id;

    string_release(&buffer_text);
}

KINO_COMMAND(goto_begin) {
    kino::View *view = kino::get_active_view();
    view_move_lines(view, -view->cursor.line);
}

KINO_COMMAND(goto_end) {
    kino::View *view = kino::get_active_view();
    kino::Buffer *buffer = kino::buffer_find(view->bufid);
    i64 line_count = get_buffer_line_count(buffer);
    view_move_lines(view, line_count - view->cursor.line - 1);
}

KINO_COMMAND(goto_line_start) {
    printf("LINE BEGIN");
    kino::View *view = kino::get_active_view();
    view_move_offset(view, -view->cursor.col);
}

KINO_COMMAND(goto_line_end) {
    printf("LINE END");
    kino::View *view = kino::get_active_view();
    kino::Buffer *buffer = kino::buffer_find(view->bufid);
    int line_len = get_buffer_line_length(buffer, view->cursor.line);
    view_move_offset(view, line_len - view->cursor.col - 1);
}


KINO_COMMAND(hsplit) {
    kino::View *view = kino::get_active_view();
    // split_view(view, Axis_Y);
}

KINO_COMMAND(vsplit) {
    kino::View *view = kino::get_active_view();
    // split_view(view, Axis_X);
}
