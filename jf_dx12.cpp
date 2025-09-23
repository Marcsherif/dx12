internal void check(HRESULT hr, char *msg)
{
    if (FAILED(hr))
    {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "D3D12 Error: %s", msg);
        fatal_error(msg);
    }
}

internal void
get_adapter(IDXGIFactory4 *factory, IDXGIAdapter1 **return_adapter, bool high_perf)
{
    *return_adapter = 0;

    IDXGIAdapter1 *adapter = 0;
    IDXGIFactory6 *factory6 = 0;

    if(SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for(u32 adapter_index = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                         adapter_index,
                         high_perf == true ?
                                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE :
                                DXGI_GPU_PREFERENCE_UNSPECIFIED,
                         IID_PPV_ARGS(&adapter)
                      ));
            ++adapter_index)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), 0)))
            {
                break;
            }
        }
    }

    if(!adapter)
    {
        for (u32 adapter_index = 0; SUCCEEDED(factory->EnumAdapters1(adapter_index, &adapter)); ++adapter_index)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;

            if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), 0)))
                break;
        }
    }

    *return_adapter = adapter;
}

void gfx_init()
{
    u32 dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    ID3D12Debug *debug_controller;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
    {
        debug_controller->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    AssertHR(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&gfx.factory)));

    if (false) // check if gpu available
    {
        // IDXGIAdapter *warpAdapter;
        // AssertHR(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        // AssertHR(D3D12CreateDevice(
        //     warpAdapter.Get(),
        //     D3D_FEATURE_LEVEL_12_0,
        //     IID_PPV_ARGS(&device)
        //     ));
    }
    else
    {
        IDXGIAdapter1 *adapter;
        get_adapter(gfx.factory, &adapter, true);

        AssertHR(D3D12CreateDevice( adapter,
                 D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&gfx.device)
        ));
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    AssertHR(gfx.device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&gfx.command_queue)));

    // Create descriptor heaps.
    descriptor_heap_init(&gfx.rtv_heap         , D3D12_DESCRIPTOR_HEAP_TYPE_RTV , FRAME_COUNT);
    descriptor_heap_init(&gfx.dsv_heap         , D3D12_DESCRIPTOR_HEAP_TYPE_DSV , 1);
    descriptor_heap_init(&gfx.cbv_srv_uav_heap , D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV , FRAME_COUNT);
    descriptor_heap_init(&gfx.sampler_heap     , D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER , 1);

    swapchain_init(&gfx.swap);

    AssertHR(gfx.factory->MakeWindowAssociation(g_wnd, DXGI_MWA_NO_ALT_ENTER));

    gfx.frame_index = gfx.swap.swap->GetCurrentBackBufferIndex();
    gfx.rtv_heap.cpu = gfx.rtv_heap.heap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = gfx.rtv_heap.cpu;

    for (u32 i = 0; i < FRAME_COUNT; i++)
    {
        HRESULT hr = gfx.swap.swap->GetBuffer(i, IID_PPV_ARGS(&gfx.swap.render_targets[i]));
        check(hr, "Failed getting render taret");
        gfx.device->CreateRenderTargetView(gfx.swap.render_targets[i], 0, rtv_handle);
        rtv_handle.ptr += gfx.rtv_heap.increment_size;

        hr = gfx.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&gfx.command_allocator[i]));
        check(hr, "Failed creating command allocator");
    }
}

// descriptor heap
//

void descriptor_heap_init(descriptor_heap *heap, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 size)
{
    heap->size = size;
    heap->type = type;

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.NumDescriptors = size;
    heap_desc.Type = type;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    {
        heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }

    heap->increment_size = gfx.device->GetDescriptorHandleIncrementSize(type);

    HRESULT hr = gfx.device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap->heap));
    check(hr, "Failed to create descripter heap");
}

// swapchain
//

