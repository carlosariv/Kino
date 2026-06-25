#include <stb/stb_sprintf.h>

#include "string.h"

String make_string_view(const char *text) {
    return make_string_view(text, strlen(text));
}

String make_string_view(const char *text, isize len) {
    return make_string_view((u8 *)text, len);
}

String make_string_view(u8 *text, isize len) {
    String string;
    string.text = text;
    string.len = len;
    return string;
}

String make_string(const char *text) {
    return make_string(text, strlen(text));
}

String make_string(const char *text, isize len) {
    return make_string((u8 *)text, len);
}

String make_string(u8 *text, isize len) {
    String string;
    string.len = len;
    string.text = (u8 *)malloc((len + 1) * sizeof(u8));
    if (len > 0) {
        memcpy(string.text, text, len);
    }
    string.text[len] = 0;
    return string;
}

String copy_string(String string) {
    return make_string(string.text, string.len);
}

bool string_equals(String a, String b) {
    if (a.len != b.len) return false;

    for (i64 i = 0; i < a.len; i++) {
        if (a.text[i] != b.text[i]) return false;
    }
    return true;
}

void string_release(String *string) {
    if (string->text) {
        free(string->text);
    }
    string->text = nullptr;
    string->len = 0;
}

String substring(String string, isize start, isize end) {
    String result = {string.text + start, (end - start)};
    return result;
}

isize string_find(String string, String match) {
    if (match.len==0) return -1;

    for (isize i = 0; i < string.len; i++) {
        if (match.text[0] == string.text[i]) {
            bool matches = true;
            for (isize j = 0; j < match.len; j++) {
                if (match.text[j] != string.text[i + j]) {
                    matches = false;
                    break;
                }
            }

            if (matches) {
                return i;
            }
        }
    }

    return -1;
}

String string_concat(String const &a, String const &b) {
    isize len = a.len + b.len;
    u8 *text = new u8[len + 1];
    memcpy(text, a.text, a.len);
    memcpy(text + a.len, b.text, b.len);
    text[len] = 0;
    return make_string(text, len);
}

String string_fmt(char const *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    int len = stbsp_vsnprintf(NULL, 0, fmt, va);

    char *buf = (char *)malloc(len + 1);

    stbsp_vsnprintf(buf, len + 1, fmt, va);

    va_end(va);

    String result = {(u8 *)buf, len};
    return result;
}


