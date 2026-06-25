#pragma once

#include <vector>
#include <unordered_map>

#include "enum_flags.hpp"
#include "base_types.h"
#include "math.h"
#include "os.h"
#include "font.h"

struct ID3D11Buffer;

namespace ui {

struct Box;
typedef u64 Key;

union Rect {
    struct {
        f32 left;
        f32 top;
        f32 right;
        f32 bottom;
    };

    struct {
        Vector2 tl;
        Vector2 br;
    };

    Vector2 elems[2];

    Rect() { }
    Rect(f32 l, f32 t, f32 r, f32 b) : left(l), top(t), right(r), bottom(b) {}
};

enum PrefSizeKind {
    PrefSizeKind_Nil = 0,
    PrefSizeKind_Pixels,
    PrefSizeKind_TextContent,
    PrefSizeKind_ParentPct,
    PrefSizeKind_ChildrenSum,
};

struct PrefSize {
    PrefSizeKind kind;
    f32 value;
};

enum BoxFlags {
    BoxFlag_Default = 0,

    BoxFlag_MouseInput    = (1<<4),
    BoxFlag_KeyboardInput = (1<<5),

    BoxFlag_Layer       = (1<<9),
    BoxFlag_FloatingX   = (1<<10),
    BoxFlag_FloatingY   = (1<<11),
    BoxFlag_FixedWidth  = (1<<12),
    BoxFlag_FixedHeight = (1<<13),

    BoxFlag_DrawBackground = (1<<15),
    BoxFlag_DrawText = (1<<16),
    BoxFlag_DrawBorder = (1<<17),

    BoxFlag_Floating  = (BoxFlag_FloatingX|BoxFlag_FloatingY),
    BoxFlag_FixedSize = (BoxFlag_FixedWidth|BoxFlag_FixedHeight),
};
ENUM_FLAG_OPERATORS(BoxFlags)

#define UI_DRAW_PROC(Name) void Name(ui::Box *box, void *draw_data)
typedef UI_DRAW_PROC(ui_draw_proc_t);
typedef ui_draw_proc_t* DrawProc;

enum Alignment {
    Alignment_Start,
    Alignment_Center,
    Alignment_End
};

struct Box {
    Box *parent;
    Box *next;
    Box *prev;
    Box *first;
    Box *last;

    Key key;
    int layer;

    BoxFlags flags = BoxFlag_Default;
    PrefSize pref_size[2];
    Axis child_layout_axis;
    Alignment child_alignment[2];
    Alignment text_alignment;
    Vector4 text_color;
    Vector4 background_color;
    Vector4 hot_color;
    Font *font;
    f32 font_size;

    void *draw_data;
    DrawProc draw_proc;

    Vector2 fixed_position;
    Vector2 fixed_size;
    Rect rect;
    String text;
    u32 depth;

    Box() {}
};

template<typename T>
struct Param {
    Array<T> stack;
    bool pop_next = false;
    T default_value = {};

    bool is_empty() {
        return stack.count == 0;
    }

    T top() {
        T result = default_value;
        if (stack.count != 0) {
            result = stack[stack.count-1];
        }
        return result;
    }

    void push(T value) {
        stack.add(value);
    }

    void pop() {
        stack.remove();
        pop_next = false;
    }

    void clear() {
        stack.release();
        pop_next = false;
    }

    void set_next(T value) {
        pop_next = true;
        push(value);
    }

    void auto_pop() {
        if (pop_next) {
            pop();
        }
    }
};

struct Vertex {
    Vector2 position;
    Vector2 uv;
    Vector4 color;

    Vertex(Vector2 position, Vector2 uv, Vector4 color) : position(position), uv(uv), color(color) {}
};

struct DrawBatch {
    Texture *texture;
    isize vertices_index;
    isize vertex_count;
};

struct DrawData {
    Texture *fallback_texture;
    Array<DrawBatch*> batches;
    DrawBatch *current_batch;
    Array<Vertex> vertices;

    uint vertex_buffer_cap = 0;
    ID3D11Buffer *vertex_buffer = nullptr;
};

struct Signal {
    Box *box;
    bool hover;
    bool clicked;
    bool pressed;
};

struct UIState {
    Arena *arena;
    Allocator allocator;

    int build_index = 0;
    Arena *build_arenas[2];

