#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "profiler.h"

Profiler g_profiler;

void add_region(ProfilerRegion region) {
    g_profiler.running_id++;
    g_profiler.regions.add(region);
}

ProfilerRegion::ProfilerRegion(const char *name) {
    this->name = name;
    start = os::get_wall_clock();
    id = g_profiler.running_id;
    add_region(*this);
}

ProfilerRegion::~ProfilerRegion() {
    os::WallClock end = os::get_wall_clock();
    elapsed_ms = os::get_elapsed_milliseconds(start, end);

    for (int i = 0; i < g_profiler.regions.count; i++) {
        ProfilerRegion *region = &g_profiler.regions[i];
        if (region->id==id) {
            region->elapsed_ms = elapsed_ms;
            break;
        }
    }
}

void DebugPrint(const char* format, ...) {
    char buf[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    OutputDebugStringA(buf);
}
