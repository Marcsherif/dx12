#ifdef _WIN32
	#define WINDOWS
    #define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#elif defined(__linux__)
	#define LINUX
	#error "Linux is not supported yet";
#elif defined(__APPLE__) && defined(__MACH__)
	#define MACOS
	#error "Macos is not supported yet";
#else
	#error "Current OS not supported!";
#endif

#define DX12
// #define OPENGL
// #define METAL
#if !(defined(DX12)||defined(OPENGL)||defined(METAL))
	#if WINDOWS
		#define DX12
	#elif LINUX
		#define OPENGL
	#elif MACOS
		#define METAL
	#endif
#endif

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>

// #define GLM_ENABLE_EXPERIMENTAL
// #include <glm/gtx/string_cast.hpp>

#include "jf_platform.h"

#include "win_main.h"
#include "jf_main.h"

#include "jf_graphics.h"
#include "jf_render.h"

// #ifdef DX12
//     #include "jf_dx12.h"
// #elif OPENGL
// #elif METAL
// #else
// #endif

#include "jf_main.cpp"

global_variable HWND g_wnd = 0;
global_variable b32 g_running = true;
#define TITLE L"test"

#include "jf_graphics.cpp"
#include "jf_render.cpp"
#include "jf_dx12.cpp"

// internal win32_window_dimension
// win32_get_window_dimension(HWND wnd)
// {
//     win32_window_dimension result;

//     RECT client_rect;
//     GetClientRect(wnd, &client_rect);
//     result.Width = client_rect.right - client_rect.left;
//     result.Height = client_rect.bottom - client_rect.top;

//     return result;
// }

internal void
win32_process_keyboard_message(game_button_state *new_state, b32 is_down)
{
    if(new_state->ended_down != is_down)
    {
        new_state->ended_down = is_down;
        ++new_state->half_transition_count;
    }
}

internal LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT result = 0;

    switch (msg)
    {
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard input came in through a non-dispatch message!");
        } break;

        case WM_SIZE:
        {
            RECT rc;
            GetClientRect(g_wnd, &rc);
            gfx.swap.width = rc.right - rc.left;
            gfx.swap.height = rc.bottom - rc.top;
        } break;

        // case WM_PAINT:
        // {
        // } break;

        case WM_CLOSE:
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        } break;

        default:
        {
            result = DefWindowProcW(wnd, msg, wparam, lparam);
        } break;
    }

    return result;
}

internal void
win32_process_messages(win32_state *state, game_controller_input *keyboard)
{
    MSG msg;
    while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        switch(msg.message)
        {
            case WM_QUIT:
            {
                g_running = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 VKCode = (u32)msg.wParam;
                b32 was_down = ((msg.lParam & (1 << 30)) != 0);
                b32 is_down ((msg.lParam & (1 << 31)) == 0);
                if(was_down != is_down)
                {
                    win32_process_keyboard_message(&keyboard->wireframe, is_down);
                    if(VKCode == 'W')
                    {
                        win32_process_keyboard_message(&keyboard->move_forward, is_down);
                    }
                    else if(VKCode == 'A')
                    {
                        win32_process_keyboard_message(&keyboard->move_left, is_down);
                    }
                    else if(VKCode == 'S')
                    {
                        win32_process_keyboard_message(&keyboard->move_backward, is_down);
                    }
                    else if(VKCode == 'D')
                    {
                        win32_process_keyboard_message(&keyboard->move_right, is_down);
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                        g_running = false;
                    }
                    if(is_down)
                    {
                        // bool32 AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
                        // if((VKCode == VK_F4) && AltKeyWasDown)
                        // {
                        //     GlobalRunning = false;
                        // }
                        // if((VKCode == VK_RETURN) && AltKeyWasDown)
                        // {
                        //     if(Message.hwnd)
                        //     {
                        //         ToggleFullscreen(Message.hwnd);
                        //     }
                        // }
                    }
                }
            } break;

            default:
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } break;
        }
    }
}

int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // Initialize the window class.
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"DXWindowClass";

    ATOM atom = RegisterClassExW(&wc);
    Assert(atom && "Failed to register window class");

    RegisterClassExW(&wc);

    i32 width = CW_USEDEFAULT;
    i32 height = CW_USEDEFAULT;

    DWORD exstyle = WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP;
    DWORD style = WS_OVERLAPPEDWINDOW;

    // uncomment in case you want fixed size window
    //style &= ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
    //RECT rect = { 0, 0, 1280, 720 };
    //AdjustWindowRectEx(&rect, style, FALSE, exstyle);
    //width = rect.right - rect.left;
    //height = rect.bottom - rect.top;

    g_wnd = CreateWindowExW(
        exstyle, wc.lpszClassName, TITLE, style,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        0, 0, wc.hInstance, 0);
    Assert(g_wnd && "Failed to create window");

    RECT rc;
    GetClientRect(g_wnd, &rc);
    gfx.swap.width = rc.right - rc.left;
    gfx.swap.height = rc.bottom - rc.top;

    win32_state state = {};
    game_memory_t game_memory = {};
    //mem->permanent_size = Kilobytes(256);
    game_memory.transient_size = Megabytes(1);
    // WARNING(marc): UNCOMMENT THIS WHEN YOU DON'T NEED TO LOOP QUICKLY
    // game_memory->scratch_size = Gigabytes(1);

    state.total_size = game_memory.permanent_size + game_memory.transient_size;
    state.memory_block = VirtualAlloc(0, (size_t)state.total_size,
                                                 MEM_RESERVE|MEM_COMMIT, //|MEM_LARGE_PAGES,
                                                 PAGE_READWRITE);
    game_memory.permanent = state.memory_block;
    game_memory.transient = ((u8 *)game_memory.permanent + game_memory.permanent_size);

    game_input_t input[2] = {};
    game_input_t *new_input = &input[0];
    game_input_t *old_input = &input[1];

    ShowWindow(g_wnd, nCmdShow);

    while (g_running)
    {
        game_controller_input *old_keyboard = get_controller(old_input, 0);
        game_controller_input *new_keyboard = get_controller(new_input, 0);

        *new_keyboard = {};
        new_keyboard->is_connected = true;
        for(int i = 0; i < arrlen(new_keyboard->buttons); ++i)
        {
            new_keyboard->buttons[i].ended_down =
                old_keyboard->buttons[i].ended_down;
        }

        win32_process_messages(&state, new_keyboard);
        main(&game_memory, new_input);

        game_input_t *temp = new_input;
        new_input = old_input;
        old_input = temp;
    }

    on_destroy();
}
