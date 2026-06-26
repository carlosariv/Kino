#include <assert.h>
#include "os.h"

#include "buffer.h"
#include "view.h"
#include "kino.h"
#include "base_commands.h"
#include "ui.h"
#include "ui_widgets.h"
#include "os.h"

namespace kino {

State *state;
FileSystem *g_file_system;

Buffer *find_buffer(BufferID id) {
    BufferManager *manager = state->buffer_manager;
    for (Buffer *buffer : manager->buffer_list) {
        if (buffer->id == id) {
            return buffer;
        }
    }
    return nullptr;
}

View *find_view(ViewID id) {
    ViewManager *manager = state->view_manager;
    for (View *view : manager->view_list) {
        if (view->id == id) {
            return view;
        }
    }
    return nullptr;
}

void set_active_view(View *view) {
    state->active_view = view;
}

View *get_active_view() {
    View *view = state->active_view;
    return view;
}

void file_system_load(FileSystem *fs, String path) {
    fs->path = copy_string(path);
    fs->files = os::list_directory_files(path);
}

void init() {
    state = new State;
    state->active_view = nullptr;

    state->view_manager = new ViewManager;
    state->buffer_manager = new BufferManager;

    KeyMap *norm = keymap_create(STRZ("NORMAL"));
    state->normal_map = norm;
    keymap_set(norm, STRZ("i"), insert_mode);
    keymap_set(norm, STRZ("o"), newline_and_insert);

    keymap_set(norm, STRZ("<DOWN>"), move_down);
    keymap_set(norm, STRZ("<UP>"), move_up);
    keymap_set(norm, STRZ("<LEFT>"), move_left);
    keymap_set(norm, STRZ("<RIGHT>"), move_right);

    keymap_set(norm, STRZ("j"), move_down);
    keymap_set(norm, STRZ("k"), move_up);
    keymap_set(norm, STRZ("h"), move_left);
    keymap_set(norm, STRZ("l"), move_right);
    keymap_set(norm, STRZ("gh"), goto_line_start);
    keymap_set(norm, STRZ("gl"), goto_line_end);
    keymap_set(norm, STRZ("gj"), goto_end);
    keymap_set(norm, STRZ("gk"), goto_begin);

    KeyMap *ins = keymap_create(STRZ("INSERT"));
    state->insert_map = ins;
    keymap_set(ins, STRZ("<ESC>"), normal_mode);
    for (char c = 0; c < 127; c++) {
        String str = {(u8 *)&c, 1};
        if (isalnum(c) || ispunct(c)) {
            keymap_set(ins, str, self_insert);
        }
    }
    keymap_set(ins, str_lit("<CR>"), newline);
    keymap_set(ins, str_lit("<S-CR>"), newline);
    keymap_set(ins, str_lit("<SPC>"), self_insert);
    keymap_set(ins, str_lit("<S-SPC>"), self_insert);
    keymap_set(ins, STRZ("<BS>"), backspace);
    keymap_set(ins, STRZ("<UP>"), move_up);
    keymap_set(ins, STRZ("<DOWN>"), move_down);
    keymap_set(ins, STRZ("<LEFT>"), move_left);
    keymap_set(ins, STRZ("<RIGHT>"), move_right);

    View *default_view = view_create();
    Buffer *buffer = buffer_create(STRZ("*scratch*"));
    default_view->bufid = buffer->id;
    set_active_view(default_view);

    FileSystem *file_system = new FileSystem;
    g_file_system = file_system;
    file_system_load(file_system, STRZ("C:/Dev/Kino/src"));
}

void handle_key_press(Key key) {
    View *view = get_active_view();
    KeyMap *map = view->map;

    if (!state->current_key_state) {
        state->current_key_state = map->root;
    }

    KeyNode *current_key = state->current_key_state;
    KeyNode *search = keymap_search(map, current_key, key);

    state->current_key_state = search;

    if (search) {
        if (search->cmd) {
            search->cmd();
            state->current_key_state = nullptr;
        }
    }
}


UI_DRAW_PROC(ui_draw_view);

// void layout_pane(Pane *pane) {
//     return;
    // Pane *parent = pane->parent;
    //
    // if (parent == nullptr) {
    //     Vector2 dim = os::main_window->get_dimension();
    //     pane->dim = dim;
    // } else {
    //     Axis axis = parent->split_axis;
    //     Axis inv_axis = invert_xy(axis);
    //     pane->dim[axis] = pane->pct * parent->dim[axis];
    //     pane->dim[inv_axis] = parent->dim[inv_axis];
   // }
    //
    // View *view = pane->view;
    //
    // printf("%f %f\n", pane->dim.x, pane->dim.y);
    //
    // if (view) {
    //     assert(pane->tl == nullptr);
    //     assert(pane->br == nullptr);
    //
    //     ui::set_next_pref_width(ui::pref_size_px(pane->dim.x));
    //     ui::set_next_pref_height(ui::pref_size_px(pane->dim.y));
    //     ui::set_next_background_color(Vector4(1, 1, 1, 1));
    //     ui::set_next_text_color(Vector4(0, 0, 0, 1));
    //     if (view == state->console_view) {
    //         ui::set_next_background_color(Vector4(0, 1, 0, 1));
    //     }
    //     view->box = ui::box_create(ui::BoxFlag_Clickable|ui::BoxFlag_Pressable, string_fmt("##view_%lld", view->id));
    //     view->box->draw_data = (void *)view;
    //     view->box->draw_proc = ui_draw_view;
    // } else {
    //     ui::set_next_pref_width(ui::pref_size_px(pane->dim.x));
    //     ui::set_next_pref_height(ui::pref_size_px(pane->dim.y));
    //     ui::set_next_layout_axis(pane->split_axis);
    //     ui::Box *container = ui::box_create(ui::BoxFlag_Default, 0);
    //
    //     ui::push_parent(container);
    //     layout_pane(pane->tl);
    //     layout_pane(pane->br);
    //     ui::pop_parent();
    // }
// }

void update(f32 dt, Array<os::Event*> window_events) {
    for (os::Event *evt : window_events) {
        switch (evt->type) {
            case os::Event::KeyPress: {
                KeyMod mod = KeyMod_None;
                mod |= (KeyMod)(KeyMod_Control*((evt->mod_flags&os::ModFlag_Control)!=0));
                mod |= (KeyMod)(KeyMod_Alt*((evt->mod_flags&os::ModFlag_Alt)!=0));
                mod |= (KeyMod)(KeyMod_Shift*((evt->mod_flags&os::ModFlag_Shift)!=0));

                Key key = make_key(evt->key, mod);

                printf("key: %x control:%d shift:%d alt:%d %s\n", key, (mod&KeyMod_Control)!=0, (mod&KeyMod_Shift)!=0, (mod&KeyMod_Alt)!=0, evt->text.text ? (char *)evt->text.text : "");

                state->self_insert = evt->text;
                handle_key_press(key);
                break;
            }
        }
    }

    Vector2 window_dim = os::main_window->get_dimension();

    ui::set_next_fixed_width(window_dim.x);
    ui::set_next_fixed_height(window_dim.y);
    ui::set_next_fixed_x(0);
    ui::set_next_fixed_y(0);
    ui::push_background_color(Vector4(0, 0, 0, 1));
    ui::push_text_color(Vector4(1, 1,1, 1));
    ui::Box *main_cont = ui::box_create(ui::BoxFlag_Layer, STRZ("~MainCont"));
    UI_Parent(main_cont) {
        ui::set_next_layout_axis(Axis_X);
        ui::set_next_pref_width(ui::pref_size_parent(1.0f));
        ui::set_next_pref_height(ui::pref_size_px(28.0f));
        ui::Box *menu_cont = ui::box_create(ui::BoxFlag_DrawBackground, STRZ("~MenuCont"));
        UI_Parent(menu_cont) {

            String menu_items[] = {
                STRZ("File"),
                STRZ("Edit"),
                STRZ("View"),
                STRZ("Run"),
                STRZ("Help"),
            };

            for (int i = 0; i < cu_count_of(menu_items); i++) {
                String item = menu_items[i];
                ui::set_next_pref_height(ui::pref_size_parent(1.0f));
                ui::set_next_pref_width(ui::pref_size_text(8.0f));
                ui::Signal sig = ui::button(item);
            }

            ui::set_next_pref_height(ui::pref_size_parent(1.0f));
            ui::set_next_pref_width(ui::pref_size_text(8.0f));
            ui::set_next_font(ui::icon_font);
            ui::set_next_text_color(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
            ui::button(ui::string_from_icon_kind(ui::ICON_WARNING, "##arrow_up"));
        }

        View *view = get_active_view();
        ui::set_next_pref_width(ui::pref_size_px(window_dim.x));
        ui::set_next_pref_height(ui::pref_size_px(window_dim.y));
        ui::set_next_background_color(Vector4(0.11f, 0.11f, 0.1f, 1.0f));
        ui::set_next_text_color(Vector4(0.85f, 0.82f, 0.75f, 1.0f));
        view->box = ui::box_create(ui::BoxFlag_MouseInput|ui::BoxFlag_KeyboardInput, string_fmt("##view_%lld", view->id));
        view->box->draw_data = (void *)view;
        view->box->draw_proc = ui_draw_view;
    }


    FileSystem *fs = g_file_system;

    ui::push_background_color(Vector4(0.14f, 0.14f, 0.13f, 1.0f));
    ui::push_text_color(Vector4(0.85f, 0.82f, 0.75f, 1.0f));
    ui::set_next_fixed_width(window_dim.x);
    ui::set_next_fixed_height(window_dim.y);
    ui::set_next_fixed_x(0.0f);
    ui::set_next_fixed_y(0.0f);
    ui::set_next_child_alignment_x(ui::Alignment_Center);
    ui::set_next_child_alignment_y(ui::Alignment_Center);
    ui::Box *file_system_layer = ui::box_create(ui::BoxFlag_Layer, STRZ("##fsLayer"));
    UI_Parent(file_system_layer) {
        ui::set_next_pref_width(ui::pref_size_parent(0.5f));
        ui::set_next_pref_height(ui::pref_size_parent(0.9f));
        ui::set_next_layout_axis(Axis_X);
        ui::Box *file_system_cont = ui::box_create(ui::BoxFlag_DrawBackground, STRZ("##fsContainer"));

        UI_Parent(file_system_cont) {
            // File Buttons
            ui::set_next_fixed_width(400.0f);
            ui::set_next_pref_height(ui::pref_size_parent(1.0f));
            UI_Parent(ui::box_create(ui::BoxFlag_Default, STRZ("##file_buttons"))) {
                ui::push_pref_width(ui::pref_size_text(4.0f));
                ui::push_pref_height(ui::pref_size_text(4.0f));

                struct LineEdit {
                    u8 *buffer;
                    int len;
                    int capacity;
                    int pos;
                };

                cu_local_persist LineEdit ed = {};

                ui::line_edit(&ed.buffer, &ed.pos, &ed.len, &ed.capacity, STRZ("##file_line"));

                for (int i = 0; i < fs->files.count; i++) {
                    os::File *file = &fs->files[i];
                    ui::Signal sig = ui::button(string_fmt("%s##file_%d", file->file_name.text, i));
                    if (sig.clicked) {
                        printf("%s\n", (char *)file->file_name.text);
                    }
                }

                ui::pop_pref_width();
                ui::pop_pref_height();
            }

            cu_local_persist ui::ScrollPt scroll_pt = {};

            ui::set_next_fixed_width(10.0f);
            ui::set_next_pref_height(ui::pref_size_parent(1.0f));
            ui::set_next_background_color(Vector4(1, 0.2, 0.2, 1.0f));

            scroll_pt = ui::scroll_bar(STRZ("##scroll"), Axis_Y, scroll_pt, 0, 12, 20);
        }
    }
    ui::pop_background_color();
    ui::pop_text_color();

    // layout_pane(state->root_pane);

    ViewManager *view_manager = state->view_manager;
}

UI_DRAW_PROC(ui_draw_view) {
    View *view = (View *)draw_data;
    Buffer *buffer = find_buffer(view->bufid);

    ui::draw_rect(box->rect, box->background_color);

    Font *font = box->font;
    String buffer_text = string_concat(String{buffer->text, buffer->gb}, String{buffer->text + buffer->ge, buffer->len - buffer->ge});
    ui::draw_text(buffer_text, box->rect.tl, font, box->text_color, box->font_size);

    Vector2 start = box->rect.tl;
    Vector2 cursor = start;

    f32 scale = box->font_size / box->font->line_skip;

    for (isize i = 0; i < buffer_text.len; i++) {
        u32 code = (u32)buffer_text[i];
        GlyphMetrics *g = font_get_glyph_metrics(font, code);

        if (view->cursor.pos == i) {
            break;
        }

        if (code == '\n') {
            cursor.x = start.x;
            cursor.y += (font->line_skip + font->line_gap) * scale;
        } else {
            cursor.x += g->advance * scale;
        }
    }

    GlyphMetrics *g = font_get_glyph_metrics(font, '_');

    if (view->cursor.pos < buffer_text.len) {
        g = font_get_glyph_metrics(font, buffer_text.text[view->cursor.pos]);
    }

    ui::Rect dst = ui::Rect(
        cursor.x, // cursor.x + g->lsb * font->scale,
        cursor.y, // + font->ascent + g->y_off,
        cursor.x + 2.0f, // cursor.x + (g->lsb * font->scale) + g->bw,
        cursor.y + (font->line_skip + font->line_gap) * scale // + font->ascent + g->y_off + g->bh
    );

    ui::draw_rect(dst, Vector4(0, 0, 0, 1));

    string_release(&buffer_text);
}

};