void swapchain_init(swapchain *swap)
{
    HRESULT hr;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.Width = swap->width;
    swapChainDesc.Height = swap->height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    IDXGISwapChain1 *temp;
    hr = gfx.factory->CreateSwapChainForHwnd(gfx.command_queue, g_wnd, &swapChainDesc, 0, 0, &temp);
    check(hr, "Failed creating swapchain");

    temp->QueryInterface(&swap->swap);
    temp->Release();
}

internal void
load_pipeline()
{
    // Create the depth stencil view.
    // Performance tip: Deny shader resource access to resources that
    // don't need shader resource views.
    {
        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resource_desc.Format = DXGI_FORMAT_D32_FLOAT;
        resource_desc.Width = gfx.swap.width;
        resource_desc.Height = gfx.swap.height;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels = 0;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL |
                              D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

        D3D12_CLEAR_VALUE depth_clear_value = {};
        depth_clear_value.Format = DXGI_FORMAT_D32_FLOAT;
        depth_clear_value.DepthStencil.Depth = 1.0f;
        depth_clear_value.DepthStencil.Stencil = 0;

        D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_desc = {};
        depth_stencil_desc.Format = DXGI_FORMAT_D32_FLOAT;
        depth_stencil_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depth_stencil_desc.Flags = D3D12_DSV_FLAG_NONE;

        // TODO: move createCommitted Resource to gpu_resource_alloc function
        AssertHR(gfx.device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_desc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depth_clear_value,
            IID_PPV_ARGS(&g_dx12.depth_stencil)
        ));

        gfx.dsv_heap.cpu = gfx.dsv_heap.heap->GetCPUDescriptorHandleForHeapStart();
        gfx.device->CreateDepthStencilView(g_dx12.depth_stencil,
                                         &depth_stencil_desc,
                                         gfx.dsv_heap.cpu);
    }

    AssertHR(gfx.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&g_dx12.bundle_allocator)));
}


// Commands

void
command_set_pipeline_state(ID3D12GraphicsCommandList *cmd, pipeline_state *pso)
{
    cmd->SetPipelineState(pso->state);
    cmd->SetGraphicsRootSignature(pso->root_signature);
}

// Graphics pipeline

gfx_shader *
gfx_create_shader(wchar_t *path, char* entrypoint, gfx_shader_type type)
{
    gfx_shader *shader = 0;
    gfx_shader *error_blob = 0;

#if defined(_DEBUG)
    // Enable better shader debugging with the gfx debugging tools.
    u32 compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    u32 compileFlags = 0;
#endif

    char *shader_type = "";
    switch (type)
    {
        case GFX_VERTEX_SHADER:
        {
            shader_type = "vs_5_0";
        } break;

        case GFX_PIXEL_SHADER:
        {
            shader_type = "ps_5_0";
        } break;

        default:
        {
            fatal_error("ERROR: unkown shader type");
        } break;
    }

    HRESULT hr = D3DCompileFromFile(path, 0, 0, entrypoint, shader_type, compileFlags, 0, &shader, &error_blob);

#if defined(_DEBUG)
    if (error_blob) {
        OutputDebugStringA((char*)error_blob->GetBufferPointer());
        error_blob->Release();
    }
#endif

    check(hr, "unable to compile shader");

    return shader;
}

internal void
gfx_pipeline_init(pipeline_state *pso, pipeline_desc desc)
{
    ID3DBlob *vertex_shader = desc.vertex;
    ID3DBlob *pixel_shader = desc.pixel;

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        //{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Describe and create the gfx pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    pso_desc.pRootSignature = pso->root_signature;


    pso_desc.VS = { vertex_shader->GetBufferPointer(), vertex_shader->GetBufferSize() };
    pso_desc.PS = { pixel_shader->GetBufferPointer(), pixel_shader->GetBufferSize() };

    pso_desc.RasterizerState.FillMode = desc.wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;

    pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pso_desc.RasterizerState.FrontCounterClockwise = false;

    pso_desc.BlendState = {
        FALSE,                         // AlphaToCoverageEnable
        FALSE,                         // IndependentBlendEnable
        {
            {
                FALSE,                // BlendEnable
                FALSE,                // LogicOpEnable
                D3D12_BLEND_ONE,      // SrcBlend
                D3D12_BLEND_ZERO,     // DestBlend
                D3D12_BLEND_OP_ADD,   // BlendOp
                D3D12_BLEND_ONE,      // SrcBlendAlpha
                D3D12_BLEND_ZERO,     // DestBlendAlpha
                D3D12_BLEND_OP_ADD,   // BlendOpAlpha
                D3D12_LOGIC_OP_NOOP,  // LogicOp
                D3D12_COLOR_WRITE_ENABLE_ALL // RenderTargetWriteMask
            }
        }
    };

    if ( desc.depth )
    {
        pso_desc.DepthStencilState.DepthEnable = true;
        pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC(desc.func);
        pso_desc.DSVFormat = desc.dsv_format;
    }
    pso_desc.DepthStencilState.StencilEnable = FALSE;

    pso_desc.SampleMask = UINT_MAX;
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.SampleDesc.Count = 1;
    AssertHR(gfx.device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso->state)));
}

