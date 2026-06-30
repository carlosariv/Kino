#pragma once

#include <vector>
#include <unordered_map>

#include "enum_flags.hpp"
#include "base_types.h"
#include "math.h"
#include "os.h"
#include "font.h"

struct ID3D11Buffer;

#define UI_PARAM_LIST(X) \
X(BoxFlags, flags) \
X(Box*, parent) \
X(f32, fixed_x) \
X(f32, fixed_y) \
X(f32, fixed_width) \
X(f32, fixed_height) \
X(Font*, font) \
X(f32, font_size) \
X(Alignment, child_alignment_x) \
X(Alignment, child_alignment_y) \
X(Alignment, text_alignment) \
X(Axis, layout_axis) \
X(PrefSize, pref_width) \
X(PrefSize, pref_height) \
X(Vector4, background_color) \
X(Vector4, border_color) \
X(Vector4, text_color) \
X(Vector4, hot_color) \
X(Vector4, active_color) \
X(os::Cursor, hover_cursor) \


#define UI_PARAM_DECLARE(Type,Name) Param<Type> Name##_stack;
#define UI_PARAM_DECLS UI_PARAM_LIST(UI_PARAM_DECLARE)

#define UI_PARAM_TOP(Type,Name) inline Type top_ ## Name() { return ui_state->Name ## _stack.top(); }
#define UI_PARAM_TOP_DECLS UI_PARAM_LIST(UI_PARAM_TOP)

#define UI_PARAM_PUSH(Type,Name) inline void push_ ## Name(Type v) { ui_state->Name ## _stack.push(v); }
#define UI_PARAM_PUSH_DECLS UI_PARAM_LIST(UI_PARAM_PUSH)

#define UI_PARAM_SET_NEXT(Type,Name) inline void set_next_ ## Name(Type v) { ui_state->Name ## _stack.set_next(v); }
#define UI_PARAM_SET_NEXT_DECLS UI_PARAM_LIST(UI_PARAM_SET_NEXT)

#define UI_PARAM_POP(Type,Name) inline void pop_ ## Name() { ui_state->Name ## _stack.pop(); }
#define UI_PARAM_POP_DECLS UI_PARAM_LIST(UI_PARAM_POP)

namespace ui {

struct Box;
typedef u64 Key;

enum IconKind {
    IconKind_Null,
    IconKind_Home,
    IconKind_Info,

    IconKind_LeftArrow,
    IconKind_RightArrow,
    IconKind_UpArrow,
    IconKind_DownArrow,

    IconKind_Folder,
    IconKind_FolderOpen,
    IconKind_Document,

    IconKind_Find,
    IconKind_Check,
    IconKind_Warning,
    IconKind_Cancel,
    IconKind_Trash,
    IconKind_ZoomPlus,
    IconKind_ZoomMinus,

    IconKind_WindowRestore,
    IconKind_WindowMaximize,
    IconKind_WindowMinimize,

    IconKind_COUNT
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
    f32 strictness;
};

enum BoxFlags {
    BoxFlag_Default = 0,

    BoxFlag_MouseClickable    = (1<<0),
    BoxFlag_MouseHoverable    = (1<<1),
    BoxFlag_KeyboardClickable = (1<<2),

    BoxFlag_Layer       = (1<<5),
    BoxFlag_FloatingX   = (1<<6),
    BoxFlag_FloatingY   = (1<<7),
    BoxFlag_FixedWidth  = (1<<8),
    BoxFlag_FixedHeight = (1<<9),
    BoxFlag_AllowOverflowX = (1<<10),
    BoxFlag_AllowOverflowY = (1<<11),

    BoxFlag_DrawBackground = (1<<15),
    BoxFlag_DrawText = (1<<16),
    BoxFlag_DrawBorder = (1<<17),
    BoxFlag_DrawHotEffects = (1<<18),
    BoxFlag_DrawActiveEffects = (1<<19),
    BoxFlag_DrawTop = (1<<20),
    BoxFlag_DrawBottom = (1<<21),
    BoxFlag_DrawLeft = (1<<22),
    BoxFlag_DrawRight = (1<<23),

