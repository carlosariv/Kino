#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>

#include <cstring>
#include "buffer.h"
#include "kino.h"

namespace kino {

void add_buffer(Buffer *buffer) {
    BufferManager *manager = state->buffer_manager;
    manager->buffer_list.add(buffer);
    buffer->id = manager->running_id;
    manager->running_id++;
}

Buffer *buffer_find(BufferID id) {
    BufferManager *manager = state->buffer_manager;
    for (Buffer *buffer : manager->buffer_list) {
        if (buffer->id == id) {
            return buffer;
        }
    }
    return nullptr;
}

Buffer *load_buffer_from_file(String file_name) {
    os::Handle file_handle = os::open_file(file_name, os::FileAccessFlag_Read|os::FileAccessFlag_Open);
    Buffer *buffer = buffer_create(file_name);

    String text = {};
    if (os::is_valid_handle(file_handle)) {
        text = os::read_entire_file(file_handle);

        buffer_insert_text(buffer, text, 0);
        string_release(&text);
    } else {
        printf("failed to load buffer file:%s\n", (char *)file_name.text);
    }

    return buffer;
}

Buffer *buffer_create(String name) {
    Buffer *buffer = new Buffer;
    add_buffer(buffer);

    int gap_size = DEFAULT_GAP_SIZE;
    buffer->text = new u8[gap_size];
    memset(buffer->text, 0, gap_size);
    buffer->gb = 0;
    buffer->ge = gap_size;
    buffer->len = gap_size;
    return buffer;
}

void ensure_gap_shift(Buffer *buffer, BufferPos gap_begin) {
    if (buffer->gb == gap_begin) return;

    i64 gap_len = get_buffer_gap_len(buffer);
    BufferPos gap_end = gap_begin + gap_len;

    // Move text that occupies the new gap to the right of it.
    if (gap_begin < buffer->gb) {
        std::memmove(buffer->text + gap_end, buffer->text + gap_begin, buffer->gb - gap_begin);
    }

    // Move text that occupies the new gap to the left.
    if (gap_begin > buffer->gb) {
        std::memmove(buffer->text + buffer->gb, buffer->text + buffer->ge, gap_end - buffer->ge);
    }

    buffer->gb = gap_begin;
    buffer->ge = gap_end;
}

void grow_buffer(Buffer *buffer, i64 min_len) {
    i64 text_len = get_buffer_len(buffer);
    i64 len = min_len + DEFAULT_GAP_SIZE;
    i64 gap_size = len - text_len;

    u8 *new_text = new u8[len];
    std::memset(new_text, 0, len);

    std::memcpy(new_text, buffer->text, buffer->gb);
    std::memcpy(new_text + buffer->gb + gap_size, buffer->text + buffer->ge, text_len);

    delete buffer->text;
    buffer->text = new_text;
    buffer->ge = buffer->gb + gap_size;
    buffer->len = len;
}

void ensure_buffer_gap_size(Buffer *buffer, BufferPos gap_position, i64 l) {
    if (get_buffer_gap_len(buffer) < l) {
        grow_buffer(buffer, get_buffer_len(buffer) + l);
    }
}

void buffer_insert_text(Buffer *buffer, String text, BufferPos pos) {
    ensure_buffer_gap_size(buffer, pos, text.len);

    ensure_gap_shift(buffer, pos);

    std::memcpy(buffer->text + buffer->gb, text.text, text.len);
    buffer->gb += text.len;

    update_line_starts(buffer);
}

void buffer_delete_region(Buffer *buffer, BufferPos start, BufferPos end) {
    ensure_gap_shift(buffer, start);

    buffer->ge += (end - start);

    update_line_starts(buffer);
}


void update_line_starts(Buffer *buffer) {
    buffer->line_starts.reset();
    i64 len = get_buffer_len(buffer);

    i64 line = 0;
    i64 col = 0;

    buffer->line_starts.add(0);

    for (BufferPos pos = 0; pos < len; pos++) {
        BufferPos i = get_buffer_index(buffer, pos);
        u8 c = buffer->text[i];
        if (c == '\n') {
            line++;
            col = 0;
            buffer->line_starts.add(pos + 1);
        }
    }

    buffer->line_starts.add(len);

    // printf("line starts:\n");
    // for (LineStart start : buffer->line_starts) {
    //     printf("%lld\n", start);
    // }
    // printf("\n");
}

};
