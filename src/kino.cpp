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

KINO_COMMAND(file_system_open) {
    FileSystem *fs = g_file_system;
    if (!fs->active) {
        fs->active = true;
        file_system_load(fs, os::current_path());
    } else {
        fs->active = false;
    }
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

    keymap_set(norm, STRZ("<C-e>"), file_system_open);

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
    //     ui::set_next_pref_width(ui::pixel_size(pane->dim.x));
    //     ui::set_next_pref_height(ui::pixel_size(pane->dim.y));
    //     ui::set_next_background_color(Vector4(1, 1, 1, 1));
    //     ui::set_next_text_color(Vector4(0, 0, 0, 1));
    //     if (view == state->console_view) {
    //         ui::set_next_background_color(Vector4(0, 1, 0, 1));
    //     }
    //     view->box = ui::box_create(ui::BoxFlag_Clickable|ui::BoxFlag_Pressable, string_fmt("##view_%lld", view->id));
    //     view->box->draw_data = (void *)view;
    //     view->box->draw_proc = ui_draw_view;
    // } else {
    //     ui::set_next_pref_width(ui::pixel_size(pane->dim.x));
    //     ui::set_next_pref_height(ui::pixel_size(pane->dim.y));
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

    Vector2 window_dim = os::main_window->get_size();

    os::clear_title_bar_client_areas();

    ui::set_next_fixed_width(window_dim.x);
    ui::set_next_fixed_height(window_dim.y);
    ui::set_next_fixed_x(0);
    ui::set_next_fixed_y(0);
    ui::push_background_color(Vector4(0, 0, 0, 1));
    ui::push_text_color(Vector4(1, 1,1, 1));
    ui::Box *main_cont = ui::box_create(ui::BoxFlag_Clip|ui::BoxFlag_Layer, STRZ("~MainCont"));
    UI_Parent(main_cont) {
        cu_local_persist f32 debug_t = 0.0f;
        const f32 debug_max = 0.8f;
        debug_t += dt;
        if (debug_t > debug_max) {
            debug_t = 0.0f;
        }

        ui::set_next_layout_axis(Axis_X);
        ui::set_next_pref_width(ui::pct_size(1.f, 1.f));
        ui::set_next_pref_height(ui::pixel_size(28.0f, 1.f));
        if (debug_t < 0.5f*debug_max) {
            ui::set_next_background_color(Vector4(1, 0, 0, 1));
        } else {
            ui::set_next_background_color(Vector4(0, 0, 0, 1));
        }
        ui::Box *title_bar = ui::box_create(ui::BoxFlag_DrawBackground, STRZ("##titlebar"));
        os::push_custom_title_bar(ui::get_rect_size(title_bar->rect).y);
        UI_Parent(title_bar) {
            String menu_items[] = {
                STRZ("File"),
                STRZ("Edit"),
                STRZ("View"),
                STRZ("Run"),
                STRZ("Help"),
            };

            for (int i = 0; i < cu_count_of(menu_items); i++) {
                String item = menu_items[i];
                ui::set_next_pref_width(ui::text_size(8.f, 1.f));
                ui::set_next_pref_height(ui::pct_size(1.f, 1.f));
                ui::Signal sig = ui::button(item);
                os::push_title_bar_client_area(sig.box->rect);
            }

            ui::spacer(ui::pct_size(1.f, 0.f));

            ui::push_pref_height(ui::pct_size(2.f, 1.f));
            ui::push_pref_width(ui::text_size(8.f, 1.f));
            ui::push_font(ui::icon_font);
            ui::push_text_color(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
            ui::push_font_size(14.0f);
            {
                ui::Signal min_sig = ui::button(ui::string_from_icon_kind(ui::IconKind_WindowMinimize, "##min"));
                ui::Signal max_sig = ui::button(ui::string_from_icon_kind(ui::IconKind_WindowMaximize, "##max"));
                ui::Signal close_sig = ui::button(ui::string_from_icon_kind(ui::IconKind_Cancel, "##close"));
                if (min_sig.clicked) {
                    os::set_window_minimized();
                }
                if (max_sig.clicked) {
                    os::set_window_maximized();
                }
                if (close_sig.clicked) {
                    os::set_window_close();
                }

                os::push_title_bar_client_area(min_sig.box->rect);
                os::push_title_bar_client_area(max_sig.box->rect);
                os::push_title_bar_client_area(close_sig.box->rect);
            }
            ui::pop_pref_height();
            ui::pop_pref_width();
            ui::pop_font();
            ui::pop_font_size();
            ui::pop_text_color();
        }

        View *view = get_active_view();
        ui::set_next_pref_width(ui::pct_size(1.f, 1.f));
        ui::set_next_pref_height(ui::pct_size(1.f, 1.f));
        ui::set_next_background_color(Vector4(.11f, .11f, .1f, 1.f));
        ui::set_next_text_color(Vector4(.85f, .82f, .75f, 1.f));
        ui::set_next_font_size(20.0f);
        view->box = ui::box_create(ui::BoxFlag_MouseClickable|ui::BoxFlag_KeyboardClickable, string_fmt("##view_%lld", view->id));
        view->box->draw_data = (void *)view;
        view->box->draw_proc = ui_draw_view;
    }

    FileSystem *fs = g_file_system;

    if (fs->active) {
        ui::push_background_color(Vector4(.11f, .12f, .17f, 1.f));
        ui::push_text_color(Vector4(0.85f, 0.82f, 0.75f, 1.0f));
        ui::set_next_fixed_x(0.0f);
        ui::set_next_fixed_y(0.0f);
        ui::set_next_pref_width(ui::pct_size(1.f, 1.f));
        ui::set_next_pref_height(ui::pct_size(1.f, 1.f));
        ui::set_next_child_alignment_x(ui::Alignment_Center);
        ui::set_next_child_alignment_y(ui::Alignment_Center);
        ui::Box *file_system_layer = ui::box_create(ui::BoxFlag_Clip|ui::BoxFlag_Layer, STRZ("##fsLayer"));
        UI_Parent(file_system_layer) {
            ui::set_next_pref_width(ui::pct_size(.5f, 1.f));
            ui::set_next_pref_height(ui::pct_size(.9f, 1.f));
            ui::set_next_layout_axis(Axis_X);
            ui::set_next_child_alignment_x(ui::Alignment_Center);
            ui::set_next_border_color(Vector4(0.65f, 0.67f, 0.8f, 1.f));
            ui::Box *file_system_cont = ui::box_create(ui::BoxFlag_DrawBackground|ui::BoxFlag_DrawBorder, STRZ("##fsContainer"));


            f32 row_height = 30.0f;

            UI_Parent(file_system_cont) {
                // File Buttons
                ui::set_next_pref_width(ui::pct_size(1.f, 1.f));
                ui::set_next_pref_height(ui::pct_size(1.f, 1.f));
                ui::set_next_layout_axis(Axis_Y);
                UI_Parent(ui::box_create(ui::BoxFlag_DrawBackground, STRZ("##file_buttons"))) {
                    ui::push_pref_width(ui::pct_size(1.f, 0.f));
                    ui::push_pref_height(ui::pixel_size(row_height, 1.f));

                    struct LineEdit {
                        u8 *buffer;
                        int len;
                        int capacity;
                        int pos;
                    };

                    cu_local_persist LineEdit ed = {};

                    ui::line_edit(&ed.buffer, &ed.pos, &ed.len, &ed.capacity, STRZ("##file_line"));

                    for (int i = 0; i < cu_clamp_top(fs->files.count, 14); i++) {
                        os::File *file = &fs->files[i];
                        ui::set_next_border_color(Vector4(0.33f, 0.43f, 0.51f, 1.f));

                        if (i %2 == 0) ui::set_next_background_color(Vector4(.094f, .102f, .153f, 1.f));

                        ui::set_next_layout_axis(Axis_X);
                        ui::set_next_pref_width(ui::pct_size(1.0f, 1.f));
                        ui::Box *row = ui::box_create(ui::BoxFlag_MouseClickable|ui::BoxFlag_DrawBackground|ui::BoxFlag_DrawBottom|ui::BoxFlag_DrawHotEffects, string_fmt("##file_%d", i));
                        UI_Parent(row) {
                            ui::set_next_pref_width(ui::text_size(4.f, 0.f));
                            ui::set_next_pref_height(ui::text_size(4.f, 1.f));
                            ui::set_next_font(ui::icon_font);
                            if (file->attributes & os::FileAttrib_Directory) {
                                ui::set_next_text_color(Vector4(0.98f, 0.73f, 0.11f, 1.f));
                                ui::label(ui::string_from_icon_kind(ui::IconKind_Folder, "##folder"));
                            } else {
                                ui::set_next_text_color(Vector4(1.f, 1.f, 1.f, 1.f));
                                ui::label(ui::string_from_icon_kind(ui::IconKind_Document, "##doc"));
                            }

                            ui::set_next_pref_width(ui::text_size(4.f, 0.f));
                            ui::set_next_pref_height(ui::text_size(4.f, 1.f));
                            ui::label(string_fmt("%s", file->file_name.text));

                            ui::set_next_pref_width(ui::text_size(4.f, 0.f));
                            ui::set_next_pref_height(ui::text_size(4.f, 1.f));
                            ui::label(string_fmt("%llu##size_%d", file->file_size, i));
                        }

                        ui::Signal row_sig = ui::signal_from_box(row);
                        if (row_sig.clicked) {
                            printf("%s\n", (char *)file->file_name.text);
                        }
                    }

                    ui::pop_pref_width();
                    ui::pop_pref_height();
                }

                cu_local_persist ui::ScrollPt scroll_pt = {};
                int visible_line_count = (file_system_cont->rect.bottom - file_system_cont->rect.top) / (ui::top_font_size() + 8.0f);

                int visible_row_count = (file_system_cont->rect.bottom - file_system_cont->rect.top) / row_height;

                scroll_pt = ui::scroll_bar(STRZ("##scroll"), Axis_Y, scroll_pt, 0, 12, visible_row_count);
            }
        }
        ui::pop_background_color();
        ui::pop_text_color();
    }

    // layout_pane(state->root_pane);

    ViewManager *view_manager = state->view_manager;
}

UI_DRAW_PROC(ui_draw_view) {
    View *view = (View *)draw_data;
    Buffer *buffer = find_buffer(view->bufid);

    ui::draw_rect(box->rect, box->background_color);

    Font *font = box->font;
    String buffer_text = string_concat(String{buffer->text, buffer->gb}, String{buffer->text + buffer->ge, buffer->len - buffer->ge});
    ui::draw_text(buffer_text, box->rect, box->rect.tl, font, box->text_color, box->font_size, 0, Vector2::ZERO, ui::DrawTextFlag_Default);

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

    Rect dst = Rect(
        cursor.x, // cursor.x + g->lsb * font->scale,
        cursor.y, // + font->ascent + g->y_off,
        cursor.x + 2.0f, // cursor.x + (g->lsb * font->scale) + g->bw,
        cursor.y + (font->line_skip + font->line_gap) * scale // + font->ascent + g->y_off + g->bh
    );

    ui::draw_rect(dst, Vector4(0, 0, 0, 1));

    string_release(&buffer_text);
}

};
