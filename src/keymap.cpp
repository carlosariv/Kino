#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include "string.h"
#include "array.h"
#include "keymap.h"
#include "os.h"

namespace kino {

KeyNode *get_key_node(KeyMap *map, i64 i) {
    return map->entries[i];
}

MapIndex get_map_index(KeyMap *map) {
    return map->entries.count;
}

Key make_key(Keycode keycode, KeyMod mod) {
    Key key = (u16)keycode | (u32)mod;
    return key;
}

KeyNode *key_node_alloc() {
    KeyNode *node = new KeyNode;
    return node;
}

KeyNode *key_node_create(KeyMap *map, KeyNode *parent, Key key) {
    KeyNode *node = key_node_alloc();
    node->index = get_map_index(map);
    node->key = key;
    node->cmd = nullptr;
    map->entries.add(node);

    if (parent) {
        parent->set.insert(node->index);
        //@Note: No longer a leaf node
        parent->cmd = nullptr;
    }

    return node;
}

KeyMap *keymap_create(String name) {
    KeyMap *map = new KeyMap;
    map->root = key_node_create(map, nullptr, make_key(Keycode::Nil));
    return map;
}

KeyMap *keymap_copy(KeyMap *old_map) {
    KeyMap *new_map = keymap_create(old_map->name);
    new_map->entries.reserve(old_map->entries.count);
    for (MapIndex i = 0; i < old_map->entries.count; i++) {
        KeyNode *old_node = old_map->entries[i];
        KeyNode *new_node = key_node_alloc();
        new_node->index = old_node->index;
        new_node->key = old_node->key;
        new_node->cmd = old_node->cmd;
        new_node->set = old_node->set;
        new_map->entries.add(new_node);
    }
    return new_map;
}

KeyNode *keymap_search(KeyMap *map, KeyNode *node, Key key) {
    for (MapIndex index : node->set) {
        KeyNode *node = get_key_node(map, index);
        if (node->key == key) {
            return node;
        }
    }

    //NOTE: Failed search
    return nullptr;
}

Key key_from_char(char c, KeyMod mods) {
    Keycode code = Keycode::Nil;
    os::ModFlags flags;
    os::get_key(c, &code, &flags);
    if (flags & os::ModFlag_Alt) {
        mods |= KeyMod_Alt;
    }
    if (flags & os::ModFlag_Control) {
        mods |= KeyMod_Control;
    }
    if (flags & os::ModFlag_Shift) {
        mods |= KeyMod_Shift;
    }

    Key key = make_key(code, mods);
    return key;
}

Key parse_key(String lhs, int *it) {
    Key key = 0;
    if (lhs.len == 0) return key;

    int i = *it;

    char c = (char)lhs.text[i];
    if (c == '<' && lhs.len > 1) {
        Array<String> parts;
        bool keep_parsing = true;
        i = i + 1;

        String part = {lhs.text + i, 0};
        while (keep_parsing && i < lhs.len) {
            c = lhs.text[i];
            if (c == '>') {
                parts.add(part);
                keep_parsing = false;
                i++;
                break;
            }
            if (c == '-') {
                parts.add(part);
                i++;
                part = {lhs.text + i, 0};
            } else {
                part.len++;
                i++;
            }
        }

        *it = i;

        KeyMod mods = KeyMod_None;

        for (int i = 0; i < parts.count ;i++) {
            String part = parts[i];

            if (i == parts.count-1) {
                if (part.len > 1) {
                    char c = 0;
                    if (string_equals(part, str_lit("CR"))) {
                        c = '\r';
                    } else if (string_equals(part, str_lit("ESC"))) {
                        c = '\x1b';
                    } else if (string_equals(part, str_lit("SPC"))) {
                        c = ' ';
                    } else if (string_equals(part, str_lit("TAB"))) {
                        c = '\t';
                    } else if (string_equals(part, str_lit("BS"))) {
                        c = '\x08';
                    } else if (string_equals(part, str_lit("DEL"))) {
                        c = '\x7f';
                    } else if (string_equals(part, str_lit("UP"))) {
                        return make_key(Keycode::Up, mods);
                    } else if (string_equals(part, str_lit("DOWN"))) {
                        return make_key(Keycode::Down, mods);
                    } else if (string_equals(part, str_lit("LEFT"))) {
                        return make_key(Keycode::Left, mods);
                    } else if (string_equals(part, str_lit("RIGHT"))) {
                        return make_key(Keycode::Right, mods);
                    }
                    assert(c != 0);
                    key = key_from_char(c, mods);
                } else {
                    c = part.text[0];
                    key = key_from_char(c, mods);
                }

                break;
            }

            if (string_equals(part, str_lit("C"))) {
                mods |= KeyMod_Control;
            } else if (string_equals(part, str_lit("A"))) {
                mods |= KeyMod_Alt;
            } else if (string_equals(part, str_lit("S"))) {
                mods |= KeyMod_Shift;
            }
        }
    } else {
        key = key_from_char(c, KeyMod_None);
        i++;
        *it = i;
    }

    return key;
}

void keymap_release_children(KeyNode *root) {
    root->set.clear();
}

void keymap_set(KeyMap *map, String lhs, EditorCmd cmd) {
    KeyNode *curr = map->root;

    int i = 0;
    while (i < lhs.len) {
        Key key = parse_key(lhs, &i);
        KeyNode *found = keymap_search(map, curr, key);
        if (found) {
            curr = found;
        } else {
            KeyNode *node = key_node_create(map, curr, key);
            curr = node;
        }
    }

    //@NOTE: Release children when overriding a key that is a parent.
    if (!curr->set.empty()) {
        keymap_release_children(curr);
    }

    assert(curr != map->root);
    curr->cmd = cmd;
}

};
