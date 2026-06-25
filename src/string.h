#pragma once

#include <stdlib.h>
#include <string.h>
#include "base_types.h"

struct String {
    u8 *text = nullptr;
    isize len = 0;

    u8 const &operator[](isize i) const {
        return text[i];
    }

    bool operator==(const String &other) const {
        return len==other.len && (strncmp((char *)text, (char *)other.text, len)==0);
    }
};

struct StringHasher {
    size_t operator()(const String& k) const {
        size_t hash_seed = 5381;
        size_t hash = hash_seed;
        for (isize i = 0; i < k.len; i++) {
            hash = (hash << 5) + hash + k[i];
        }
        return hash;
        // return std::hash<isize>{}(k.len) ^ (std::hash<u8*>{}(k.text) << 1);
    }
};

#define str_lit(Name) make_string_view((Name), sizeof((Name))-1)
#define STRZ(Name) make_string_view((Name), sizeof((Name))-1)

String make_string(const char *text);
String make_string(u8 *text, isize len);
String make_string(const char *text, isize len);
String make_string_view(const char *text);
String make_string_view(u8 *text, isize len);
String make_string_view(const char *text, isize len);
String copy_string(String string);

String substring(String string, isize start, isize end);
String string_concat(String const &a, String const &b);
void string_release(String *string);
isize string_find(String string, String match);

String string_fmt(char const *fmt, ...);

bool string_equals(String a, String b);