void
gfx_pipeline_init(pipeline_desc desc)
{
    gfx_pipeline_init(&g_dx12.state, desc);
}

internal void
gfx_pipeline_rebuild(pipeline_state *pso, pipeline_desc desc)
{
    RELEASE(pso->state);
    pso->state = 0;
    gfx_pipeline_init(pso, desc);
    if (g_dx12.bundle)
    {
        record_bundle(pso);
    }
}

void root_signature_init(root_signature_type *types, u32 types_len)
{
    D3D12_DESCRIPTOR_RANGE *ranges = push_array(&g_state.arena, types_len, D3D12_DESCRIPTOR_RANGE);
    D3D12_ROOT_PARAMETER *parameters = push_array(&g_state.arena, types_len, D3D12_ROOT_PARAMETER);

    for (u32 i = 0; i < types_len; ++i)
    {
        D3D12_DESCRIPTOR_RANGE *range = &ranges[i];

        range->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE(types[i].range);
        range->NumDescriptors = 1;
        range->BaseShaderRegister = 0;
        range->RegisterSpace = 0;
        // range->Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;

        D3D12_ROOT_PARAMETER *param = &parameters[i];

        if (types[i].param == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            param->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param->DescriptorTable.NumDescriptorRanges = 1;
            param->DescriptorTable.pDescriptorRanges = range;
        }
        else if (types[i].param != D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
        {
            param->ParameterType = (D3D12_ROOT_PARAMETER_TYPE)types[i].param;
            param->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param->Descriptor.ShaderRegister = 0;
            param->Descriptor.RegisterSpace = 0;
        }
    }

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = types_len;
    root_signature_desc.pParameters = parameters;
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ID3DBlob *root_signature;
    ID3DBlob *error;
    AssertHR(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature, &error));
    AssertHR(gfx.device->CreateRootSignature(0, root_signature->GetBufferPointer(), root_signature->GetBufferSize(), IID_PPV_ARGS(&g_dx12.state.root_signature)));

    root_signature->Release();
}

void sampler_init(sampler_address address, sampler_filter filter, b8 mips)
{
    D3D12_SAMPLER_DESC desc = {};
    desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE(address);
    desc.AddressV = desc.AddressU;
    desc.AddressW = desc.AddressU;
    desc.Filter = D3D12_FILTER(filter);
    desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    desc.MinLOD = 0.0f;
    if (mips) {
        desc.MaxLOD = D3D12_FLOAT32_MAX;
    }
    desc.MipLODBias = 0.0f;
    desc.BorderColor[0] = 1.0f;
    desc.BorderColor[1] = 1.0f;
    desc.BorderColor[2] = 1.0f;
    desc.BorderColor[3] = 1.0f;

    D3D12_CPU_DESCRIPTOR_HANDLE handle = gfx.sampler_heap.heap->GetCPUDescriptorHandleForHeapStart();
    gfx.device->CreateSampler(&desc, handle);
}

