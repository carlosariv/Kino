#include <assert.h>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#define NOMINMAX
#include <windows.h>

#include "base_types.h"
#include "allocator.h"

Allocator g_heap_allocator = { heap_allocator_proc, nullptr };

static constexpr size_t alignment = alignof(std::max_align_t);

void *cu_allocate(Allocator a, isize size) {
    return a.proc(a.data, AllocationKind_Allocate, size, alignment, nullptr, 0, 0);
}

void *cu_free(Allocator a, void *ptr) {
    return a.proc(a.data, AllocationKind_Free, 0, alignment, ptr, 0, 0);
}

void *cu_resize(Allocator a, void *ptr, isize old_size, isize new_size) {
    return a.proc(a.data, AllocationKind_Resize, new_size, alignment, ptr, new_size, 0);
}

Allocator arena_allocator(Arena *arena) {
    Allocator allocator;
    allocator.proc = arena_allocator_proc;
    allocator.data = (void *)arena;
    return allocator;
}

ALLOCATOR_PROC(arena_allocator_proc) {
    void *ptr = nullptr;
    Arena *arena = (Arena *)allocator_data;
    assert(arena != nullptr);
    switch (kind) {
        case AllocationKind_Allocate: {
            ptr = arena_push(arena, size);
            break;
        }
        case AllocationKind_Free: {
            break;
        }
        case AllocationKind_Resize: {
            if (size == 0) {
                ptr = nullptr;
            } else if (size <= old_size) {
                ptr = old_memory;
            } else {
                ptr = arena_push(arena, size);
                memmove(ptr, old_memory, old_size);
            }
            break;
        }
        // case AllocationKind_FreeAll: {
        //     arena_release(arena);
        //     break;
        // }
    }

    return ptr;
}

ALLOCATOR_PROC(heap_allocator_proc) {
    void *ptr = nullptr;

    switch (kind) {
        case AllocationKind_Allocate: {
            ptr = calloc(size, 1);
            break;
        }
        case AllocationKind_Free: {
            ptr = nullptr;
            if (old_memory) {
                free(old_memory);
            }
            break;
        }
        case AllocationKind_Resize: {
            if (size > 0) {
                ptr = realloc(old_memory, size);
                //NOTE: Zero out new memory
                if (size > old_size) {
                    memset((void *)((u8 *)ptr + old_size), 0, size - old_size);
                }
            } else if (old_memory) {
                free(old_memory);
            }
            break;
        }
        // case AllocationKind_FreeAll: {
        //     arena_release(arena);
        //     break;
        // }
    }

    return ptr;
}



Arena *make_arena(u64 size) {
    size += ARENA_HEADER_SIZE;
    // size = AlignForward(size, 8);

    void *mem = VirtualAlloc(NULL, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    // Arena *arena = (Arena *)((u8 *)mem + AlignForward(ARENA_HEADER_SIZE, alignment));
    Arena *arena = (Arena *)mem;
    arena->prev = nullptr;
    arena->current = arena;
    arena->pos = ARENA_HEADER_SIZE;
    arena->end = size;
    arena->base_pos = ARENA_HEADER_SIZE;
    arena->align = alignment;
    arena->default_size = size;
    return arena; 
}

void *arena_push(Arena *arena, u64 size) {
    Arena *current = arena->current;
    // printf("pos:%llu\n", current->pos);
    u64 new_pos = AlignForward(current->pos, arena->align);
    // printf("new_pos:%llu\n", new_pos);
    if (new_pos + size > current->end) {
        Arena *new_block = make_arena(arena->default_size);
        //@Todo Should this be aligned to power of 2?
        new_block->base_pos = (current->base_pos + current->end);
        new_block->prev = current;
        arena->current = new_block;
        new_pos = ARENA_HEADER_SIZE;
    }

    void *memory = (void *)((u8 *)arena->current + new_pos);
    arena->current->pos = new_pos + size;
    return memory;
}

void arena_release(Arena *arena) {
    for (Arena *node = arena, *prev = nullptr; node; node = prev) {
        prev = node->prev;

        VirtualFree(node, 0, MEM_RELEASE);
    }
}

void arena_pop_to(Arena *arena, u64 pos) {
    if (pos < ARENA_HEADER_SIZE) pos = ARENA_HEADER_SIZE;
    Arena *current = arena->current;
    while (current->base_pos > pos) {
        Arena *prev = current->prev;
        VirtualFree(current, 0, MEM_RELEASE);
        current = prev;
    }
    assert(current);
    current->pos = pos;
    arena->current = current;
}

void arena_clear(Arena *arena) {
    arena_pop_to(arena, 0);
}
