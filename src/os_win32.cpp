
#include <dwmapi.h>

#include "array.h"
#include "os.h"
#include "os_win32.h"
#include "key_code.h"

void render_state_init();
void d3d11_init(HWND hWnd);
int update_frame();

void __debug_breakpoint() {
    DebugBreak();
}

namespace os {
Window *main_window;
Array<Event*> window_events;
LARGE_INTEGER query_performance_frequency;
Array<Rect> title_bar_rects;

String current_path() {
    DWORD len = GetCurrentDirectory(0, NULL);
    u8 *buffer = NewArray(u8, heap_allocator(), len+1);
    len = GetCurrentDirectory(len+1, (LPSTR)buffer);
    String result = {buffer, len};
    return result;
}

void set_window_minimized() {
    ShowWindow((HWND)main_window->handle, SW_MINIMIZE);
}

void set_window_maximized() {
    HWND hWnd = (HWND)main_window->handle;
    if (IsMaximized(hWnd)) {
        ShowWindow(hWnd, SW_RESTORE);
    } else {
        ShowWindow(hWnd, SW_MAXIMIZE);
    }
}

void set_window_close() {
    PostMessageA((HWND)main_window->handle, WM_CLOSE, 0, 0);
}

void clear_title_bar_client_areas() {
    title_bar_rects.reset();
}

void push_title_bar_client_area(Rect rect) {
    title_bar_rects.add(rect);
}

void push_custom_title_bar(f32 thickness) {
    main_window->custom_border_thickness = thickness;
}

WallClock get_wall_clock() {
    LARGE_INTEGER clock;
    QueryPerformanceCounter(&clock);
    return (WallClock)clock.QuadPart;
}

u64 get_elapsed_ticks(WallClock start, WallClock end) {
    return (end - start);
}

f64 get_elapsed_seconds(WallClock start, WallClock end) {
    return (f64)get_elapsed_ticks(start, end) / query_performance_frequency.QuadPart;
}

f64 get_elapsed_milliseconds(WallClock start, WallClock end) {
    return 1000 * ((f64)get_elapsed_ticks(start, end) / query_performance_frequency.QuadPart);
}

u64 get_file_last_write_time(Handle handle) {
    FILETIME ft;
    GetFileTime((HANDLE)handle, NULL, NULL, &ft);
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart;
}

bool is_valid_handle(Handle handle) {
    return (HANDLE)handle != INVALID_HANDLE_VALUE;
}

void close_handle(Handle handle) {
    BOOL ret = CloseHandle((HANDLE)handle);
}

String read_entire_file(Handle file_handle) {
    HANDLE hFile = (HANDLE)file_handle;

    DWORD file_size_high = 0;
    DWORD file_size_low = GetFileSize(hFile, &file_size_high);
    ULARGE_INTEGER uli;
    uli.LowPart = file_size_low;
    uli.HighPart = file_size_high;
    u64 file_size = uli.QuadPart;

    u8 *data = new u8[file_size + 1];
    DWORD bytes_to_read = (DWORD)file_size;
    DWORD bytes_read;

    if (!ReadFile(hFile, data, bytes_to_read, &bytes_read, NULL)) {
        DWORD err = GetLastError();
    }

    String result;
    result.text = data;
    result.len = bytes_read;
    result.text[result.len] = 0;
    return result;
}

Handle open_file(String file_name, FileAccessFlags flags) {
    DWORD dwDesiredAccess = 0;
    if (flags & FileAccessFlag_Read) dwDesiredAccess |= GENERIC_READ;
    if (flags & FileAccessFlag_Write) dwDesiredAccess |= GENERIC_WRITE;

    //TODO: Share mode
    DWORD dwShareMode = 0;

    DWORD createFlags = OPEN_EXISTING;
    if (flags & FileAccessFlag_Create) {
        createFlags = CREATE_ALWAYS;
    } else if (flags & FileAccessFlag_Open) {
        createFlags = OPEN_EXISTING;
    } else if (flags & FileAccessFlag_Append) {
        createFlags = TRUNCATE_EXISTING;
        //TODO: Allow for non-existing file, change to create_always if so
    }

    DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

    HANDLE handle = CreateFileA(reinterpret_cast<char *>(file_name.text), dwDesiredAccess, dwShareMode, NULL, createFlags, dwFlagsAndAttributes, NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        //TODO: Error logging
    }

    return (Handle)handle;
}

void set_cursor_pos(Window *window, int xpos, int ypos) {
    POINT point = {xpos, ypos};
    window->last_cursor_x = xpos;
    window->last_cursor_y = ypos;

    ClientToScreen((HWND)window->handle, &point);
    SetCursorPos(point.x, point.y);
}

void poll_events() {
    for (int i = 0; i < window_events.count; i++) {
        delete window_events[i];
    }
    window_events.reset();

    if (GetActiveWindow() == (HWND)main_window->handle) {
        main_window->focus_active = true;
    }

    UINT dpi = GetDpiForWindow((HWND)main_window->handle);
    main_window->dpi = dpi;

    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void Window::disable_cursor() {
    capture_cursor = true;

    RECT rect;
    GetClientRect((HWND)handle, &rect);
    ClipCursor(&rect);
}

Keycode key_from_virtual_keycode(int vk) {
    switch (vk) {
        default:
            return Keycode::Nil;

        case VK_OEM_1: return Keycode::Semicolon;
        case VK_OEM_2: return Keycode::Slash;
        case VK_OEM_3: return Keycode::Tilde;
        case VK_OEM_4: return Keycode::OpenBracket;
        case VK_OEM_5: return Keycode::BackSlash;
        case VK_OEM_6: return Keycode::CloseBracket;
        case VK_OEM_7: return Keycode::Quote;
        case VK_OEM_PLUS: return Keycode::Plus;
        case VK_OEM_COMMA: return Keycode::Comma;
        case VK_OEM_MINUS: return Keycode::Dash;
        case VK_OEM_PERIOD: return Keycode::Dot;

        // case VK_XBUTTON1:
        // case VK_XBUTTON2:
        case VK_BACK: return Keycode::Backspace;
        case VK_TAB: return Keycode::Tab;
        // case VK_CLEAR:
        case VK_RETURN: return Keycode::Enter;
        case VK_SHIFT: return Keycode::Shift;
        case VK_CONTROL: return Keycode::Control;
        case VK_MENU: return Keycode::Alt;
        case VK_PAUSE: return Keycode::Pause;
        // case VK_CAPITAL:
        // case VK_KANA:
        // case VK_HANGUL:
        // case VK_IME_ON:
        // case VK_JUNJA:
        // case VK_FINAL:
        // case VK_HANJA:
        // case VK_KANJI:
        // case VK_IME_OFF:
        case VK_ESCAPE: return Keycode::Escape;
        // case VK_CONVERT:
        // case VK_NONCONVERT:
        // case VK_ACCEPT:
        // case VK_MODECHANGE:
        case VK_SPACE: return Keycode::Space;
        // case VK_PRIOR:
        // case VK_NEXT:
        case VK_END: return Keycode::End;
        case VK_HOME: return Keycode::Home;
        case VK_LEFT: return Keycode::Left;
        case VK_UP: return Keycode::Up;
        case VK_RIGHT: return Keycode::Right;
        case VK_DOWN: return Keycode::Down;
        // case VK_SELECT:
        case VK_PRINT: return Keycode::Print;
        // case VK_EXECUTE:
        // case VK_SNAPSHOT:
        case VK_INSERT: return Keycode::Insert;
        case VK_DELETE: return Keycode::Delete;
        case VK_HELP:   return Keycode::Help;

        case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: case 0x38: case 0x39:
            return (Keycode)((int)Keycode::Zero + vk - 0x30);

        case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F: case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58: case 0x59: case 0x5A:
            return (Keycode)((int)Keycode::A + vk - 0x41);

        // case VK_NUMPAD0:
        // case VK_NUMPAD1:
        // case VK_NUMPAD2:
        // case VK_NUMPAD3:
        // case VK_NUMPAD4:
        // case VK_NUMPAD5:
        // case VK_NUMPAD6:
        // case VK_NUMPAD7:
        // case VK_NUMPAD8:
        // case VK_NUMPAD9:
        // case VK_MULTIPLY:
        // case VK_ADD:
        // case VK_SEPARATOR:
        // case VK_SUBTRACT:
        // case VK_DECIMAL:
        // case VK_DIVIDE:

        case VK_F1: case VK_F2: case VK_F3: case VK_F4: case VK_F5: case VK_F6: case VK_F7: case VK_F8: case VK_F9: case VK_F10: case VK_F11: case VK_F12: case VK_F13: case VK_F14: case VK_F15: case VK_F16: case VK_F17: case VK_F18: case VK_F19: case VK_F20: case VK_F21: case VK_F22: case VK_F23: case VK_F24:
            return (Keycode)((int)Keycode::F1 + vk - VK_F1);

        // case VK_NUMLOCK:
        // case VK_SCROLL:
        // case VK_LSHIFT:
        // case VK_RSHIFT:
        // case VK_LCONTROL:
        // case VK_RCONTROL:
        // case VK_LMENU:
        // case VK_RMENU:
    }
}

ModFlags get_modifier_flags() {
    bool ctrl  = (GetKeyState(VK_CONTROL)& 0x8000)!=0;
    bool alt   = (GetKeyState(VK_MENU)   & 0x8000)!=0;
    bool shift = (GetKeyState(VK_SHIFT)  & 0x8000)!=0;

    ModFlags flags = ModFlag_Nil;
    flags |= (ModFlags)(ModFlag_Control*ctrl);
    flags |= (ModFlags)(ModFlag_Shift  *shift);
    flags |= (ModFlags)(ModFlag_Alt    *alt);
    return flags;
}

void get_key(char character, Keycode *code_out, ModFlags *flags_out) {
    HKL keyboard_layout = GetKeyboardLayout(0);
    SHORT scan = VkKeyScanExA(character, keyboard_layout);
    BYTE vk = LOBYTE(scan);
    BYTE state = HIBYTE(scan);

    Keycode key_code = key_from_virtual_keycode(vk);

    ModFlags flags = ModFlag_Nil;
    flags |= (ModFlags)(ModFlag_Shift  *((state&1)!=0));
    flags |= (ModFlags)(ModFlag_Control*((state&2)!=0));
    flags |= (ModFlags)(ModFlag_Alt    *((state&4)!=0));

    *code_out = key_code;
    *flags_out = flags;
}

BOOL rect_contains_point(Rect rect, POINT pt) {
    return rect.left <= pt.x && rect.right > pt.x && rect.top <= pt.y && rect.bottom > pt.y;
}

LRESULT window_event_callback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    // Window *window = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    Window *window = main_window;


    bool release = false;
    bool horizontal = false;

    auto mouse_wm_to_key = [msg]() -> Keycode {
        switch (msg) {
            case WM_LBUTTONDBLCLK:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
                return Keycode::MouseLeft;
            case WM_RBUTTONDBLCLK:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
                return Keycode::MouseRight;
            case WM_MBUTTONDBLCLK:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
                return Keycode::MouseMiddle;
            default:
                return Keycode::Nil;
        }
    };

    Event *evt = nullptr;

    switch (msg) {
        case WM_CREATE:
            render_state_init();
            d3d11_init(hWnd);
            break;

        case WM_CLOSE:
            evt = push_event(Event::Quit);
        break;

        case WM_SETFOCUS:
            window->focus_active = true;
        break;

        case WM_KILLFOCUS:
            window->focus_active = false;
            break;

        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
                window->focus_active = false;
            else // WA_ACTIVE or WA_CLICKACTIVE
                window->focus_active = true;
            break;

        case WM_SIZE: {
            window->width = LOWORD(lParam);
            window->height = HIWORD(lParam);
        case WM_PAINT:
            window->is_minimized = IsIconic(hWnd);
            PAINTSTRUCT ps = {0};
            BeginPaint(hWnd, &ps);
            update_frame();
            EndPaint(hWnd, &ps);
            DwmFlush();
        } break;

        case WM_NCCALCSIZE: {
            if (window->custom_border) {
                //- TODO: Need to handle DPI settings. GetSystemMetricsForDpi
                int padding = GetSystemMetrics(SM_CXPADDEDBORDER);
                int border_width = GetSystemMetrics(SM_CXFRAME) + padding;
                int border_height = GetSystemMetrics(SM_CYFRAME) + padding;

                RECT* rect = 0;

                if (wParam) {
                    rect = ((NCCALCSIZE_PARAMS *) lParam)->rgrc;
                } else {
                    rect = (RECT* ) lParam;
                }

                rect->left += border_width;
                rect->right -= border_width;
                rect->bottom -= border_height;

                //- NOTE: Maximized window
                if (IsZoomed(hWnd)) {
                    rect->top += border_height;
                }
            } else {
                result = DefWindowProcA(hWnd, msg, wParam, lParam);
            }
        } break;


        case WM_NCHITTEST: {
            if (!window->custom_border) {
                result = DefWindowProcA(hWnd, msg, wParam, lParam);
            } else {
                LRESULT hit = DefWindowProcA(hWnd, msg, wParam, lParam);

                bool default_handled = false;
                switch (hit) {
                    case HTNOWHERE:
                    case HTTOPLEFT:
                    case HTTOPRIGHT:
                    case HTLEFT:
                    case HTRIGHT:
                    case HTBOTTOM:
                    case HTBOTTOMLEFT:
                    case HTBOTTOMRIGHT:
                        default_handled = true;
                        break;
                }

                if (!default_handled) {
                    //- TODO: Need to handle DPI settings. GetSystemMetricsForDpi
                    int padding = GetSystemMetrics(SM_CXPADDEDBORDER);
                    int frame_x = GetSystemMetrics(SM_CXFRAME);
                    int frame_y = GetSystemMetrics(SM_CYFRAME);

                    // Expand the hit test area for resizing

                    POINT window_pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                    POINT pt = window_pt;
                    ScreenToClient(hWnd, &pt);

                    bool is_over_top_resize = pt.y >= 0 && pt.y < frame_y;
                    bool is_over_title_bar = pt.y >= 0 && pt.y < window->custom_border_thickness;

                    bool is_over_title_bar_client_area = false;
                    for (Rect rect : title_bar_rects) {
                        if (rect_contains_point(rect, pt)) {
                            is_over_title_bar_client_area = true;
                            break;
                        }
                    }

                    if (IsMaximized(hWnd)) {
                        if (is_over_title_bar_client_area) {
                            hit = HTCLIENT;
                        } else if (is_over_title_bar) {
                            hit = HTCAPTION;
                        } else {
                            hit = HTCLIENT;
                        }
                    } else {
                        if (is_over_title_bar_client_area) {
                            hit = HTCLIENT;
                        } else if (is_over_top_resize) {
                            hit = HTTOP;
                        } else if (is_over_title_bar) {
                            hit = HTCAPTION;
                        } else {
                            hit = HTCLIENT;
                        }
                    }
                }
                result = hit;
            }
        } break;

        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                HCURSOR hCursor;
                switch (window->current_cursor) {
                    default:
                    case Cursor_Nil:
                    case Cursor_Arrow:
                        hCursor = LoadCursor(NULL, IDC_ARROW);
                        break;
                    case Cursor_Hand:
                        hCursor = LoadCursor(NULL, IDC_HAND);
                        break;
                    case Cursor_IBeam:
                        hCursor = LoadCursor(NULL, IDC_IBEAM);
                        break;
                }
                SetCursor(hCursor);
            } else {
                result = DefWindowProcA(hWnd, msg, wParam, lParam);
            }
        } break;