// Generate a simple black and white checkerboard texture.
u8 *GenerateTextureData()
{
    u32 rowPitch = TextureWidth * TexturePixelSize;
    u32 cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
    u32 cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
    u32 textureSize = rowPitch * TextureHeight;

    u8 *data = push_array(&g_state.arena, textureSize, u8);

    for (u32 n = 0; n < textureSize; n += TexturePixelSize)
    {
        u32 x = n % rowPitch;
        u32 y = n / rowPitch;
        u32 i = x / cellPitch;
        u32 j = y / cellHeight;

        if (i % 2 == j % 2)
        {
            data[n] = 0x00;        // R
            data[n + 1] = 0x00;    // G
            data[n + 2] = 0x00;    // B
            data[n + 3] = 0xff;    // A
        }
        else
        {
            data[n] = 0xff;        // R
            data[n + 1] = 0xff;    // G
            data[n + 2] = 0xff;    // B
            data[n + 3] = 0xff;    // A
        }
    }

    return data;
}

void command_buffer_copy_buffer_to_texture(ID3D12Resource *dst, ID3D12Resource *src, u32 levels, u32 mip_count)
{
    D3D12_RESOURCE_DESC desc = dst->GetDesc();

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *footprints = push_array(&g_state.arena, levels, D3D12_PLACED_SUBRESOURCE_FOOTPRINT);
    u32 *num_rows = push_array(&g_state.arena, levels, u32);
    u64 *row_sizes = push_array(&g_state.arena, levels, u64);
    u64 total_size = 0;

    gfx.device->GetCopyableFootprints(&desc, 0, levels, 0, footprints, num_rows, row_sizes, &total_size);

    for (uint32_t i = 0; i < levels; i++) {
        D3D12_TEXTURE_COPY_LOCATION src_copy = {};
        src_copy.pResource = src;
        src_copy.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src_copy.PlacedFootprint = footprints[i];

        D3D12_TEXTURE_COPY_LOCATION dst_copy = {};
        dst_copy.pResource = dst;
        dst_copy.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst_copy.SubresourceIndex = i;

        gfx.command_list->CopyTextureRegion(&dst_copy, 0, 0, 0, &src_copy, 0);
    }
}

