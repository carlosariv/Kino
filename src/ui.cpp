#include <assert.h>
#include <spdlog/spdlog.h>

#include "string.h"
#include "ui.h"
#include "texture.h"
#include "math.h"
#include "utils.h"


namespace ui {

UIState *ui_state;
DrawData *ui_draw_data;
Font *icon_font;

Arena *get_build_arena() {
    return ui_state->build_arenas[ui_state->build_index%cu_count_of(ui_state->build_arenas)];
}

char *icon_kind_strings[] = {
    "W",
    "x",
    "!",
    "1",
    "U",
    "D",
    "L",
    "R",
    "9",
    "0",
    "7",
    "8",
    "{",
    "}",
    "C",
    "F",
    "#",
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

bool is_hot_box_key(Key key) {
    return ui_state->hot_box_key == key;
}

bool is_active_box_key(Key key) {
    return ui_state->active_box_key == key;
}

void set_hot_box_key(Key key) {
    ui_state->hot_box_key = key;
}

void set_active_box_key(Key key) {
    ui_state->active_box_key = key;
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
}


void clear_params() {
    ui_state->parent_stack.clear();
    ui_state->fixed_x_stack.clear();
    ui_state->fixed_y_stack.clear();
    ui_state->fixed_width_stack.clear();
    ui_state->fixed_height_stack.clear();
    ui_state->font_stack.clear();
    ui_state->font_size_stack.clear();
    ui_state->child_alignment_x_stack.clear();
    ui_state->child_alignment_y_stack.clear();
    ui_state->text_alignment_stack.clear();
    ui_state->layout_axis_stack.clear();
    ui_state->pref_width_stack.clear();
    ui_state->pref_height_stack.clear();
    ui_state->background_color_stack.clear();
    ui_state->text_color_stack.clear();
    ui_state->hot_color_stack.clear();
}

void auto_pop_params() {
    ui_state->parent_stack.auto_pop();
    ui_state->fixed_x_stack.auto_pop();
    ui_state->fixed_y_stack.auto_pop();
    ui_state->fixed_width_stack.auto_pop();
    ui_state->fixed_height_stack.auto_pop();
    ui_state->font_stack.auto_pop();
    ui_state->font_size_stack.auto_pop();
    ui_state->child_alignment_x_stack.auto_pop();
    ui_state->child_alignment_y_stack.auto_pop();
    ui_state->text_alignment_stack.auto_pop();
    ui_state->layout_axis_stack.auto_pop();
    ui_state->pref_width_stack.auto_pop();
    ui_state->pref_height_stack.auto_pop();
    ui_state->background_color_stack.auto_pop();
    ui_state->text_color_stack.auto_pop();
    ui_state->hot_color_stack.auto_pop();
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
    bool is_transient = (key == 0);
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
    box->text_color = top_text_color();
    box->hot_color = Vector4(0.0f, 0.47f, 0.65f, 1.0f);

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

    auto_pop_params();

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

Box *box_create(BoxFlags flags, String string) {
    Box *parent = top_parent();
    Key seed = 0;
    if (parent) {
        seed = get_seed_key(parent);
    }

    String hash_part = string;
    isize hash_start = string_find(string, STRZ("##"));
    if (hash_start != -1) {
        hash_part = substring(string, hash_start, string.len);
    }

    Key key = get_hash_key(seed, hash_part);
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
    f32 scale = size / font->line_skip;
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
            w += g->advance * scale;
        }
    }
    if (w > dim.x) {
        dim.x = w;
    }

    return dim;
}

void layout_calc_fixed_sizes(Box *root, Axis axis) {
    if (!(root->flags & (BoxFlag_FixedWidth<<axis))) {
        f32 value = 0.0f;
        switch (root->pref_size[axis].kind) {
            case PrefSizeKind_Pixels:
                value = root->pref_size[axis].value;
                break;
            case PrefSizeKind_TextContent: {
                f32 padding = root->pref_size[axis].value;
                value = measure_text_size(root->text, root->font, root->font_size)[axis];
                value += padding * 2.0f;
                break;
            }
        }
        root->fixed_size[axis] = value;
    }

    for (Box *child = root->first; child; child = child->next) {
        layout_calc_fixed_sizes(child, axis);
    }
}

void layout_calc_upwards_dependent_sizes(Box *root, Axis axis) {
    if (root->pref_size[axis].kind == PrefSizeKind_ParentPct) {
        f32 pct = root->pref_size[axis].value;

        Box *fixed_parent = nullptr;
        for (Box *p = root->parent; p && !fixed_parent; p = p->parent) {
            if (p->flags & BoxFlag_FixedWidth<<axis
                || p->pref_size[axis].kind==PrefSizeKind_ParentPct
                || p->pref_size[axis].kind==PrefSizeKind_Pixels
                || p->pref_size[axis].kind==PrefSizeKind_TextContent) {
                fixed_parent = p;
                break;
            }
        }

        root->fixed_size[axis] = pct * fixed_parent->fixed_size[axis];
    }

    for (Box *child = root->first; child; child = child->next) {
        layout_calc_upwards_dependent_sizes(child, axis);
    }
}

void layout_calc_downwards_dependent_sizes(Box *root, Axis axis) {
    for (Box *child = root->first; child; child = child->next) {
        layout_calc_downwards_dependent_sizes(child, axis);
    }

    if (root->pref_size[axis].kind == PrefSizeKind_ChildrenSum) {
        f32 sum = 0.0f;
        for (Box *child = root->first; child; child = child->next) {
            if (root->child_layout_axis == axis) {
                sum += child->fixed_size[axis];
            } else {
                sum = cu_max(sum, child->fixed_size[axis]);
            }
        }
        root->fixed_size[axis] = sum;
    }
}

void layout_place_boxes(Box *root, Axis axis) {
    f32 layout_position = 0.0f;

    if (root->child_layout_axis == axis) {
        switch (root->child_alignment[axis]) {
            case Alignment_Start:
                layout_position = 0.0f;
                break;
            case Alignment_Center: {
                f32 actual_content_size = 0.0f;
                for (Box *child = root->first; child; child = child->next) {
                    actual_content_size += child->fixed_size[axis];
                }
                layout_position = (root->fixed_size[axis] - actual_content_size) * 0.5f;
                break;
            }
            case Alignment_End: {
                layout_position = root->fixed_size[axis];
                if (root->last) layout_position -= root->last->fixed_size[axis];
                break;
            }
        }
    }

    for (Box *child = root->first; child; child = child->next) {
        if (!(child->flags & (BoxFlag_FloatingX<<axis))) {
            if (root->child_layout_axis == axis) {
                child->fixed_position[axis] = layout_position;

                switch (root->child_alignment[axis]) {
                    case Alignment_Start:
                    case Alignment_Center:
                        layout_position += child->fixed_size[axis];
                        break;
                    case Alignment_End:
                        layout_position -= child->fixed_size[axis];
                        break;
                }
            } else {
                switch (root->child_alignment[axis]) {
                    case Alignment_Start:
                        layout_position = 0.0f;
                        break;
                    case Alignment_Center:
                        layout_position = (root->fixed_size[axis] - child->fixed_size[axis]) * 0.5f;
                        break;
                    case Alignment_End:
                        layout_position = root->fixed_size[axis] - child->fixed_size[axis];
                        break;
                }

                child->fixed_position[axis] = layout_position;
            }
        }

        child->rect.tl[axis] = root->rect.tl[axis] + child->fixed_position[axis];
        child->rect.br[axis] = child->rect.tl[axis] + child->fixed_size[axis];
    }

    for (Box *child = root->first; child; child = child->next) {
        layout_place_boxes(child, axis);
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

        default_font = font_create(STRZ("fonts/seguisb.ttf"), 40);

        u32 icon_font_glyphs[] = { 87, 120, 33, 49, 85, 68, 76, 82, 57, 48, 55, 56, 123, 125, 67, 70, 35 };

        icon_font = font_create(STRZ("fonts/icons.ttf"), 24, icon_font_glyphs, cu_count_of(icon_font_glyphs));


        ui_state->mx_last_frame = ui_state->my_last_frame = ui_state->mx = ui_state->my = -1;

        ui_state->layout_axis_stack.default_value = Axis_Y;
        ui_state->font_stack.default_value = default_font;
        ui_state->child_alignment_x_stack.default_value = Alignment_Start;
        ui_state->child_alignment_y_stack.default_value = Alignment_Start;
        ui_state->text_alignment_stack.default_value = Alignment_Start;
    }

    ui_state->events = events;

    Box *focus_active_box = find_box(ui_state->active_box_key);

    for (os::Event *event : events) {
        switch (event->type) {
            case os::Event::MouseRelease:
                if (focus_active_box) {
                    set_active_box_key(0);
                }
                break;

            case os::Event::MouseMove:
                ui_state->mx_last_frame = ui_state->mx;
                ui_state->my_last_frame = ui_state->my;
                ui_state->mx = event->mx;
                ui_state->my = event->my;
                break;
            case os::Event::MouseLeave:
                printf("MOSUE LEAVE\n");
                ui_state->mx_last_frame = ui_state->my_last_frame = ui_state->mx = ui_state->my = -1;
                break;
        }
    }

    if (!os::main_window->focus_active) {
        ui_state->mx_last_frame = ui_state->my_last_frame = ui_state->mx = ui_state->my = -1;
    }

    ui_state->depth_counter = 0;

    push_layout_axis(Axis_Y);
    push_font(default_font);
    push_font_size(24);
    push_pref_width(pref_size_px(100.0f));
    push_pref_height(pref_size_px(100.0f));
    push_background_color(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    push_text_color(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    push_hot_color(Vector4(0.0f, 0.47f, 0.65f, 1.0f));

    set_next_fixed_width(render_dimension.x);
    set_next_fixed_height(render_dimension.y);
    set_next_fixed_x(0);
    set_next_fixed_y(0);
    Box *root = box_create(BoxFlag_FixedSize, STRZ("#~Root"));
    ui_state->root = root;

    push_parent(root);
}

void end_frame() {
    Box *root = ui_state->root;

    Box *hot_box = find_box(ui_state->hot_box_key);
    if (hot_box) {
        if (!mouse_in_rect(hot_box->rect)) {
            set_hot_box_key(0);
        }
    }

    Box *focus_active_box = find_box(ui_state->active_box_key);

    apply_layout();

    draw_layout();

    clear_params();

    ui_state->build_index++;
}

void set_current_draw_batch(DrawBatch *batch) {
    ui_draw_data->current_batch = batch;
}

DrawBatch *current_draw_batch() {
    DrawBatch *batch = ui_draw_data->current_batch;
    return batch;
}

DrawBatch *draw_batch_new(Texture *texture) {
    DrawBatch *batch = New(DrawBatch, arena_allocator(get_build_arena()));
    batch->texture = texture;
    batch->vertices_index = ui_draw_data->vertices.count;
    batch->vertex_count = 0;
    return batch;
}

DrawBatch *draw_batch_create() {
    DrawBatch *batch = draw_batch_new(nullptr);
    ui_draw_data->batches.add(batch);
    return batch;
}

void draw_set_texture(Texture *texture) {
    DrawBatch *batch = current_draw_batch();
    if (batch == nullptr || batch->texture != texture) {
        batch = draw_batch_create();
        set_current_draw_batch(batch);

        //TODO: Copy previous batch
        batch->texture = texture;
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

void draw_text(String text, Vector2 position, Font *font, Vector4 color, f32 size) {
    f32 scale = size / font->line_skip;
    draw_set_texture(font->texture);

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
            cursor.x += g->advance * scale;
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

void draw_box(Box *box) {
    DrawData *draw_data = ui_draw_data;

    draw_set_texture(box->font->texture);

    if (box->draw_proc) {
        box->draw_proc(box, box->draw_data);
        return;
    }

    if (box->flags & BoxFlag_DrawBackground) {
        Vector4 background_color = box->background_color;
        if (is_hot_box_key(box->key)) {
            background_color = box->hot_color;
        }
        draw_rect(box->rect, background_color);
    }

    for (Box *child = box->first; child; child = child->next) {
        draw_box(child);
    }


    if (box->flags & BoxFlag_DrawText) {
        Vector2 text_position = get_box_text_position(box);
        draw_text(box->text, text_position, box->font, box->text_color, box->font_size);
    }
}

void draw_layout() {
    DrawData *draw_data = ui_draw_data;
    draw_data->vertices.reset();
    draw_data->batches.release();
    set_current_draw_batch(nullptr);

    for (Box *box = ui_state->root->first; box; box = box->next) {
        draw_box(box);
    }
}

Signal signal_from_box(Box *box) {
    Signal signal = {};
    signal.box = box;

    Vector2 mouse = get_mouse_cursor();
    bool mouse_over = mouse_in_rect(box->rect);
    bool hover = false;
    if (mouse_over && (box->flags & BoxFlag_MouseInput)) {
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

    if (hover && !is_hot_box_key(box->key)) {
        set_hot_box_key(box->key);
        ui_state->hot_t = 0;
    }

    bool clicked = false;
    bool pressed = false;
    for (int i = 0; i < ui_state->events.count; i++) {
        os::Event *evt = ui_state->events[i];
        switch (evt->type) {
            case os::Event::MousePress:
                if ((box->flags & BoxFlag_MouseInput) && hover) {
                    set_active_box_key(box->key);
                    ui_state->active_t = 0;
                    clicked = true;
                }
                break;

            case os::Event::MouseMove:
                if ((box->flags & BoxFlag_MouseInput) && is_active_box_key(box->key)) {
                    signal.dragging = true;
                }
                break;
        }
    }

    signal.hover = hover;
    signal.clicked = clicked;
    signal.pressed = pressed;
    return signal;
}


};
