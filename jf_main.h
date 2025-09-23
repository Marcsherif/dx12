struct memory_arena
{
    u64 size;
    u8 *base;
    u64 used;
};

inline void
initialize_arena(memory_arena *arena, u64 size, void *base)
{
    arena->size = size;
    arena->base = (u8 *)base;
    arena->used = 0;
}

#define push_struct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define push_array(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
inline void *
PushSize_(memory_arena *Arena, u64 size)
{
    Assert((Arena->used + size) <= Arena->size);
    void *Result = Arena->base + Arena->used;
    Arena->used += size;
    return(Result);
}

#define zero_struct(Instance) zero_size(sizeof(Instance), &Instance)
inline void
zero_size(u64 size, void *ptr)
{
    u8 *byte = (u8 *)ptr;
    while(size--)
    {
        *byte++ = 0;
    }
}

struct game_state
{
    memory_arena arena;
};

global_variable game_state g_state = {};