internal void
load_assets()
{
    // Create the command list.
    AssertHR(gfx.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, gfx.command_allocator[gfx.frame_index], 0, IID_PPV_ARGS(&gfx.command_list)));

    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        vertex triangleVertices[] =
        {
            { glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) },
            { glm::vec3(1.0f, 1.0f, -1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
            { glm::vec3(1.0f, 1.0f, 1.0f), glm::vec4(0.0f, 1.0f, 1.0f, 1.0f) },

            { glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) },
            { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f) },
            { glm::vec3(1.0f, -1.0f, -1.0f), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) },
            { glm::vec3(1.0f, -1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },
            { glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) },
        };

        const u32 vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not
        // recommended. Every time the GPU needs it, the upload heap will be marshalled
        // over. Please read up on Default Heap usage. An upload heap is used here for
        // code simplicity and because there are very few verts to actually transfer.
        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        resource_desc.Width = vertexBufferSize;
        resource_desc.Height = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        resource_desc.MipLevels = 1;

        AssertHR(gfx.device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            0,
            IID_PPV_ARGS(&g_dx12.vertex_buffer)));

        // Copy the triangle data to the vertex buffer.
        u8* vertex_data_ptr;
        D3D12_RANGE readRange = {};        // We do not intend to read from this resource on the CPU.
        AssertHR(g_dx12.vertex_buffer->Map(0, &readRange, (void**)&vertex_data_ptr));
        memcpy(vertex_data_ptr, triangleVertices, vertexBufferSize);
        g_dx12.vertex_buffer->Unmap(0, 0);

        // Initialize the vertex buffer view.
        g_dx12.vertex_buffer_view.BufferLocation = g_dx12.vertex_buffer->GetGPUVirtualAddress();
        g_dx12.vertex_buffer_view.StrideInBytes = sizeof(vertex);
        g_dx12.vertex_buffer_view.SizeInBytes = vertexBufferSize;

        //    3________ 2
        //    /|      /|
        //   /_|_____/ |
        //  0|7|_ _ 1|_|6
        //   | /     | /
        //   |/______|/
        //  4       5
        //
        // Create index buffer
        u16 indices[] =
        {
            // TOP
            3,1,0,
            2,1,3,

            // FRONT
            0,5,4,
            1,5,0,

            // RIGHT
            3,4,7,
            0,4,3,

            // LEFT
            1,6,5,
            2,6,1,

            // BACK
            2,7,6,
            3,7,2,

            // BOTTOM
            6,4,5,
            7,4,6,
        };

        u32 index_buffer_size = sizeof(indices);

        resource_desc.Width = index_buffer_size;

        AssertHR(gfx.device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            0,
            IID_PPV_ARGS(&g_dx12.index_buffer)));

        // Copy the cube data to the vertex buffer.
        AssertHR(g_dx12.index_buffer->Map(0, &readRange, (void **)(&vertex_data_ptr)));
        memcpy(vertex_data_ptr, indices, sizeof(indices));
        g_dx12.index_buffer->Unmap(0, 0);

        // Initialize the index buffer view.
        g_dx12.index_buffer_view.BufferLocation = g_dx12.index_buffer->GetGPUVirtualAddress();
        g_dx12.index_buffer_view.Format = DXGI_FORMAT_R16_UINT;
        g_dx12.index_buffer_view.SizeInBytes = index_buffer_size;
    }

    record_bundle(&g_dx12.state);

    // Create the constant buffer.
    {
        // u32 constant_buffer_size = sizeof(scene_constant_buffer);    // CB size is required to be 256-byte aligned.
        // u64 cb_size = n_draw_calls * FRAME_COUNT * constant_buffer_size;

        u32 raw_cb_size = sizeof(scene_constant_buffer);
        u32 constant_buffer_size = (raw_cb_size + 255) & ~255;
        u64 cb_size = n_draw_calls * FRAME_COUNT * constant_buffer_size;

        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resource_desc = {};
        resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resource_desc.Alignment = 0;
        resource_desc.Width = cb_size;
        resource_desc.Height = 1;
        resource_desc.DepthOrArraySize = 1;
        resource_desc.MipLevels = 1;
        resource_desc.Format = DXGI_FORMAT_UNKNOWN;
        resource_desc.SampleDesc.Count = 1;
        resource_desc.SampleDesc.Quality = 0;
        resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        AssertHR(gfx.device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            0,
            IID_PPV_ARGS(&g_dx12.constant_buffer)));


        AssertHR(g_dx12.constant_buffer->Map(0, 0, (void **)(&g_dx12.constant_buffer_data)));

        // GPU virtual address of the resource
        g_dx12.constant_data_gpu_addr = g_dx12.constant_buffer->GetGPUVirtualAddress();

        // NOTE: the following is for a descriptor heap, we will be using a root descriptor
        // // Describe and create a constant buffer view.
        // D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        // cbvDesc.BufferLocation = g_dx12.constant_buffer->GetGPUVirtualAddress();
        // cbvDesc.SizeInBytes = constantBufferSize;
        // gfx.device->CreateConstantBufferView(&cbvDesc, g_dx12.cbv_srv_uav_heap->GetCPUDescriptorHandleForHeapStart());

        // // Map and initialize the constant buffer. We don't unmap this until the
        // // app closes. Keeping things mapped for the lifetime of the resource is okay.
        // D3D12_RANGE readRange = {};        // We do not intend to read from this resource on the CPU.
        // AssertHR(g_dx12.constant_buffer->Map(0, &readRange, reinterpret_cast<void**>(&g_dx12.cbv_data_begin)));
        // memcpy(g_dx12.cbv_data_begin, &g_dx12.constant_buffer_data, sizeof(g_dx12.constant_buffer_data));
    }

    // Create texture
    ID3D12Resource *textureUploadHeap;

    // Create the texture.
    {
        D3D12_HEAP_PROPERTIES heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

        // Describe and create a Texture2D.
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.Width = TextureWidth;
        textureDesc.Height = TextureHeight;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        AssertHR(gfx.device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            0,
            IID_PPV_ARGS(&g_dx12.texture)));

        u32 rowPitch = TextureWidth * TexturePixelSize;
        u32 alignedRowPitch = (rowPitch + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1))
            & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);

        u64 uploadBufferSize = alignedRowPitch * TextureHeight;

        heap_properties = {};
        heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC buffer_desc = {};
        buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        buffer_desc.Alignment = 0;
        buffer_desc.Width = uploadBufferSize;
        buffer_desc.Height = 1;
        buffer_desc.DepthOrArraySize = 1;
        buffer_desc.MipLevels = 1;
        buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
        buffer_desc.SampleDesc.Count = 1;
        buffer_desc.SampleDesc.Quality = 0;
        buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        buffer_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Create the GPU upload buffer.
        AssertHR(gfx.device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &buffer_desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            0,
            IID_PPV_ARGS(&textureUploadHeap)));

        // Copy data to the intermediate upload heap and then schedule a copy
        // from the upload heap to the Texture2D.
        u8 *texture = GenerateTextureData();

        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = &texture[0];
        textureData.RowPitch = TextureWidth * TexturePixelSize;
        textureData.SlicePitch = textureData.RowPitch * TextureHeight;

        // Map and copy the generated checkerboard to the upload heap
        u8* mappedData;
        AssertHR(textureUploadHeap->Map(0, 0, (void**)(&mappedData)));

        for (u32 y = 0; y < TextureHeight; ++y) {
            memcpy(mappedData + y * alignedRowPitch, texture + y * rowPitch, rowPitch);
        }

        textureUploadHeap->Unmap(0, 0);

        command_buffer_copy_buffer_to_texture(g_dx12.texture, textureUploadHeap, g_dx12.texture->GetDesc().MipLevels, 1);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = g_dx12.texture;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        gfx.command_list->ResourceBarrier(1, &barrier);

        // Describe and create a SRV for the texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        D3D12_CPU_DESCRIPTOR_HANDLE handle = gfx.cbv_srv_uav_heap.heap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += gfx.cbv_srv_uav_heap.increment_size;
        gfx.device->CreateShaderResourceView(g_dx12.texture, &srvDesc, handle);
    }

    AssertHR(gfx.command_list->Close());
    ID3D12CommandList* ppCommandLists[] = { gfx.command_list };
    gfx.command_queue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects.
    {
        AssertHR(gfx.device->CreateFence(gfx.fence_value[gfx.frame_index], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gfx.fence)));
        gfx.fence_value[gfx.frame_index]++;

        // Create an event handle to use for frame synchronization.
        gfx.fence_event = CreateEvent(0, FALSE, FALSE, 0);
        if (!gfx.fence_event)
        {
            AssertHR(HRESULT_FROM_WIN32(GetLastError()));
        }

        // we are using the command list above to create the texture in our main loop so wait for it to finish
        //wait_for_previous_frame();
        wait_for_gpu();
    }

    // initialize viewport and scissor_rect
    {
        g_dx12.viewport = {};
        g_dx12.scissor_rect = {};

        g_dx12.viewport.Width = (f32)gfx.swap.width;
        g_dx12.viewport.Height = (f32)gfx.swap.height;
        g_dx12.viewport.MaxDepth = 1.0f;

        g_dx12.scissor_rect.right = gfx.swap.width;
        g_dx12.scissor_rect.bottom = gfx.swap.height;
    }
}

