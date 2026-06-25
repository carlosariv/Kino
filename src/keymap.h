#pragma once

#include <unordered_set>

#include "base_types.h"
#include "string.h"
#include "enum_flags.hpp"
#include "key_code.h"

namespace kino {

#define KINO_COMMAND(Name) void Name()
typedef KINO_COMMAND((*EditorCmd));

// alt     bit 18
// shift   bit 17
// control bit 16
// key     bits 0-15
typedef u32 Key;

enum KeyMod {
    KeyMod_None = 0,
    KeyMod_Control = (1<<16),
    KeyMod_Shift = (1<<17),
    KeyMod_Alt = (1<<18),
};
ENUM_FLAG_OPERATORS(KeyMod);

typedef i64 MapIndex;

struct KeyNode {
    MapIndex index;
    Key key;
    EditorCmd cmd;
    std::unordered_set<MapIndex> set;
};

struct KeyMap {
    String name;
    Array<KeyNode*> entries;
    KeyNode *root;
};

KeyMap *keymap_create(String name);
KeyMap *keymap_copy(KeyMap *old_map);
Key key_from_char(char c, KeyMod mods);
Key make_key(Keycode keycode, KeyMod mod = KeyMod_None);
KeyMap *keymap_create();
KeyNode *key_node_create(KeyMap *map, KeyNode *parent, Key key);
KeyNode *keymap_search(KeyMap *map, KeyNode *node, Key key);
void keymap_set(KeyMap *map, String lhs, EditorCmd cmd);
Key parse_key(String lhs);

};
