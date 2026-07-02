#include <assert.h>
#include <spdlog/spdlog.h>

#include "string.h"
#include "ui.h"
#include "texture.h"
#include "math.h"
#include "utils.h"

#define UI_GEN_AUTO_POP(Type,Name) ui_state->Name ## _stack.auto_pop();
#define UI_AUTO_POP_STACKS UI_PARAM_LIST(UI_GEN_AUTO_POP)

#define UI_GEN_CLEAR(Type,Name) ui_state->Name ## _stack.clear();
#define UI_CLEAR_STACKS UI_PARAM_LIST(UI_GEN_CLEAR)

namespace ui {

UIState *ui_state;
DrawData *ui_draw_data;
Font *icon_font;

Arena *get_build_arena() {
    return ui_state->build_arenas[ui_state->build_index%cu_count_of(ui_state->build_arenas)];
}

char *icon_kind_strings[] = {
    "\0",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
};

String string_from_icon_kind(IconKind kind, const char *end) {
    char *icon_string = icon_kind_strings[kind];
    String result = string_fmt("%s%s", icon_string, end);
    return result;
}

Vector2 get_mouse_cursor() {
    return Vector2(ui_state->mx, ui_state->my);
}

Vector2 get_rect_size(Rect rect) {
    return Vector2(rect.right - rect.left, rect.bottom - rect.top);
}

bool point_in_rect(Vector2 pt, Rect rect) {
    return pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom;
}

bool mouse_in_rect(Rect rect) {
    Vector2 pt = get_mouse_cursor();
    return point_in_rect(pt, rect);
}

bool key_match(Key a, Key b) {
    return a == b;
}

Key key_zero() {
    return (Key)0;
}

// FNV-1
Key get_hash_key(Key seed, String string) {
    // u64 offset_basis = 0xcbf29ce484222325;
    u64 offset_basis = seed;
    u64 prime = 0x100000001b3;

    Key hash = offset_basis;
    for (isize i = 0; i < string.len; i++) {
        char ch = (char)string.text[i];
        hash = hash * prime;
        hash = hash ^ ch;
    }
    return hash;
}

Key get_seed_key(Box *parent) {
    Key seed = 0;
    while (parent) {
        if (parent->key != 0) {
            seed = parent->key;
            break;
        }
        parent = parent->parent;
    }
    return seed;
}

Box *find_box(Key key) {
    Box* box = nullptr;
    auto it = ui_state->persistent_map.find(key);
    if (it != ui_state->persistent_map.end()) {
        box = it->second;
    }
    return box;
}

void push_box_to_parent(Box *parent, Box *box) {
    assert(parent != nullptr);

    //@Note: Childrens to root are actually separate layers with different depths (sequential).
    if (parent == ui_state->root) {
        box->depth = ui_state->depth_counter;
        ui_state->depth_counter++;
    }

    box->parent = parent;

    box->prev = parent->last;
    if (!parent->first) {
        parent->first = box;
        parent->last = box;
    } else {
        parent->last->next = box;
        parent->last = box;
    }
    box->child_count++;
}

Box *find_layer_container(Box *box) {
    Box *p = box;
    while (p) {
        if (p->parent == ui_state->root) {
            break;
        }
        p = p->parent;
    }
    return p;
}

Box *box_create(BoxFlags flags, Key key) {
    bool is_transient = key == key_zero();
    Box *box = find_box(key);
    bool just_created = false;
    if (!box || is_transient) {
        just_created = true;
        box = New(Box, ui_state->allocator);
    }

    if (just_created && !is_transient) {
        ui_state->persistent_map.insert({ key, box });
    }

    box->flags = flags;
    box->key = key;
    box->depth = 0;

    box->fixed_position = Vector2(0, 0);
    box->fixed_size = Vector2(0, 0);

    box->parent = nullptr;
    box->next = nullptr;
    box->prev = nullptr;
    box->first = nullptr;
    box->last = nullptr;
    box->child_count = 0;

    Box *parent = top_parent();
    if (flags & BoxFlag_Layer) {
        parent = ui_state->root;
    }

    if (parent) {
        push_box_to_parent(parent, box);
    }

    box->font = top_font();
    box->font_size = top_font_size();
    box->child_layout_axis = top_layout_axis();
    box->child_alignment[Axis_X] = top_child_alignment_x();
    box->child_alignment[Axis_Y] = top_child_alignment_y();
    box->text_alignment = top_text_alignment();
    box->background_color = top_background_color();
    box->border_color = top_border_color();
    box->text_color = top_text_color();
    box->hot_color = top_hot_color();
    box->active_color = top_active_color();
    box->focus_color = Vector4(1, 0, 0, 1);

    if (!ui_state->fixed_x_stack.is_empty()) {
        box->flags |= BoxFlag_FloatingX;
        box->fixed_position.x = top_fixed_x();
    }
    if (!ui_state->fixed_y_stack.is_empty()) {
        box->flags |= BoxFlag_FloatingY;
        box->fixed_position.y = top_fixed_y();
    }

    if (!ui_state->fixed_width_stack.is_empty()) {
        box->flags |= BoxFlag_FixedWidth;
        box->fixed_size.x = top_fixed_width();
    } else {
        box->pref_size[Axis_X] = top_pref_width();
    }
    if (!ui_state->fixed_height_stack.is_empty()) {
        box->flags |= BoxFlag_FixedHeight;
        box->fixed_size.y = top_fixed_height();
    } else {
        box->pref_size[Axis_Y] = top_pref_height();
    }

    box->hover_cursor = top_hover_cursor();

    UI_AUTO_POP_STACKS;

    return box;
}

String get_hash_part(String string) {
    String hash_part = string;
    isize start = string_find(string, STRZ("##"));
    if (start != -1) {
        hash_part = substring(string, start, string.len - start);
    }
    return hash_part;
}

Key get_active_seed_key() {
    Key key = 0;
    Box *keyed_ancestor = nullptr;
    Box *parent = top_parent();
    for (Box *p = parent; p; p = p->parent) {
        if (p->key != 0) {
            keyed_ancestor = p;
            break;
        }
    }
    if (keyed_ancestor) {
        key = keyed_ancestor->key;
    }
    return key;
}

Box *box_create(BoxFlags flags, String string) {
    Box *parent = top_parent();

    String hash_part = string;
    isize hash_start = string_find(string, STRZ("##"));
    if (hash_start != -1) {
        hash_part = substring(string, hash_start, string.len);
    }

    Key key = get_hash_key(get_active_seed_key(), hash_part);
    Box *box = box_create(flags, key);

    if (flags & BoxFlag_DrawText) {
        String text = string;
        if (hash_start != -1) {
            text.len = hash_start;
        }
        box->text = text;
    }

    // printf("create: %s hashstr: %s hash: %llu\n", (char *)string.text, (char *)hash_part.text, key);
    return box;
}

Vector2 measure_text_size(String text, Font *font, f32 size) {
    f32 dpi = os::main_window->dpi;
    // f32 line_height = size * (dpi / 72.f);
    f32 scale = size / font->size;
    Vector2 dim = Vector2(0, font->line_skip * scale);
    f32 w = 0.0f;
    for (isize i = 0; i < text.len; i++) {
        u32 code = (u32)text.text[i];
        GlyphMetrics *g = font_get_glyph_metrics(font, code);

        if (code == '\n') {
            if (w > dim.x) {
                dim.x = w;
            }
            w = 0.0f;
            dim.y += font->line_skip * scale;
        } else {
            w += g ? g->advance * scale : 0;
        }
    }
    if (w > dim.x) {
        dim.x = w;
    }

    return dim;
}

struct BoxRec {
    Box *next;
    int push_count;
    int pop_count;
};


BoxRec box_rec_pre(Box *box, Box *root) {
    BoxRec result = {};
    if (box->first) {
        result.next = box->first;
        result.push_count = 1;
    } else for (Box *p = box; p && p != root; p = p->parent) {
        if (p->next) {
            result.next = p->next;
            break;
        }
        result.pop_count += 1;
    }
    return result;
}

void layout_calc_fixed_sizes(Box *root, Axis axis) {
    for (Box *box = root; box; box = box_rec_pre(box, root).next) {
        if (!(box->flags & (BoxFlag_FixedWidth<<axis))) {
            f32 value = 0.0f;
            switch (box->pref_size[axis].kind) {
                case PrefSizeKind_Pixels:
                    value = box->pref_size[axis].value;
                    break;
                case PrefSizeKind_TextContent: {
                    f32 padding = box->pref_size[axis].value;
                    value = measure_text_size(box->text, box->font, box->font_size)[axis];
                    value += padding * 2.0f;
                    break;
                }
            }
            box->fixed_size[axis] = value;
        }
    }
}

void layout_calc_upwards_dependent_sizes(Box *root, Axis axis) {
    for (Box *box = root; box; box = box_rec_pre(box, root).next) {
        if (box->pref_size[axis].kind == PrefSizeKind_ParentPct) {
            f32 pct = box->pref_size[axis].value;

            Box *fixed_parent = nullptr;
            for (Box *p = box->parent; p && !fixed_parent; p = p->parent) {
                if (p->flags & BoxFlag_FixedWidth<<axis
                    || p->pref_size[axis].kind==PrefSizeKind_ParentPct
                    || p->pref_size[axis].kind==PrefSizeKind_Pixels
                    || p->pref_size[axis].kind==PrefSizeKind_TextContent) {
                    fixed_parent = p;
                    break;
                }
            }

            box->fixed_size[axis] = pct * fixed_parent->fixed_size[axis];
        }
    }
}

void layout_calc_downwards_dependent_sizes(Box *root, Axis axis) {
    BoxRec rec = {};
    for (Box *b = root; b; b = rec.next) {
        rec = box_rec_pre(b, root);
        int pop_index = 0;

        for (Box *box = b; b && pop_index <= rec.pop_count; box = box->parent, pop_index += 1) {
            if (box->pref_size[axis].kind == PrefSizeKind_ChildrenSum) {
                f32 sum = 0.0f;
                for (Box *child = box->first; child; child = child->next) {
                    if (box->child_layout_axis == axis) {
                        sum += child->fixed_size[axis];
                    } else {
                        sum = cu_max(sum, child->fixed_size[axis]);
                    }
                }
                box->fixed_size[axis] = sum;
            }
        }
    }
}

void layout_enforce_constraints(Box *root, Axis axis) {
    for (Box *box = root; box; box = box_rec_pre(box, root).next) {
        //- NOTE: Layers do not get to resolve violation constraints because layers are independent from each other layout-wise so sizes do not get violated at the layer level.
        if (box==ui_state->root || box->flags & BoxFlag_Layer) {
            continue;
        }

        if (!(box->flags & BoxFlag_AllowOverflowX<<axis)) {
            //- NOTE: Fix children sizes along non-layout direction
            if (axis != box->child_layout_axis) {
                f32 allowed_size = box->fixed_size[axis];
                for (Box *child = box->first; child; child = child->next) {
                    f32 child_size = child->fixed_size[axis];

                    f32 fixup = child_size - allowed_size;
                    f32 max_fixup = child_size;
                    fixup = cu_clamp(0, fixup, max_fixup);

                    if (fixup > 0) {
                        // printf("NL axis:%d box:%lld: child_size:%f allowed:%f\n", axis, box->key, child_size, allowed_size);
                        child->fixed_size[axis] -= fixup;
                    }
                }
            }

            //- NOTE: Fix children sizes along layout direction
            if (axis == box->child_layout_axis) {
                f32 allowed_size = box->fixed_size[axis];
                f32 total_size = 0;
                f32 total_weighted_size = 0;
                for (Box *child = box->first; child; child = child->next) {
                    total_size += child->fixed_size[axis];
                    total_weighted_size += child->fixed_size[axis] * (1-child->pref_size[axis].strictness);
                }

                f32 violation = total_size - allowed_size;

                if (violation > 0 && total_weighted_size > 0) {
                    // printf("L axis:%d box:%lld total:%f allowed:%f\n", axis, box->key, total_size, allowed_size);
                    Array<f32> children_fixups;
                    {
                        int child_index = 0;
                        for (Box *child = box->first; child; child = child->next, child_index += 1) {
                            f32 fixup = child->fixed_size[axis] * (1-child->pref_size[axis].strictness);
                            fixup = cu_clamp_bot(0, fixup);
                            children_fixups.add(fixup);
                        }
                    }

                    {
                        int child_index = 0;
                        for (Box *child = box->first; child; child = child->next, child_index += 1) {
                            f32 fixup = children_fixups[child_index];
                            f32 fixup_pct = violation / total_weighted_size;
                            fixup_pct = cu_clamp(0, fixup_pct, 1);
                            child->fixed_size[axis] -= fixup * fixup_pct;
                        }
                    }

                    children_fixups.release();
                }
            }
        }

        //- NOTE: Only on overflow
        // for (Box *child = box->first; child; child = child->next) {
        //     if (child->pref_size[axis].kind == PrefSizeKind_ParentPct)  {
        //         child->fixed_size[axis] = box->fixed_size[axis] * child->pref_size[axis].value;
        //     }
        // }
    }
}

void layout_place_boxes(Box *root, Axis axis) {
    for (Box *box = root; box; box = box_rec_pre(box, root).next) {
        f32 layout_position = 0.0f;
        f32 gap = 0.0f;

        if (box->child_layout_axis == axis) {
            switch (box->child_alignment[axis]) {
                case Alignment_Start:
                    layout_position = 0.0f;
                    break;
                case Alignment_Center: {
                    f32 actual_content_size = 0.0f;
                    for (Box *child = box->first; child; child = child->next) {
                        actual_content_size += child->fixed_size[axis];
                    }
                    layout_position = (box->fixed_size[axis] - actual_content_size) * 0.5f;
                    break;
                }
                case Alignment_End: {
                    f32 actual_content_size = 0.0f;
                    for (Box *child = box->first; child; child = child->next) {
                        actual_content_size += child->fixed_size[axis];
                    }
                    layout_position = box->fixed_size[axis] - actual_content_size;
                    break;
                }
                case Alignment_SpaceBetween: {
                    f32 children_sum = 0.0f;
                    for (Box *child = box->first; child; child = child->next) {
                        children_sum += child->fixed_size[axis];
                    }
                    gap = (box->fixed_size[axis] - children_sum) / cu_clamp_bot(1, box->child_count);
                    break;
                }
            }
        }

        for (Box *child = box->first; child; child = child->next) {
            if (!(child->flags & (BoxFlag_FloatingX<<axis))) {
                if (box->child_layout_axis == axis) {
                    child->fixed_position[axis] = layout_position;

                    switch (box->child_alignment[axis]) {
                        case Alignment_Start:
                        case Alignment_Center:
                            layout_position += child->fixed_size[axis];
                            break;
                        case Alignment_End:
                            layout_position += child->fixed_size[axis];
                            break;
                        case Alignment_SpaceBetween:
                            layout_position += child->fixed_size[axis] + gap;
                            break;
                    }
                } else {
                    switch (box->child_alignment[axis]) {
                        case Alignment_Start:
                            layout_position = 0.0f;
                            break;
                        case Alignment_Center:
                            layout_position = (box->fixed_size[axis] - child->fixed_size[axis]) * 0.5f;
                            break;
                        case Alignment_End:
                            layout_position = box->fixed_size[axis] - child->fixed_size[axis];
                            break;
                    }

                    child->fixed_position[axis] = layout_position;
                }
            }

            child->rect.tl[axis] = box->rect.tl[axis] + child->fixed_position[axis];
            child->rect.br[axis] = child->rect.tl[axis] + child->fixed_size[axis];
        }
    }
}

void apply_layout() {
    ui_state->blacklist_rects.reset();

    Box *root = ui_state->root;
    for (Box *layer = root->first; layer; layer = layer->next) {
        layer->rect.tl = root->rect.tl + layer->fixed_position;
        layer->rect.br = layer->rect.tl + layer->fixed_size;
    }

    for (Axis axis = Axis_X; axis <= Axis_Y; axis = (Axis)(axis + 1)) {
        layout_calc_fixed_sizes(root, axis);
        layout_calc_upwards_dependent_sizes(root, axis);
        layout_calc_downwards_dependent_sizes(root, axis);
        layout_enforce_constraints(root, axis);
        layout_place_boxes(root, axis);
    }

    for (Box *layer = root->first; layer; layer = layer->next) {
        Rect blacklist_rect = {};
        //NOTE: Blacklist whole container if it has a background.
        // Otherwise get the minimum bounding rectangle that covers all its children.
        if (layer->flags & BoxFlag_DrawBackground) {
            blacklist_rect = layer->rect;
        } else {
            if (layer->first) {
                blacklist_rect = layer->first->rect;
            }
            for (Box *box = layer->first; box; box = box->next) {
                blacklist_rect.left = cu_min(blacklist_rect.left, box->rect.left);
                blacklist_rect.right = cu_max(blacklist_rect.right, box->rect.right);
                blacklist_rect.top = cu_min(blacklist_rect.top, box->rect.top);
                blacklist_rect.bottom = cu_max(blacklist_rect.bottom, box->rect.bottom);
            }
        }

        ui_state->blacklist_rects.add(blacklist_rect);
        // printf("blacklist rect %d: ", layer->depth);
        // printf("%f %f %f %f\n", blacklist_rect.left, blacklist_rect.top, blacklist_rect.right, blacklist_rect.bottom);
    }
}

void per_frame_update(Vector2 render_dimension, f32 frame_delta, Array<os::Event*> &events) {
    cu_local_persist Font *default_font;

    if (!ui_state) {
        Arena *ui_arena = make_arena();
        Allocator allocator = arena_allocator(ui_arena);
        ui_state = New(UIState, allocator)();
        ui_state->allocator = allocator;
        ui_state->arena = ui_arena;
        ui_state->build_arenas[0] = make_arena();
        ui_state->build_arenas[1] = make_arena();
        ui_draw_data = New(DrawData, ui_state->allocator);

        u8 white_bitmap[] = {
            0xFF, 0xFF, 0xFF, 0xFF
        };
        ui_draw_data->fallback_texture = texture_create(white_bitmap, 2, 2, TextureFormat::R8_UNorm);

        default_font = font_create(STRZ("fonts/seguisb.ttf"), 20);

        u32 icon_font_glyphs[] = { 0, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90 };

        icon_font = font_create(STRZ("fonts/icons.ttf"), 20, icon_font_glyphs, cu_count_of(icon_font_glyphs));


        ui_state->mx_last_frame = ui_state->my_last_frame = ui_state->mx = ui_state->my = -1;

        ui_state->layout_axis_stack.default_value = Axis_Y;
        ui_state->font_stack.default_value = default_font;
        ui_state->child_alignment_x_stack.default_value = Alignment_Start;
        ui_state->child_alignment_y_stack.default_value = Alignment_Start;
        ui_state->text_alignment_stack.default_value = Alignment_Start;
    }

    ui_state->frame_delta = frame_delta;

    ui_state->events = events;

    for (os::Event *event : events) {
        MouseButtonKind mouse_kind = (MouseButtonKind)0;
        if (event->key == Keycode::MouseLeft) mouse_kind = MouseButtonKind_Left;
        else if (event->key == Keycode::MouseMiddle) mouse_kind = MouseButtonKind_Middle;
        else if (event->key == Keycode::MouseRight) mouse_kind = MouseButtonKind_Right;

        switch (event->type) {
            case os::Event::MouseRelease:
                ui_state->active_box_key[mouse_kind] = 0;
                break;

            case os::Event::MouseMove:
                ui_state->mx_last_frame = ui_state->mx;
                ui_state->my_last_frame = ui_state->my;
                ui_state->mx = event->mx;
                ui_state->my = event->my;
                break;

            case os::Event::MouseLeave:
                ui_state->mx_last_frame = ui_state->my_last_frame = ui_state->mx = ui_state->my = -1;
                break;
        }
    }

    if (!os::main_window->focus_active) {
        ui_state->mx_last_frame = ui_state->my_last_frame = ui_state->mx = ui_state->my = -1;
    }

    if (os::main_window->is_minimized) {
        ui_state->hot_box_key = 0;
        ui_state->active_box_key[MouseButtonKind_Left] = 0;
        ui_state->active_box_key[MouseButtonKind_Middle] = 0;
        ui_state->active_box_key[MouseButtonKind_Right] = 0;
    }

    ui_state->depth_counter = 0;

    push_layout_axis(Axis_Y);
    push_font(default_font);
    push_font_size(14);
    push_pref_width(pixel_size(100.f, 1.f));
    push_pref_height(pixel_size(100.f, 1.f));
    push_background_color(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    push_border_color(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    push_text_color(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    push_hot_color(Vector4(0.0f, 0.47f, 0.65f, 1.0f));
    push_hot_color(Vector4(0.0f, 0.47f, 0.65f, 1.0f));
    push_active_color(Vector4(0.93f, 0.14f, 0.59f, 1.0f));

    set_next_fixed_width(render_dimension.x);
    set_next_fixed_height(render_dimension.y);
    set_next_fixed_x(0);
    set_next_fixed_y(0);
    Box *root = box_create(BoxFlag_Clip, STRZ("#~Root"));
    root->rect.tl = Vector2(0, 0);
    root->rect.br = root->rect.tl + render_dimension;
    ui_state->root = root;

    push_parent(root);
}

void end_frame() {
    Box *root = ui_state->root;

    os::main_window->current_cursor = os::Cursor_Arrow;

    Box *hot_box = find_box(ui_state->hot_box_key);
    if (hot_box) {
        if (!mouse_in_rect(hot_box->rect)) {
            ui_state->hot_box_key = 0;
        }

        os::main_window->current_cursor = hot_box->hover_cursor;
    }

    if (!key_match(ui_state->focus_box_key, key_zero())) {
        Box *focus_box = find_box(ui_state->focus_box_key);
        if (focus_box) {
            for (os::Event *evt : ui_state->events) {
                if (evt->type == os::Event::MousePress) {
                    Vector2 pt = Vector2(evt->mx, evt->my);
                    if (!point_in_rect(pt, focus_box->rect)) {
                        ui_state->focus_box_key = key_zero();
                    }
                }
            }
        }
    }

    f32 dt = ui_state->frame_delta;

    f32 hot_rate = 24.0f * dt;
    f32 active_rate = 24.0f * dt;

    for (auto it : ui_state->persistent_map) {
        Box *box = it.second;
        bool is_hot = key_match(ui_state->hot_box_key, box->key);
        bool is_active = key_match(ui_state->active_box_key[MouseButtonKind_Left], box->key);

        box->hot_t += hot_rate * ((f32)is_hot - box->hot_t);
        box->active_t += active_rate * ((f32)is_active - box->active_t);
    }

    apply_layout();

    draw_layout();


    UI_CLEAR_STACKS;

    ui_state->build_index++;
}

void set_current_draw_batch(DrawBatch *batch) {
    ui_draw_data->current_batch = batch;
}

DrawBatch *current_draw_batch() {
    DrawBatch *batch = ui_draw_data->current_batch;
    return batch;
}

DrawBatch *draw_batch_new() {
    DrawBatch *batch = New(DrawBatch, arena_allocator(get_build_arena()));
    batch->vertices_index = ui_draw_data->vertices.count;
    batch->vertex_count = 0;
    return batch;
}

DrawBatch *draw_batch_create() {
    DrawBatch *batch = draw_batch_new();
    ui_draw_data->batches.add(batch);
    return batch;
}

void draw_batch_copy(DrawBatch *dst, DrawBatch *src) {
    dst->texture = src->texture;
    dst->clip = src->clip;
}

void draw_set_texture(Texture *texture) {
    DrawBatch *batch = current_draw_batch();
    if (batch->texture != texture) {
        DrawBatch *new_batch = draw_batch_create();
        draw_batch_copy(new_batch, batch);
        set_current_draw_batch(new_batch);
    }
}

bool rect_equals(Rect a, Rect b) {
    return a.x0 == b.x0 && a.y0 == b.y0 && a.x1 == b.x1 && a.y1 == b.y1;
}

void draw_set_clip(Rect clip) {
    DrawBatch *batch = current_draw_batch();
    if (rect_equals(batch->clip, clip)) {
        DrawBatch *new_batch = draw_batch_create();
        draw_batch_copy(new_batch, batch);
        set_current_draw_batch(new_batch);
    }
}

void draw_batch_params(Texture *texture, Rect clip) {
    DrawBatch *batch = current_draw_batch();
    if (batch==nullptr || batch->texture != texture || !rect_equals(batch->clip, clip)) {
        DrawBatch *new_batch = draw_batch_create();
        if (batch) {
            draw_batch_copy(new_batch, batch);
        }
        new_batch->texture = texture;
        new_batch->clip = clip;
        set_current_draw_batch(new_batch);
    }
}

void draw_vertex(Vertex v) {
    DrawBatch *batch = current_draw_batch();
    ui_draw_data->vertices.add(v);
    batch->vertex_count++;
}

void draw_rect(Rect rect, Vector4 color) {
    Vertex bl = Vertex(Vector2(rect.left, rect.bottom), Vector2(0, 0), color);
    Vertex br = Vertex(rect.br,                         Vector2(0, 0), color);
    Vertex tr = Vertex(Vector2(rect.right, rect.top),   Vector2(0, 0), color);
    Vertex tl = Vertex(rect.tl,                         Vector2(0, 0), color);
    draw_vertex(bl);
    draw_vertex(tr);
    draw_vertex(br);
    draw_vertex(bl);
    draw_vertex(tl);
    draw_vertex(tr);
}

void draw_rect(Rect dst, Rect src, Vector4 color) {
    Vertex bl = Vertex(Vector2(dst.left, dst.bottom), Vector2(src.left, src.bottom), color);
    Vertex br = Vertex(dst.br,                        src.br,                        color);
    Vertex tr = Vertex(Vector2(dst.right, dst.top),   Vector2(src.right, src.top),   color);
    Vertex tl = Vertex(dst.tl,                        src.tl,                        color);
    draw_vertex(bl);
    draw_vertex(tr);
    draw_vertex(br);
    draw_vertex(bl);
    draw_vertex(tl);
    draw_vertex(tr);
}

void draw_text(String text, Rect bounds, Vector2 position, Font *font, Vector4 color, f32 size, f32 max_x, Vector2 text_size, DrawTextFlags flags) {
    f32 dpi = os::main_window->dpi;
    // f32 line_height = size * (dpi / 72.f);
    f32 scale = size / font->size;
    draw_set_texture(font->texture);

    bool truncate = flags & DrawTextFlag_Truncate;
    const String ellipses = STRZ("...");
    const f32 ellipses_width = 0.0f;
    bool trailer_enabled = false;
    if (truncate) {
        measure_text_size(ellipses, font, size).x;
        trailer_enabled = text_size.x > max_x && ellipses_width < max_x;
    }

    Vector2 cursor = position;
    for (isize i = 0; i < text.len; i++) {
        u32 code = (u32)text.text[i];
        GlyphMetrics *g = font_get_glyph_metrics(font, code);

        if (g) {
            Rect dst;
            dst.left = cursor.x + g->lsb * scale;
            dst.right = dst.left + g->bw * scale;
            dst.top = cursor.y + (font->ascent + g->y_off) * scale;
            dst.bottom = dst.top + g->bh * scale;

            //- NOTE: Truncate text
            if (trailer_enabled) {
                if (dst.right > bounds.x1-ellipses_width) {
                    draw_text(ellipses, bounds, cursor, font, color, size, 1000, Vector2(0, 0), flags);
                    break;
                }
            }

            Rect src = Rect(
                g->bl/font->width,
                g->bt/font->height,
                (g->bl + g->bw) / font->width,
                (g->bt + g->bh) / font->height
            );

            draw_rect(dst, src, color);
        }

        if (code == '\n') {
            cursor.x = position.x;
            cursor.y += font->line_skip * scale;
        } else {
            cursor.x += g ? g->advance * scale : 0;
        }
    }
}

Vector2 get_box_text_position(Box *box) {
    Vector2 padding = Vector2::ZERO;
    if (box->pref_size[Axis_X].kind == PrefSizeKind_TextContent) {
        padding.x = box->pref_size[Axis_X].value;
    }
    if (box->pref_size[Axis_Y].kind == PrefSizeKind_TextContent) {
        padding.y = box->pref_size[Axis_Y].value;
    }

    Vector2 pos = {};
    switch (box->text_alignment) {
        case Alignment_Start:
            pos = box->rect.tl + padding;
            break;
        case Alignment_Center: {
            Vector2 text_size = measure_text_size(box->text, box->font, box->font_size);
            pos.x = (box->rect.tl.x + box->rect.br.x) * 0.5f - ((text_size.x + padding.x) * 0.5f);
            pos.y = box->rect.tl.y + padding.y;
            break;
        }
        case Alignment_End: {
            Vector2 text_size = measure_text_size(box->text, box->font, box->font_size);
            pos = box->rect.br - (text_size + padding);
            break;
        }
    }
    return pos;
}


void draw_layout() {
    Box *root = ui_state->root;

    DrawData *draw_data = ui_draw_data;
    draw_data->current_batch = nullptr;
    draw_data->vertices.reset();
    draw_data->batches.release();

    for (Box *box = root; box; box = box_rec_pre(box, root).next) {
        DrawData *draw_data = ui_draw_data;

        Rect clip_rect = {};
        for (Box *b = box; b; b = b->parent) {
            if (b->flags & BoxFlag_Clip) {
                clip_rect = b->rect;
                break;
            }
        }

        draw_batch_params(box->font->texture, clip_rect);

        if (box->flags & BoxFlag_DrawBackground) {
            Vector4 background_color = box->background_color;
            bool mouse_active = false;

            if (box->flags & BoxFlag_DrawHotEffects && key_match(ui_state->hot_box_key, box->key)) {
                background_color = box->hot_color * box->hot_t;
                background_color.w = box->hot_color.w;
            }

            if (box->flags & BoxFlag_DrawActiveEffects) {
                cu_foreach_enum_val(MouseButtonKind, mouse_kind) {
                    Key active_key = ui_state->active_box_key[mouse_kind];
                    if (key_match(active_key, box->key)) {
                        background_color = box->active_color * box->active_t;
                        background_color.w = box->active_color.w;
                        break;
                    }
                }
            }

            if (key_match(ui_state->focus_box_key, box->key)) {
                background_color = box->focus_color;
            }


            draw_rect(box->rect, background_color);
        }

        if (box->flags & BoxFlag_DrawBorder) {
            Rect border_rect = box->rect;
            box->rect.x0 -= 1.0f;
            box->rect.y0 -= 1.0f;
            box->rect.x1 += 1.0f;
            box->rect.y1 += 1.0f;

            draw_rect(box->rect, box->border_color);
        }

        if (box->flags & (BoxFlag_DrawTop|BoxFlag_DrawBottom|BoxFlag_DrawLeft|BoxFlag_DrawRight)) {
            Vector4 border_color = box->border_color;
            f32 half_thickness = 0.5f;
            Rect r = box->rect;

            if (box->flags & BoxFlag_DrawTop) {
                draw_rect(Rect(r.x0, r.y0, r.x1, r.y0 + 2*half_thickness), border_color);
            }

            if (box->flags & BoxFlag_DrawBottom) {
                draw_rect(Rect(r.x0, r.y1 - 2*half_thickness, r.x1, r.y1), border_color);
            }

            if (box->flags & BoxFlag_DrawLeft) {
                draw_rect(Rect(r.x0, r.y0, r.x0 + 2*half_thickness, r.y1), border_color);
            }

            if (box->flags & BoxFlag_DrawRight) {
                draw_rect(Rect(r.x1 - 2*half_thickness, r.y0, r.x1, r.y1), border_color);
            }
        }

        if (box->flags & BoxFlag_DrawText) {
            Vector2 text_position = get_box_text_position(box);
            f32 max_x = box->rect.x1 - text_position.x;
            Vector2 text_size = measure_text_size(box->text, box->font, box->font_size);
            DrawTextFlags flags = DrawTextFlag_Default;
            if (!(box->flags & BoxFlag_DisableTruncateText)) {
                flags |= DrawTextFlag_Truncate;
            }
            draw_text(box->text, box->rect, text_position, box->font, box->text_color, box->font_size, max_x, text_size, flags);
        }

        if (box->draw_proc) {
            box->draw_proc(box, box->draw_data);
        }
    }
}

Signal signal_from_box(Box *box) {
    Signal signal = {};
    signal.box = box;
    signal.scroll.x = 0.f;
    signal.scroll.y = 0.f;

    Vector2 mouse = get_mouse_cursor();
    bool mouse_over = mouse_in_rect(box->rect);
    bool hover = false;
    if (mouse_over && (box->flags & BoxFlag_MouseClickable)) {
        hover = true;
        Box *layer_cont = find_layer_container(box);
        for (int d = layer_cont->depth + 1; d < ui_state->blacklist_rects.count; d++) {
            Rect blacklist_rect = ui_state->blacklist_rects[d];
            if (point_in_rect(mouse, blacklist_rect)) {
                hover = false;
                break;
            }
        }
    }

    if (hover && !key_match(ui_state->hot_box_key, box->key)) {
        ui_state->hot_box_key = box->key;
        box->hot_t = 0;
    }

    bool clicked = false;
    bool pressed = false;
    bool focused = false;
    for (int i = 0; i < ui_state->events.count; i++) {
        os::Event *evt = ui_state->events[i];

        MouseButtonKind mouse_kind = (MouseButtonKind)0;
        if (evt->key == Keycode::MouseLeft) mouse_kind = MouseButtonKind_Left;
        else if (evt->key == Keycode::MouseMiddle) mouse_kind = MouseButtonKind_Middle;
        else if (evt->key == Keycode::MouseRight) mouse_kind = MouseButtonKind_Right;

        switch (evt->type) {
            case os::Event::MousePress: {
                if (box->flags & BoxFlag_MouseClickable && hover) {
                    ui_state->active_box_key[mouse_kind] = box->key;
                    box->active_t = 0;
                    clicked = true;
                    if (box->flags & BoxFlag_ClickToFocus) {
                        ui_state->focus_box_key = box->key;
                        focused = true;
                    }
                }
                break;
            }

            case os::Event::MouseMove:
                if (box->flags & BoxFlag_MouseClickable && key_match(ui_state->active_box_key[mouse_kind], box->key)) {
                    signal.dragging = true;
                }
                break;

            case os::Event::MouseWheel:
                if (box->flags & BoxFlag_Scroll && mouse_over) {
                    if (evt->scroll.x < 0) {
                        signal.scroll.x = -1.f;
                    } else if (evt->scroll.x > 0) {
                        signal.scroll.x = 1.f;
                    }
                    if (evt->scroll.y < 0) {
                        signal.scroll.y = 1.f;
                    } else if (evt->scroll.y > 0) {
                        signal.scroll.y = -1.f;
                    }
                }
                break;
        }
    }

    signal.hover = hover;
    signal.clicked = clicked;
    signal.pressed = pressed;
    signal.focused = focused;
    return signal;
}



};
