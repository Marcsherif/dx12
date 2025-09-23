#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef u8 b8;

typedef float f32;
typedef double r64;

#define internal static
#define local_persist static
#define global_variable static

#include <intrin.h>
#define Assert(cond) do { if (!(cond)) __debugbreak(); } while (0)
#define AssertHR(hr) Assert(SUCCEEDED(hr))

#define arrlen(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

global_variable f32 PI = glm::pi<float>();

internal void fatal_error(const char* message)
{
    MessageBoxA(NULL, message, "Error", MB_ICONEXCLAMATION);
    __debugbreak();
}

void log(const char* msg, ...)
{
    char buf[4096];
    va_list vl;

    va_start(vl, msg);
    vsnprintf(buf, sizeof(buf), msg, vl);
    va_end(vl);
    printf("%s\n", buf);
}

/*
 NOTE(marc): Services that the game provides to the platform layer.
 (this may expand in the future - sound on seperate thread, etc.)
*/

struct game_button_state
{
    i32 half_transition_count;
    b32 ended_down;
};

struct game_controller_input
{
    b32 is_connected;
    b32 is_analog;
    f32 stick_average_x;
    f32 stick_average_y;

    union
    {
        game_button_state buttons[10];
        struct
        {
            game_button_state move_left;
            game_button_state move_right;
            game_button_state move_forward;
            game_button_state move_backward;

            game_button_state wireframe;

            game_button_state fov_in;
            game_button_state fov_out;

            game_button_state zoom_in;
            game_button_state zoom_out;

            game_button_state debug;

            //

            game_button_state terminator;
        };
    };
};

struct game_input_t
{
    // Mouse for debugging
    game_button_state mouseButtons[5];
    f32 mouseX, mouseY, mouseZ;

    f32 dt;
    u64 uptime;
    u64 frequency;
    f32 last_mouse_x;
    f32 last_mouse_y;

    game_controller_input controllers[1]; // extend for more players
};

inline game_controller_input *get_controller(game_input_t *input, u32 controller_index)
{
    Assert(controller_index < arrlen(input->controllers));
    game_controller_input *result = &input->controllers[controller_index];
    return(result);
};

struct game_memory_t
{
    b32 initialized;

    u64 permanent_size;
    void *permanent; // NOTE(marc): Required to be cleared to be zero at startup

    u64 transient_size;
    void *transient; // NOTE(marc): Required to be cleared to be zero at startup
};
