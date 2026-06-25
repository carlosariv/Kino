#pragma once

#include "array.h"
#include "base_types.h"
#include "string.h"

#define DEFAULT_GAP_SIZE 1024

namespace kino {


struct Cursor {
    i64 pos = 0;
    i64 line = 0;
    i64 col = 0;
};

typedef i64 BufferPos;
typedef i64 BufferID;

struct Buffer {
    BufferID id;
    String file_name;

    BufferPos gb;
    BufferPos ge;
    BufferPos len;
    u8 *text;

    Array<BufferPos> line_starts;
};

struct BufferManager {
    int running_id = 1;
    Array<Buffer*> buffer_list;
};

inline i64 get_buffer_gap_len(Buffer *buffer) { return (buffer->ge - buffer->gb); }
inline i64 get_buffer_len(Buffer *buffer) { return buffer->len - get_buffer_gap_len(buffer); }

inline BufferPos get_buffer_index(Buffer *buffer, BufferPos p) {
    if (p >= buffer->gb) {
        p += get_buffer_gap_len(buffer);
    }
    return p;
}

inline i64 get_buffer_line_count(Buffer *buffer) { return buffer->line_starts.count - 1; }
inline i64 get_buffer_line_length(Buffer *buffer, i64 line) {
    return buffer->line_starts[line + 1] - buffer->line_starts[line];
}

Buffer *buffer_find(BufferID bufid);

Buffer *buffer_create(String name);
Buffer *load_buffer_from_file(String file_name);

void buffer_insert_text(Buffer *buffer, String text, BufferPos pos);
void buffer_delete_region(Buffer *buffer, BufferPos start, BufferPos end);

void update_line_starts(Buffer *buffer);

};
