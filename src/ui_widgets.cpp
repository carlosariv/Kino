
#include "ui.h"

namespace ui {

Box *label(String string) {
    set_next_pref_width(pref_size_text(0.0f));
    set_next_pref_height(pref_size_text(0.0f));
    Box *box = box_create(BoxFlag_DrawBackground|BoxFlag_DrawText, string);
    return box;
}

Signal button(String string) {
    Box *box = box_create(BoxFlag_MouseInput|BoxFlag_DrawBackground|BoxFlag_DrawText, string);
    return signal_from_box(box);
}


void line_edit_text_op(u8 **buffer_out, int *pos, int *len, int *capacity, Keycode keycode) {
    int new_pos = *pos;
    if (keycode == Keycode::Left) {
        new_pos -= 1;
    } else if (keycode == Keycode::Right) {
        new_pos += 1;
    }
    *pos = new_pos;
}

struct LineEditDraw {
    u8 *txt;
    int pos;
    int len;
};

UI_DRAW_PROC(line_edit_draw) {
    LineEditDraw *ed = (LineEditDraw *)draw_data;

    String text = make_string_view(ed->txt, ed->len);

    draw_rect(box->rect, box->background_color);
    draw_text(text, box->rect.tl, box->font, box->text_color, box->font_size);
    Vector2 cursor = box->rect.tl;
    f32 scale = box->font_size / box->font->line_skip;
    for (int i = 0; i < ed->pos; i++) {
        u32 code = (u32)text.text[i];
        GlyphMetrics *g = font_get_glyph_metrics(box->font, code);
        cursor.x += g->advance * box->font->scale * scale;
    }
    Rect cursor_rect;
    cursor_rect.tl = cursor;
    cursor_rect.br = cursor_rect.tl + Vector2(2, box->font->line_skip);
    draw_rect(cursor_rect, Vector4(0, 0, 0, 1));
}

void line_edit_text_op(u8 **buffer_out, int *pos, int *len, int *capacity, String text) {
    u8 *buffer = *buffer_out;

    if (text.len == 0) return;

    int old_len = *len;
    int old_pos = *pos;
    int old_cap = *capacity;
    int new_len = old_len + text.len;
    int new_pos = old_pos + text.len;

    if (new_len > old_cap) {
        int new_cap = (new_len + 1) * 2;
        *capacity = new_cap;

        u8 *new_buffer = new u8[new_cap];
        memcpy(new_buffer, buffer, *len);
        buffer = new_buffer;
        *buffer_out = new_buffer;
    }

    // Move text after insertion point to after the text inserted at position
    memmove(buffer + new_pos, buffer + old_pos, old_len - old_pos);
    // Copy actual text to position
    memcpy(buffer + old_pos, text.text, text.len);

    *len = new_len;
    *pos = new_pos;
}

void line_edit(u8 **buffer, int *pos, int *len, int *capacity, String string) {
    Box *box = box_create(BoxFlag_MouseInput|BoxFlag_KeyboardInput|BoxFlag_DrawBackground|BoxFlag_DrawText, string);

    box->text = make_string_view(*buffer, *len);

    Signal sig = signal_from_box(box);

    if (is_hot_box_key(box->key)) {
        for (os::Event *evt  : os::window_events) {
            switch (evt->type) {
                case os::Event::KeyPress:
                    if (evt->key == Keycode::Left||evt->key == Keycode::Right) {
                        line_edit_text_op(buffer, pos, len, capacity, evt->key);
                    } else {
                        line_edit_text_op(buffer, pos, len, capacity, evt->text);
                    }
                    break;
            }
        }
    }

    LineEditDraw *draw_data = new LineEditDraw;
    draw_data->txt = *buffer;
    draw_data->pos = *pos;
    draw_data->len = *len;
    box->draw_data = (void *)draw_data;
    box->draw_proc = line_edit_draw;
}


};