internal void
record_bundle(pipeline_state *pso)
{
    AssertHR(gfx.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, g_dx12.bundle_allocator, 0, IID_PPV_ARGS(&g_dx12.bundle)));
    command_set_pipeline_state(g_dx12.bundle, pso);
    g_dx12.bundle->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_dx12.bundle->IASetVertexBuffers(0, 1, &g_dx12.vertex_buffer_view);
    g_dx12.bundle->IASetIndexBuffer(&g_dx12.index_buffer_view);
    g_dx12.bundle->DrawIndexedInstanced(36, 1, 0, 0, 0);
    AssertHR(g_dx12.bundle->Close());
}

b32
on_update(pipeline_desc *previous_desc, b32 wireframe)
{
    local_persist pipeline_desc current_desc = {};
    b32 dirty_pipeline = false;

    //ImGui::Checkbox("Wireframe", &current_desc.wireframe);
    current_desc.wireframe = wireframe;
    current_desc.depth = true;
    current_desc.vertex = previous_desc->vertex;
    current_desc.pixel = previous_desc->pixel;

    if (memcmp(&current_desc, previous_desc, sizeof(pipeline_desc)) != 0) {
        *previous_desc = current_desc;
        dirty_pipeline = true;
    }

    f32 rotationSpeed = 0.015f;

    // Update the rotation constant
    g_render.rotation_angle += rotationSpeed;
    if (g_render.rotation_angle >= 2 * PI)
    {
        g_render.rotation_angle -= 2 * PI;
    }

    // Rotate the cube around the Y-axis
    g_render.world_matrix = glm::rotate(glm::mat4(1.0), -g_render.rotation_angle, glm::vec3(0, 1, 0));

    // const float translationSpeed = 0.005f;
    // const float offsetBounds = 1.25f;

    // g_dx12.constant_buffer_data.offset.x += translationSpeed;
    // if (g_dx12.constant_buffer_data.offset.x > offsetBounds)
    // {
    //     g_dx12.constant_buffer_data.offset.x = -offsetBounds;
    // }
    // memcpy(g_dx12.cbv_data_begin, &g_dx12.constant_buffer_data, sizeof(g_dx12.constant_buffer_data));

    return dirty_pipeline;
}

