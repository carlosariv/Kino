#pragma once

#include "os.h"

struct ProfilerRegion;

struct ProfilerRegion {
    const char *name;
    int id = 0;
    os::WallClock start;
    f32 elapsed_ms = 0;

    ProfilerRegion(const char *name);
    ~ProfilerRegion();
};

struct Profiler {
    Array<ProfilerRegion> regions;
    int running_id = 1;
};

extern Profiler g_profiler;

#define __CONCAT_INNER(a, b) a ## b
#define __CONCAT(a, b) __CONCAT_INNER(a, b)

#ifdef _DEBUG
#define PROFILE_LOG(Name, Time) spdlog::info("{}: {:.4f}ms", Name, Time)
#else
#define PROFILE_LOG(Name, Time) 0
// #define PROFILE_LOG(Name, Time) DebugPrint("Name: %s %.4fms\n", Name, Time);
#endif

#define ProfilerZone(Name) ProfilerRegion __CONCAT(_profiler_region_, __LINE__) = ProfilerRegion(Name)
