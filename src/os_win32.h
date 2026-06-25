#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define WIN_32_EXTRA_LEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <timeapi.h>

namespace os {
extern LARGE_INTEGER query_performance_frequency;
};
