#pragma once

#include <string>

#include "array.h"
#include "base_types.h"
#include "utils.h"
#include "string.h"
#include "vector.h"
#include "enum_flags.hpp"
#include "key_code.h"

namespace os {

struct Event;
struct Window;

typedef u64 Handle;
typedef u64 WallClock;

extern Array<Event*> window_events;
extern Window *main_window;

void set_window_minimized();
void set_window_maximized();
void set_window_close();

void clear_title_bar_client_areas();
void push_title_bar_client_area(Rect rect);

enum ModFlags {
    ModFlag_Nil     = 0,
    ModFlag_Alt     = (1<<0),
    ModFlag_Shift   = (1<<1),
    ModFlag_Control = (1<<2),
};
ENUM_FLAG_OPERATORS(ModFlags);

enum Cursor {
    Cursor_Nil,
    Cursor_Arrow,
    Cursor_Hand,
    Cursor_IBeam,
    Cursor_NS,
    Cursor_EW,
    Cursor_NESW,
    Cursor_NWSE,
    Cursor_Unavailable,
    Cursor_COUNT
};

struct Event {
    enum Type {
        Unknown,
        KeyPress,
        KeyRelease,
        MouseMove,
        MousePress,
        MouseRelease,
        MouseWheel,
        MouseLeave,
        Text,
        Quit,
        Count
    };

    Type type = Type::Unknown;
    Keycode key = Keycode::Nil;
    ModFlags mod_flags = ModFlag_Nil;
    String text = {};
    int mx, my;
    int wheel_delta;

    Event() { }

    Event(Type ty) {
        type = ty;
    }
};

enum FileAttribs {
    FileAttrib_Normal    = (1<<0),
    FileAttrib_ReadOnly  = (1<<1),
    FileAttrib_Directory = (1<<2),
    FileAttrib_Hidden    = (1<<3),
    FileAttrib_System    = (1<<4),
    FileAttrib_Archive   = (1<<5),
    FileAttrib_Device    = (1<<6),
};

struct File {
    Handle handle;
    String file_name;
    u64 file_size;
    u64 create_time;
    u64 access_time;
    u64 write_time;
};

enum FileAccessFlags {
    FileAccessFlag_Read   = (1<<0),
    FileAccessFlag_Write  = (1<<1),
    FileAccessFlag_Create = (1<<2),
    FileAccessFlag_Open   = (1<<3),
    FileAccessFlag_Append = (1<<4),
};
ENUM_FLAG_OPERATORS(FileAccessFlags);

struct Window {
    Handle handle;
    int width = 0;
    int height = 0;
    int last_cursor_x, last_cursor_y;
    bool destroy_next_frame = false;
    bool capture_cursor = false;
    bool focus_active = false;
    bool tracking_mouse = false;
    bool is_minimized = false;
    bool custom_border = false;
    int custom_border_thickness = 0;
    int dpi = 96.f;

    Cursor current_cursor = Cursor_Nil;

    inline bool is_focused() { return focus_active; }
    inline bool should_close() { return destroy_next_frame; }

    void disable_cursor();
    Vector2 get_size();
};

WallClock get_wall_clock();
u64 get_elapsed_ticks(WallClock start, WallClock end);
f64 get_elapsed_seconds(WallClock start, WallClock end);
f64 get_elapsed_milliseconds(WallClock start, WallClock end);

bool is_valid_handle(Handle handle);
u64 get_file_last_write_time(Handle handle);
void close_handle(Handle handle);
Handle open_file(String file_name, FileAccessFlags flags);
String read_entire_file(Handle file_handle);

void poll_events();

Window *window_create(int width, int height);

void set_cursor_pos(Window *window, int xpos, int ypos);

Event *push_event(Event::Type type);
void sleep_ms(int ms);

void get_key(char character, Keycode *code_out, ModFlags *flags_out);

Array<File> list_directory_files(String path);


void push_custom_title_bar(f32 thickness);

};