    BoxFlag_Clip = (1<<30),

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
    Alignment_End,
    // Alignment_SpaceBetween,
};

// enum TextAlignment {
//     TextAlign_Start,
//     TextAlign_Center,
//     TextAlign_End,
// };

struct Box {
    Box *parent;
    Box *next;
    Box *prev;
    Box *first;
    Box *last;
    int child_count;

    Key key;
    int layer;

    BoxFlags flags = BoxFlag_Default;
    PrefSize pref_size[2];
    Axis child_layout_axis;
    Alignment child_alignment[2];
    Alignment text_alignment;
    Vector4 text_color;
    Vector4 background_color;
    Vector4 border_color;
    Vector4 hot_color;
    Vector4 active_color;
    Font *font;
    f32 font_size;
    os::Cursor hover_cursor;

    void *draw_data;
    DrawProc draw_proc;

    Vector2 fixed_position;
    Vector2 fixed_size;
    Rect rect;
    String text;
    u32 depth;

    f32 hot_t = 0;
    f32 active_t = 0;

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
    bool dragging;
};

enum MouseButtonKind {
    MouseButtonKind_Left,
    MouseButtonKind_Middle,
    MouseButtonKind_Right,
    MouseButtonKind_COUNT
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

    f32 frame_delta = 0;

    Array<os::Event*> events;

    std::unordered_map<Key,Box*> persistent_map;
    Box *root = nullptr;

    Key hot_box_key = 0;
    Key active_box_key[MouseButtonKind_COUNT] = {};

    Array<Rect> blacklist_rects;
    u32 depth_counter = 0;

    UI_PARAM_DECLS;
};

extern UIState *ui_state;
extern DrawData *ui_draw_data;
extern Font *icon_font;

//- NOTE: UI state parameter function declarations.
UI_PARAM_TOP_DECLS;
UI_PARAM_PUSH_DECLS;
UI_PARAM_SET_NEXT_DECLS;
UI_PARAM_POP_DECLS;


inline PrefSize pixel_size(f32 value, f32 strictness) {
    return { PrefSizeKind_Pixels, value, strictness };
}

inline PrefSize text_em_size(f32 value, f32 strictness) {
    return { PrefSizeKind_Pixels, top_font_size() * value, strictness };
}

inline PrefSize text_size(f32 padding, f32 strictness) {
    return { PrefSizeKind_TextContent, padding, strictness };
}

inline PrefSize children_sum_size(f32 strictness) {
    return { PrefSizeKind_ChildrenSum, 0.f, strictness };
}

inline PrefSize pct_size(f32 value, f32 strictness) {
    return { PrefSizeKind_ParentPct, value, strictness };
}

inline void push_pref_size(Axis axis, PrefSize v) { if (axis==Axis_X) push_pref_width(v); else push_pref_height(v); }
inline void set_next_pref_size(Axis axis, PrefSize v) { if (axis==Axis_X) set_next_pref_width(v); else set_next_pref_height(v); }

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
void draw_text(String text, Rect bounds, Vector2 position, Font *font, Vector4 color, f32 size, f32 max_x, Vector2 text_size);
void draw_box(Box *box);
void draw_layout();


Vector2 measure_text_size(String text, Font *font, f32 size);


Vector2 get_box_text_position(Box *box);


Vector2 get_mouse_cursor();


String string_from_icon_kind(IconKind kind, const char *end);


Vector2 get_rect_size(Rect rect);

Key key_zero();
bool key_match(Key a, Key b);


};

#define UI_ROW() CU_DEFER_LOOP(ui::row_begin(), ui::row_end())

#define UI_Parent(parent) CU_DEFER_LOOP(ui::push_parent((parent)), ui::pop_parent())
