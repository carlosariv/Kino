#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>

#include "base_types.h"
#include "os.h"
#include "os_win32.h"
#include "shader.h"
#include "math.h"
#include "ui.h"
#include "ui_widgets.h"
#include "render.h"
#include "profiler.h"
#include "kino.h"
#include "view.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "kernel32.lib")

#if defined(PLATFORM_WINDOWS) && defined(_DEBUG)
#pragma comment(linker, "/subsystem:console")
int main(int argc, char **argv) {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    return WinMain(hInstance, 0, GetCommandLine(), SW_SHOWDEFAULT);
}
#elif defined(PLATFORM_WINDOWS)
#pragma comment (linker, "/subsystem:windows")
#else
#endif

int update_frame() {
    ProfilerZone("This Frame");

    int quit = 0;

    static bool first_call = true;
    if (first_call) {
        render_init();

        kino::init();
        first_call = false;
    }

    static f64 frame_delta = 0.0f;
    static os::WallClock last_clock = os::get_wall_clock();
    static int desired_target_frames_per_second = 120;
    static f64 target_ms_per_frame = 1000.0 / desired_target_frames_per_second;

    g_profiler.regions.reset();
    g_profiler.running_id = 1;

    os::poll_events();

    for (os::Event *evt : os::window_events) {
        if (evt->type == os::Event::Quit) {
            quit = 1;
            break;
        }
    }

    Vector2 render_dim = os::main_window->get_size();
    
    ui::per_frame_update(render_dim, frame_delta, os::window_events);

    kino::update((f32)frame_delta, os::window_events);

    ui::end_frame();

    {
        ProfilerZone("Renderer");
        render_frame(render_dim);
    }

    os::WallClock work_clock = os::get_wall_clock();
    f64 work_ms_elapsed = os::get_elapsed_milliseconds(last_clock, work_clock);
    int sleep_ms = (int)(target_ms_per_frame - work_ms_elapsed);
    if (sleep_ms > 0) {
        os::sleep_ms(sleep_ms);
    }

    os::WallClock end_clock = os::get_wall_clock();
    f64 frame_seconds_elapsed = os::get_elapsed_seconds(last_clock, end_clock);
    int frame_rate = (1.0 / frame_seconds_elapsed);
    // spdlog::info("fps: {} work: {:.4f}ms sleep: {}", frame_rate, work_ms_elapsed, sleep_ms);

    frame_delta = frame_seconds_elapsed;
    last_clock = end_clock;

    for (int i = 0; i < g_profiler.regions.count; i++) {
        ProfilerRegion region = g_profiler.regions[i];
        // spdlog::info("{}: {:.4f}ms", (char *)region.name, region.elapsed_ms);
    }

    return quit;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    timeBeginPeriod(1);
    QueryPerformanceFrequency(&os::query_performance_frequency);

    os::list_directory_files(STRZ("."));

    os::Window *window = os::window_create(1280, 720);

    for (int quit = 0; !quit; ) {
        quit = update_frame();
    }

    return 0;
}