        case WM_KEYUP: {
            evt = push_event(Event::KeyRelease);
            int vk = (int)wParam;
            evt->key = key_from_virtual_keycode(vk);
        } break;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            int vk = (int)wParam;

            static BYTE key_state[256];
            GetKeyboardState(key_state);
            wchar_t ch[2];
            int result = ToUnicode(vk, MapVirtualKey(vk, MAPVK_VK_TO_VSC), key_state, ch, 2, 0);
            if (result > 0) {
            } else {
                evt = push_event(Event::KeyPress);
                evt->key = key_from_virtual_keycode(vk);
                evt->mod_flags = get_modifier_flags();
            }
        } break;

        case WM_SYSCHAR:
        case WM_CHAR: {
            UINT scan_code = (lParam >> 16) & 0xFF;
            UINT vk = MapVirtualKey(scan_code, MAPVK_VSC_TO_VK);
            char ch = (char)wParam;

            Keycode key_code = key_from_virtual_keycode(vk);
            ModFlags mod_flags = get_modifier_flags();

            evt = push_event(Event::KeyPress);
            evt->key = key_code;
            evt->mod_flags = mod_flags;
            evt->text = make_string(&ch, 1);
        } break;

        case WM_MOUSEMOVE: {
            if (window->tracking_mouse) {
                evt = push_event(Event::MouseMove);
                evt->mx = GET_X_LPARAM(lParam);
                evt->my = GET_Y_LPARAM(lParam);

                POINT point = {evt->mx, evt->my};
                ClientToScreen((HWND)window->handle, &point);
                window->last_cursor_x = point.x;
                window->last_cursor_y = point.y;
            } else {
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                if (TrackMouseEvent(&tme)) {
                    window->tracking_mouse = true;
                }
            }
        } break;

        case WM_MOUSELEAVE:
            window->tracking_mouse = false;
            evt = push_event(Event::MouseLeave);
            break;

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            release = true;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN: {
            evt = push_event(release ? Event::MouseRelease : Event::MousePress);
            evt->key = mouse_wm_to_key();
            evt->mx = GET_X_LPARAM(lParam);
            evt->my = GET_Y_LPARAM(lParam);

            if (release) {
                ReleaseCapture();
            } else {
                SetCapture(hWnd);
            }
        } break;

        case WM_MOUSEHWHEEL: {
            evt = push_event(Event::MouseWheel);
            evt->mx = GET_X_LPARAM(lParam);
            evt->my = GET_Y_LPARAM(lParam);
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            evt->scroll.x = (f32)delta;
            evt->scroll.y = 0.f;
        } break;

        case WM_MOUSEWHEEL: {
            evt = push_event(Event::MouseWheel);
            evt->mx = GET_X_LPARAM(lParam);
            evt->my = GET_Y_LPARAM(lParam);
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            evt->scroll.x = 0.f;
            evt->scroll.y = (f32)delta;
        } break;

        default:
            result = DefWindowProcA(hWnd, msg, wParam, lParam);
            break;
    }

    return result;
}

