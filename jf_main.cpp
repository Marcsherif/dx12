int main(game_memory_t *game_memory, game_input_t *game_input)
{
    if (!game_memory->initialized)
    {
        initialize_arena(&g_state.arena, game_memory->transient_size, (u8 *)game_memory->transient);

        gfx_init();

        scene_init();

        game_memory->initialized = true;
    }

    scene_update(game_input->controllers[0].wireframe.ended_down);
}