void
on_render()
{
    // Record all the commands we need to render the scene into the command list.
    populate_command_list(gfx.command_list);

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { gfx.command_list };
    gfx.command_queue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    AssertHR(gfx.swap.swap->Present(1, 0));

    //wait_for_previous_frame();
    move_to_next_frame();
}

void
on_destroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    wait_for_previous_frame();

    CloseHandle(gfx.fence_event);
}

internal void
populate_command_list(ID3D12GraphicsCommandList *command_list)
{
    AssertHR(gfx.command_allocator[gfx.frame_index]->Reset());
    AssertHR(command_list->Reset(gfx.command_allocator[gfx.frame_index], 0));

    // Set necessary state.
    command_set_pipeline_state(command_list, &g_dx12.state);

    // ID3D12DescriptorHeap* heaps[] = { gfx.cbv_srv_uav_heap, gfx.sampler_heap };
    ID3D12DescriptorHeap* heaps[] = { gfx.cbv_srv_uav_heap.heap };
    command_list->SetDescriptorHeaps(_countof(heaps), heaps);

    D3D12_GPU_DESCRIPTOR_HANDLE cbv_handle = gfx.cbv_srv_uav_heap.heap->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE srv_handle = cbv_handle;
    srv_handle.ptr += gfx.cbv_srv_uav_heap.increment_size; // advance by 1 slot

    // command_list->SetGraphicsRootDescriptorTable(0, cbv_handle);
    // command_list->SetGraphicsRootDescriptorTable(1, srv_handle);
    // command_list->SetGraphicsRootDescriptorTable(2, g_dx12.sampler_heap->GetGPUDescriptorHandleForHeapStart());

    command_list->RSSetViewports(1, &g_dx12.viewport);
    command_list->RSSetScissorRects(1, &g_dx12.scissor_rect);

    // Index into the available constant buffers based on the number
    // of draw calls. We've allocated enough for a known number of
    // draw calls per frame times the number of back buffers
    u32 constant_buffer_index = n_draw_calls * (gfx.frame_index % FRAME_COUNT);

    // Set the per-frame constants
    scene_constant_buffer cb_parameters = {};

    // Shaders compiled with default row-major matrices
    cb_parameters.world_matrix      = glm::transpose(g_render.world_matrix);
    cb_parameters.view_matrix       = glm::transpose(g_render.view_matrix);
    cb_parameters.projection_matrix = glm::transpose(g_render.projection_matrix);

    // Set the constants for the first draw call
    memcpy(g_dx12.constant_buffer_data + constant_buffer_index, &cb_parameters, sizeof(scene_constant_buffer));

    // Bind the constants to the shader
    D3D12_GPU_VIRTUAL_ADDRESS base_gpu_address = g_dx12.constant_data_gpu_addr + sizeof(scene_constant_buffer) * constant_buffer_index;
    command_list->SetGraphicsRootConstantBufferView(0, base_gpu_address);

    // Indicate that the back buffer will be used as a render target.
    // TODO: make this Barrier into a function
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = gfx.swap.render_targets[gfx.frame_index];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    command_list->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = gfx.rtv_heap.cpu;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = gfx.dsv_heap.cpu;
    rtv_handle.ptr += gfx.frame_index * gfx.rtv_heap.increment_size;

    command_list->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    command_list->ClearRenderTargetView(rtv_handle, clearColor, 0, 0);
    command_list->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, 0);

    command_list->ExecuteBundle(g_dx12.bundle);

    base_gpu_address += sizeof(scene_constant_buffer);
    ++constant_buffer_index;

    // Update the World matrix of the second cube
    glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0), glm::vec3(0.2f, 0.2f, 0.2f));
    glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.0), 2.0f * g_render.rotation_angle, glm::vec3(0, 1, 0));
    glm::mat4 translate_matrix = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, 0.0f, 20.0f));

    // Update the world variable to reflect the current light
    cb_parameters.world_matrix = glm::transpose(rotation_matrix * (scale_matrix * translate_matrix));

    // Set the constants for the draw call
    memcpy(g_dx12.constant_buffer_data + constant_buffer_index, &cb_parameters, sizeof(scene_constant_buffer));

    // Bind the constants to the shader
    command_list->SetGraphicsRootConstantBufferView(0, base_gpu_address);

    // Draw the second cube
    command_list->DrawIndexedInstanced(36, 1, 0, 0, 0);

    // Indicate that the back buffer will now be used to present.
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

    command_list->ResourceBarrier(1, &barrier);

    AssertHR(command_list->Close());
}

