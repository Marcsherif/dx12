void
scene_init()
{
    // sampler_address address = Wrap;
    // sampler_filter filter = Linear;
    // b8 mips = true;
    // sampler_init(address, filter, mips);

    // Create an empty root signature.
    //root_signature_type types[] = { CBV, SRV, Sampler };
    root_signature_type types[] = {
        { CBV , CBV_Param }
    };
    root_signature_type *types_ptr = &types[0];
    root_signature_init(types_ptr, arrlen(types));

    g_render.desc.depth = true;
    g_render.desc.vertex = gfx_create_shader(L"shaders.hlsl", "VSMain", GFX_VERTEX_SHADER);
    g_render.desc.pixel = gfx_create_shader(L"shaders.hlsl", "PSMain", GFX_PIXEL_SHADER);
    gfx_pipeline_init(g_render.desc);

    g_render.rotation_angle = 0.0f;

    // Initialize the world matrix
    g_render.world_matrix = glm::mat4(1.0);

    // Initialize the view matrix
    glm::vec3 c_eye = { 0.0f, 3.0f, -10.0f };
    glm::vec3 c_at = { 0.0f, 1.0f, 0.0f };
    glm::vec3 c_up = { 0.0f, 1.0f, 0.0f };
    g_render.view_matrix = glm::lookAt(c_eye, c_at, c_up);

    // Initialize the projection matrix
    g_render.projection_matrix = glm::perspective(glm::radians(45.0f), f32(gfx.swap.width) / (f32)gfx.swap.height, 0.01f, 100.0f);

    load_pipeline();
    load_assets();
}

void
scene_update(bool wireframe)
{
    b32 dirty_pipeline = on_update(&g_render.desc, wireframe);
    on_render();

    if (dirty_pipeline)
    {
        wait_for_gpu();
        gfx_pipeline_rebuild(&g_dx12.state, g_render.desc);
    }
}
