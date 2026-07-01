
#include "ui.h"
#include "ui_widgets.h"

namespace ui {

Signal spacer(PrefSize size) {
    Box *parent = top_parent();
    ui::set_next_pref_size(parent->child_layout_axis, size);
    Box *box = box_create(BoxFlag_Default, key_zero());
    return signal_from_box(box);
}

Box *label(String string) {
    Box *box = box_create(BoxFlag_DrawText, string);
    return box;
}

Signal button(String string) {
    Box *box = box_create(BoxFlag_MouseClickable|BoxFlag_DrawBackground|BoxFlag_DrawText|BoxFlag_DrawHotEffects|BoxFlag_DrawActiveEffects, string);
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

    Vector2 text_position = get_box_text_position(box);
    f32 max_x = box->rect.x1 - text_position.x;

    f32 scale = box->font_size / box->font->size;
    f32 padding = scale * box->font->max_advance;

    Vector2 text_size = measure_text_size(text, box->font, box->font_size);
    f32 cursor_text_width = measure_text_size(substring(text, 0, ed->pos), box->font, box->font_size).x;
    f32 box_width = box->rect.x1 - box->rect.x0 - padding;
    f32 overflow = cursor_text_width - box_width;
    overflow = cu_clamp_bot(0, overflow);
    text_position.x -= overflow;

    draw_text(text, box->rect, text_position, box->font, box->text_color, box->font_size, max_x, text_size, DrawTextFlag_Default);

    Vector2 cursor = text_position;
    for (int i = 0; i < ed->pos; i++) {
        u32 code = (u32)text.text[i];
        GlyphMetrics *g = font_get_glyph_metrics(box->font, code);
        cursor.x += g->advance * scale;
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
    set_next_hover_cursor(os::Cursor_IBeam);
    Box *box = box_create(BoxFlag_MouseClickable|BoxFlag_ClickToFocus|BoxFlag_KeyboardClickable|BoxFlag_DrawBackground|BoxFlag_Clip, string);
    box->focus_color = Vector4(0.3f, 0.34f, 0.47f, 1.f);

    box->text = make_string_view(*buffer, *len);

    Signal sig = signal_from_box(box);

    if (key_match(ui_state->focus_box_key, box->key)) {
        for (os::Event *evt : ui_state->events) {
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

ScrollPt scroll_bar(String name, Axis axis, ScrollPt scroll_pt, i64 view_min, i64 view_max, i64 view_indices) { ScrollPt new_pt = scroll_pt;
    // i64 scroll_indices = view_indices + (view_max - view_min);
    // f32 scroll_ratio = (f32)(view_max - view_min) / (f32)scroll_indices;

    i64 scroll_indices = view_indices;

    Axis flipped_axis = flip_axis(axis);
    PrefSize axis_size = pct_size(1.f, 1.f);
    PrefSize flipped_size = pixel_size(20.f, 1.f);

    set_next_layout_axis(axis);
    set_next_pref_size(axis, axis_size);
    set_next_pref_size(flipped_axis, flipped_size);
    Box *container = box_create(BoxFlag_Default, name);
    UI_Parent(container) {
        set_next_background_color(Vector4(.16f, .16f, .16f, 1.f));
        set_next_pref_width(pct_size(1.f, 1.f));
        set_next_pref_height(pct_size(1.f, 1.f));
        Box *thumb_cont = box_create(BoxFlag_MouseClickable|BoxFlag_DrawBackground, STRZ("##thumb_cont"));
        Vector2 thumb_dim = get_rect_size(thumb_cont->rect);

        Signal scroll_sig = signal_from_box(thumb_cont);

        if (scroll_sig.clicked) {
            Vector2 scroll_pos = get_mouse_cursor() - thumb_cont->rect.tl;
            new_pt.idx = (i64)(scroll_pos[axis] / (thumb_dim[axis] / (f32)view_indices));
            // printf("CLICKED %lld %f %f\n", new_pt.idx, scroll_pos.x, scroll_pos.y);
        }

        f32 scroll_ratio = 1.0f / (f32)scroll_indices;

        f32 thumb_pos = thumb_dim[axis] * ((f32)(scroll_pt.idx+1) / (f32)scroll_indices);

        UI_Parent(thumb_cont) {
            set_next_fixed_x(0.f);
            set_next_fixed_y(thumb_pos);
            set_next_pref_size(axis, pct_size(scroll_ratio, 1.f));
            set_next_pref_size(flipped_axis, flipped_size);
            set_next_hover_cursor(os::Cursor_Hand);
            set_next_background_color(Vector4(0.42f, 0.44f, 0.49f, 1.f));
            Box *thumb_box = box_create(BoxFlag_MouseClickable|BoxFlag_DrawBackground, STRZ("##thumb"));
            Signal thumb_sig = signal_from_box(thumb_box);
            if (thumb_sig.dragging) {
                Vector2 scroll_pos = get_mouse_cursor() - thumb_cont->rect.tl;
                new_pt.idx = (i64)(scroll_pos[axis] / (thumb_dim[axis] / (f32)view_indices));
            }
        }
    }

    new_pt.idx = cu_clamp(0, new_pt.idx,  view_indices - 1);
    return new_pt;
}


};