    int mx = 0;
    int my = 0;
    int mx_last_frame = 0;
    int my_last_frame = 0;

    Array<os::Event*> events;

    std::unordered_map<Key,Box*> persistent_map;
    Box *root = nullptr;

    Key active_box_key = 0;
    Key hot_box_key = 0;
    f32 hot_t = 0;
    f32 active_t = 0;

    Array<Rect> blacklist_rects;
    u32 depth_counter = 0;

    // Stack Parameters
    Param<Box*> parent_stack;
    Param<f32> fixed_x_stack;
    Param<f32> fixed_y_stack;
    Param<f32> fixed_width_stack;
    Param<f32> fixed_height_stack;
    Param<Font*> font_stack;
    Param<f32> font_size_stack;
    Param<Alignment> child_alignment_x_stack;
    Param<Alignment> child_alignment_y_stack;
    Param<Alignment> text_alignment_stack;
    Param<Axis> layout_axis_stack;
    Param<PrefSize> pref_width_stack;
    Param<PrefSize> pref_height_stack;
    Param<Vector4> background_color_stack;
    Param<Vector4> text_color_stack;
    Param<Vector4> hot_color_stack;
};

extern UIState *ui_state;
extern DrawData *ui_draw_data;

inline PrefSize pref_size_px(f32 px) {
    PrefSize size;
    size.kind = PrefSizeKind_Pixels;
    size.value = px;
    return size;
}

inline PrefSize pref_size_parent(f32 pct) {
    PrefSize size;
    size.kind = PrefSizeKind_ParentPct;
    size.value = pct;
    return size;
}

inline PrefSize pref_size_sum() {
    PrefSize size;
    size.kind = PrefSizeKind_ChildrenSum;
    return size;
}

inline PrefSize pref_size_text(f32 padding) {
    PrefSize size;
    size.kind = PrefSizeKind_TextContent;
    size.value = padding;
    return size;
}

inline Box *top_parent() { return ui_state->parent_stack.top(); }
inline f32 top_fixed_x() { return ui_state->fixed_x_stack.top(); }
inline f32 top_fixed_y() { return ui_state->fixed_y_stack.top(); }
inline f32 top_fixed_width() { return ui_state->fixed_width_stack.top(); }
inline f32 top_fixed_height() { return ui_state->fixed_height_stack.top(); }
inline Font *top_font() { return ui_state->font_stack.top(); }
inline f32 top_font_size() { return ui_state->font_size_stack.top(); }
inline Axis top_layout_axis() { return ui_state->layout_axis_stack.top(); }
inline Alignment top_child_alignment_x() { return ui_state->child_alignment_x_stack.top(); }
inline Alignment top_child_alignment_y() { return ui_state->child_alignment_y_stack.top(); }
inline Alignment top_text_alignment() { return ui_state->text_alignment_stack.top(); }
inline PrefSize top_pref_width() { return ui_state->pref_width_stack.top(); }
inline PrefSize top_pref_height() { return ui_state->pref_height_stack.top(); }
inline Vector4 top_background_color() { return ui_state->background_color_stack.top(); }
inline Vector4 top_text_color() { return ui_state->text_color_stack.top(); }
inline Vector4 top_hot_color() { return ui_state->hot_color_stack.top(); }

inline void push_parent(Box *v) { ui_state->parent_stack.push(v); }
inline void push_fixed_x(f32 v) { ui_state->fixed_x_stack.push(v); }
inline void push_fixed_y(f32 v) { ui_state->fixed_y_stack.push(v); }
inline void push_fixed_width(f32 v) { ui_state->fixed_width_stack.push(v); }
inline void push_fixed_height(f32 v) { ui_state->fixed_height_stack.push(v); }
inline void push_font(Font *v) { ui_state->font_stack.push(v); }
inline void push_font_size(f32 v) { ui_state->font_size_stack.push(v); }
inline void push_layout_axis(Axis v) { ui_state->layout_axis_stack.push(v); }
inline void push_child_alignment_x(Alignment v) { ui_state->child_alignment_x_stack.push(v); }
inline void push_child_alignment_y(Alignment v) { ui_state->child_alignment_y_stack.push(v); }
inline void push_text_alignment(Alignment v) { ui_state->text_alignment_stack.push(v); }
inline void push_pref_width(PrefSize v) { ui_state->pref_width_stack.push(v); }
inline void push_pref_height(PrefSize v) { ui_state->pref_height_stack.push(v); }
inline void push_background_color(Vector4 v) { ui_state->background_color_stack.push(v); }
inline void push_text_color(Vector4 v) { ui_state->text_color_stack.push(v); }
inline void push_hot_color(Vector4 v) { ui_state->hot_color_stack.push(v); }

inline void set_next_parent(Box *v) { ui_state->parent_stack.set_next(v); }
inline void set_next_fixed_x(f32 v) { ui_state->fixed_x_stack.set_next(v); }
inline void set_next_fixed_y(f32 v) { ui_state->fixed_y_stack.set_next(v); }
inline void set_next_fixed_width(f32 v) { ui_state->fixed_width_stack.set_next(v); }
inline void set_next_fixed_height(f32 v) { ui_state->fixed_height_stack.set_next(v); }
inline void set_next_font(Font *v) { ui_state->font_stack.set_next(v); }
inline void set_next_font_size(f32 v) { ui_state->font_size_stack.set_next(v); }
inline void set_next_child_alignment_x(Alignment v) { ui_state->child_alignment_x_stack.set_next(v); }
inline void set_next_child_alignment_y(Alignment v) { ui_state->child_alignment_y_stack.set_next(v); }
inline void set_next_text_alignment(Alignment v) { ui_state->text_alignment_stack.set_next(v); }
inline void set_next_layout_axis(Axis v) { ui_state->layout_axis_stack.set_next(v); }
inline void set_next_pref_width(PrefSize v) { ui_state->pref_width_stack.set_next(v); }
inline void set_next_pref_height(PrefSize v) { ui_state->pref_height_stack.set_next(v); }
inline void set_next_background_color(Vector4 v) { ui_state->background_color_stack.set_next(v); }
inline void set_next_text_color(Vector4 v) { ui_state->text_color_stack.set_next(v); }
inline void set_next_hot_color(Vector4 v) { ui_state->hot_color_stack.set_next(v); }

inline void pop_parent() { ui_state->parent_stack.pop(); }
inline void pop_fixed_x() { ui_state->fixed_x_stack.pop(); }
inline void pop_fixed_y() { ui_state->fixed_y_stack.pop(); }
inline void pop_fixed_width() { ui_state->fixed_width_stack.pop(); }
inline void pop_fixed_height() { ui_state->fixed_height_stack.pop(); }
inline void pop_font() { ui_state->font_stack.pop(); }
inline void pop_font_size() { ui_state->font_size_stack.pop(); }
inline void pop_child_alignment_x() { ui_state->child_alignment_x_stack.pop(); }
inline void pop_child_alignment_y() { ui_state->child_alignment_y_stack.pop(); }
inline void pop_text_alignment() { ui_state->text_alignment_stack.pop(); }
inline void pop_layout_axis() { ui_state->layout_axis_stack.pop(); }
inline void pop_pref_width() { ui_state->pref_width_stack.pop(); }
inline void pop_pref_height() { ui_state->pref_height_stack.pop(); }
inline void pop_background_color() { ui_state->background_color_stack.pop(); }
inline void pop_text_color() { ui_state->text_color_stack.pop(); }
inline void pop_hot_color() { ui_state->hot_color_stack.pop(); }

Box *box_create(BoxFlags flags, Key key);
Box *box_create(BoxFlags flags, String string);
Signal signal_from_box(Box *box);

void per_frame_update(Vector2 render_dimension, f32 frame_delta, Array<os::Event*> &events);
void end_frame();


DrawBatch *draw_batch_create();
void draw_set_texture(Texture *texture);
void draw_vertex(Vertex v);
void draw_rect(Rect rect, Vector4 color);
void draw_rect(Rect dst, Rect src, Vector4 color);
void draw_text(String text, Vector2 position, Font *font, Vector4 color, f32 size);
void draw_box(Box *box);
void draw_layout();

bool is_hot_box_key(Key key);

Vector2 get_box_text_position(Box *box);

};

#define DEFER_LOOP(Start, End) for (int __d_ = ((Start), 1); __d_; __d_=((End), 0))

#define UI_ROW() DEFER_LOOP(ui::row_begin(), ui::row_end())

#define UI_Parent(parent) DEFER_LOOP(ui::push_parent((parent)), ui::pop_parent())