Window *window_create(int width, int height) {
    Window *window = new Window();
    window->custom_border = true;
    main_window = window;

    WNDCLASSEXA wndclass = {};
    wndclass.cbSize = sizeof(WNDCLASSEXA);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = window_event_callback;
    wndclass.hInstance = GetModuleHandle(NULL);
    wndclass.lpszClassName = "WINDOW_CLASS";

    RegisterClassExA(&wndclass);

    int client_width = width;
    int client_height = height;

    DWORD style_flags = 0;
    style_flags |= WS_EX_NOREDIRECTIONBITMAP;

    DWORD dwStyle = WS_OVERLAPPEDWINDOW;
    HWND hWnd = CreateWindowExA(style_flags, "WINDOW_CLASS", "kino", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, client_width, client_height, NULL, NULL, NULL, NULL);

    SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)window);

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    window->handle = (Handle)hWnd;

    return window;
}

Vector2 Window::get_size() {
    return Vector2(width, height);
}

void sleep_ms(int ms) {
    Sleep(ms);
}

Event *push_event(Event::Type type) {
    Event *event = new Event(type);
    window_events.add(event);
    return event;
}

Array<File> list_directory_files(String path) {
    Array<File> files;
    String find_path = string_concat( path, STRZ("/*"));
    WIN32_FIND_DATAA file_data;
    HANDLE find_handle = FindFirstFile((LPCSTR)find_path.text, &file_data);

    if (find_handle != INVALID_HANDLE_VALUE) {
        do {
            File file = {};
            file.file_name = make_string(file_data.cFileName);
            file.file_size = ((u64)file_data.nFileSizeHigh << 32) + file_data.nFileSizeLow;
            file.create_time = ((u64)file_data.ftCreationTime.dwHighDateTime << 32) + file_data.ftCreationTime.dwLowDateTime;
            file.access_time = ((u64)file_data.ftLastAccessTime.dwHighDateTime << 32) + file_data.ftLastAccessTime.dwLowDateTime;
            file.write_time = ((u64)file_data.ftLastWriteTime.dwHighDateTime << 32) + file_data.ftLastWriteTime.dwLowDateTime;

            file.attributes = FileAttrib_Default;
            if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                file.attributes |= FileAttrib_Directory;
            }
            if (file_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
                file.attributes |= FileAttrib_Hidden;
            }
            if (file_data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                file.attributes |= FileAttrib_ReadOnly;
            }
            if (file_data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) {
                file.attributes |= FileAttrib_Normal;
            }

            if (file.attributes == FileAttrib_Default) {
                file.attributes |= FileAttrib_Normal;

            }

            files.add(file);
        } while (FindNextFileA(find_handle, &file_data));

        FindClose(find_handle);
    }

    return files;
}
};
