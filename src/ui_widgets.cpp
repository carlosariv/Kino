
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

void scrollable_list_begin(ScrollListParams params) {
    Axis side_axis = flip_axis(params.axis);

    ui::set_next_pref_width(params.size[Axis_X]);
    ui::set_next_pref_height(params.size[Axis_Y]);
    ui::set_next_layout_axis(side_axis);
    ui::set_next_child_alignment_x(ui::Alignment_Center);
    ui::set_next_border_color(Vector4(0.65f, 0.67f, 0.8f, 1.f));
    ui::Box *scroll_list_cont = ui::box_create(ui::BoxFlag_Clip|ui::BoxFlag_DrawBackground|ui::BoxFlag_Scroll, STRZ("##scroll_list"));

    int visible_row_count = (scroll_list_cont->rect.bottom - scroll_list_cont->rect.top) / params.row_height;

    ScrollPt scroll_pt = *params.scroll_pt;
    i64 max_pt = 0;
    if (params.item_count > visible_row_count) {
        max_pt = params.item_count - (params.item_count % cu_clamp_bot(1, visible_row_count));
    }

    ui::Signal scroll_list_sig = ui::signal_from_box(scroll_list_cont);

    push_parent(scroll_list_cont);

    f32 row_offset = -(scroll_pt.idx * params.row_height + scroll_pt.off);

    // File Buttons
    ui::set_next_pref_width(ui::pct_size(1.f, 0.9f));
    ui::set_next_pref_height(ui::pct_size(1.f, 0.9f));
    ui::set_next_layout_axis(params.axis);
    ui::set_next_fixed_position(params.axis, row_offset);
    ui::Box *scrollable_cont = ui::box_create(ui::BoxFlag_AllowOverflowY|ui::BoxFlag_Scroll, STRZ("##items_cont"));

    if (scroll_list_sig.scroll.y != 0) {
        scroll_pt.idx += scroll_list_sig.scroll.y;
        scroll_pt.idx = cu_clamp(0, scroll_pt.idx, max_pt);
    }

    push_parent(scrollable_cont);

    *params.scroll_pt = scroll_pt;
}

void scrollable_list_end(ScrollListParams params) {
    pop_parent();

    Box *scroll_list_cont = top_parent();

    ScrollPt scroll_pt = *params.scroll_pt;

    int visible_row_count = (scroll_list_cont->rect.bottom - scroll_list_cont->rect.top) / params.row_height;

    Vector2 mouse = get_mouse_cursor();


    scroll_pt = ui::scroll_bar(STRZ("##scroll"), params.axis, scroll_pt, params.item_count, visible_row_count);
    *params.scroll_pt = scroll_pt;

    pop_parent();
}

ScrollPt scroll_bar(String name, Axis axis, ScrollPt scroll_pt, i64 item_count, i64 view_indices) {
    //- TODO: Fix scrollbar sizing and thumb container.

    //- TODO: Disable scroll bar if not active and not in mouse over scroll bar
    //        Should be an optional style parameter
    //
    // Rect scroll_bar_rect = scroll_list_cont->rect;
    // scroll_bar_rect.x0 += 0.8f*(scroll_list_cont->rect.x1-scroll_list_cont->rect.x0);
    // if (point_in_rect(mouse, scroll_bar_rect)) {
    // }

    ScrollPt new_pt = scroll_pt;
    // i64 scroll_indices = view_indices + (view_max - view_min);
    // f32 scroll_ratio = (f32)(view_max - view_min) / (f32)scroll_indices;
    // f32 scroll_ratio = 1.0f / (f32)scroll_indices;
    // i64 scroll_indices = view_indices;

    f32 scroll_ratio = (f32)view_indices / (f32)cu_clamp_bot(item_count, 1);
    scroll_ratio = cu_clamp_top(scroll_ratio, 1.f);

    i64 max_pt = 0;
    if (item_count > view_indices) {
        max_pt = item_count - (item_count % cu_clamp_bot(1, view_indices));
    }

    Axis side_axis = flip_axis(axis);
    PrefSize axis_size = pct_size(1.f, 1.f);
    PrefSize side_size = pixel_size(20.f, 1.f);

    Signal thumb_sig = {};
    Signal scroll_sig = {};

    Box *thumb_cont = nullptr;
    f32 thumb_pos;
    Vector2 thumb_dim;

    set_next_layout_axis(axis);
    set_next_pref_size(axis, axis_size);
    push_pref_size(side_axis, side_size);
    Box *container = box_create(BoxFlag_Default, name);
    UI_Parent(container) {
        // set_next_pref_size(axis, text_size(4.f,1.f));
        // Box *arrow_up = box_create(BoxFlag_MouseClickable|BoxFlag_DrawBackground|BoxFlag_DrawText|BoxFlag_DrawHotEffects, STRZ("<##arrow_up"));
        // Signal arrow_up_sig = signal_from_box(arrow_up);
        //
        // if (arrow_up_sig.clicked) {
        //     new_pt.idx -= 1;
        // }

        thumb_dim = get_rect_size(container->rect);

        f32 available_size = thumb_dim[axis] * (1.f-scroll_ratio);// - 60.f;

        // thumb_pos = available_size * ((f32)scroll_pt.idx / (f32)cu_clamp_bot(view_indices, 1)) + scroll_pt.off;
        thumb_pos = available_size * ((f32)scroll_pt.idx / (f32)cu_clamp_bot(max_pt, 1)) + scroll_pt.off;

        set_next_background_color(Vector4(.16f, .16f, .16f, 1.f));
        set_next_fixed_position(axis, thumb_pos);
        set_next_pref_size(axis, pixel_size(thumb_dim[axis] - available_size, 1.f));
        set_next_hover_cursor(os::Cursor_Hand);
        set_next_background_color(Vector4(0.42f, 0.44f, 0.49f, 1.f));
        thumb_cont = box_create(BoxFlag_MouseClickable|BoxFlag_DrawBackground, STRZ("##thumb_cont"));

        thumb_sig = signal_from_box(thumb_cont);
        scroll_sig = signal_from_box(container);

        // set_next_pref_size(axis, text_size(4.f,1.f));
        // set_next_fixed_position(axis, thumb_dim[axis] - 30.f);
        // Box *arrow_down = box_create(BoxFlag_MouseClickable|BoxFlag_DrawBackground|BoxFlag_DrawText|BoxFlag_DrawHotEffects, STRZ(">##arrow_down"));
        // Signal arrow_down_sig = signal_from_box(arrow_down);
        //
        // if (arrow_down_sig.clicked) {
        //     new_pt.idx += 1;
        // }

        f32 pt_height = cu_clamp_bot(available_size / (f32)max_pt, 1);

        if (scroll_sig.clicked || thumb_sig.dragging) {
            f32 scroll_pos = (get_mouse_cursor() - thumb_cont->rect.tl)[axis];
            new_pt.idx = scroll_pos / pt_height;
            new_pt.off = scroll_pos - new_pt.idx*pt_height;
        }

        if (new_pt.idx > max_pt || new_pt.idx < 0) {
            new_pt.idx = cu_clamp(0, new_pt.idx, max_pt);
            new_pt.off = 0.0f;
        }
    }


    pop_pref_size(side_axis);

    return new_pt;
}


};

