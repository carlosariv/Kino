#pragma once

#include <cstring>
#include <stdlib.h>

#include "utils.h"

inline static constexpr size_t DEFAULT_ARENA_SIZE = 1024*64;

enum AllocationKind {
    AllocationKind_Allocate,
    AllocationKind_Free,
    AllocationKind_Resize,
    AllocationKind_FreeAll
};

enum AllocationFlags {
    AllocationFlag_ZeroOut = (1<<0),
};

#define ALLOCATOR_PROC(Name) \
void * Name(void *allocator_data, AllocationKind kind, \
            isize size, isize alignment, \
            void *old_memory, isize old_size, \
            u64 flags)
typedef ALLOCATOR_PROC(AllocatorProc);

struct Allocator {
    AllocatorProc *proc;
    void *data;
};

#define ARENA_HEADER_SIZE 128
struct Arena {
    Arena *prev;
    Arena *current;
    u64 pos;
    u64 end;
    u64 base_pos;
    int align;
    u64 default_size;
};

Allocator arena_allocator(Arena *arena);
Arena *make_arena(u64 size = DEFAULT_ARENA_SIZE);
void *arena_push(Arena *arena, u64 size);
void arena_release(Arena *arena);
void arena_pop_to(Arena *arena, u64 pos);
void arena_clear(Arena *arena);

ALLOCATOR_PROC(arena_allocator_proc);
ALLOCATOR_PROC(heap_allocator_proc);

void *cu_allocate(Allocator a, isize size);
void *cu_free(Allocator a, void *ptr);
void *cu_resize(Allocator a, void *ptr, isize old_size, isize new_size);

#define cu_alloc_item(T,a,size) (T*)cu_allocate(a, size)
#define cu_alloc_array(T,a,size) (T*)cu_free(a, size)

// #define push_array(A,T,C) (T*)arena_push(A, sizeof(T)*C)
#define New(T,A) new (cu_allocate(A,sizeof(T))) T
#define NewArray(T,C,A) new (cu_allocate(A,(C)*sizeof(T))) T

extern Allocator g_heap_allocator;