internal void
wait_for_previous_frame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    u64 fence = gfx.fence_value[gfx.frame_index];
    AssertHR(gfx.command_queue->Signal(gfx.fence, fence));
    gfx.fence_value[gfx.frame_index]++;

    // Wait until the previous frame is finished.
    if (gfx.fence->GetCompletedValue() < fence)
    {
        AssertHR(gfx.fence->SetEventOnCompletion(fence, gfx.fence_event));
        WaitForSingleObject(gfx.fence_event, INFINITE);
    }

    gfx.frame_index = gfx.swap.swap->GetCurrentBackBufferIndex();
}

// Wait for pending GPU work to complete.
internal void
wait_for_gpu()
{
    // Schedule a Signal command in the queue.
    AssertHR(gfx.command_queue->Signal(gfx.fence, gfx.fence_value[gfx.frame_index]));

    // Wait until the fence has been processed.
    AssertHR(gfx.fence->SetEventOnCompletion(gfx.fence_value[gfx.frame_index], gfx.fence_event));
    WaitForSingleObjectEx(gfx.fence_event, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    gfx.fence_value[gfx.frame_index]++;
}

// Prepare to render the next frame.
internal void
move_to_next_frame()
{
    // Schedule a Signal command in the queue.
    u64 current_fence_value = gfx.fence_value[gfx.frame_index];
    AssertHR(gfx.command_queue->Signal(gfx.fence, current_fence_value));

    // Update the frame index.
    gfx.frame_index = gfx.swap.swap->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (gfx.fence->GetCompletedValue() < gfx.fence_value[gfx.frame_index])
    {
        AssertHR(gfx.fence->SetEventOnCompletion(gfx.fence_value[gfx.frame_index], gfx.fence_event));
        WaitForSingleObjectEx(gfx.fence_event, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    gfx.fence_value[gfx.frame_index] = current_fence_value + 1;
}
