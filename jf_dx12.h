#define FRAME_COUNT 2
#define RELEASE(object) if (object != 0) object->Release()

global_variable const u32 TextureWidth = 256;
global_variable const u32 TextureHeight = 256;
global_variable const u32 TexturePixelSize = 4; // # of bytes used to represent a pixel in the texture

// Implement Generic types
typedef ID3DBlob gfx_shader;

internal void check(HRESULT hr, char *msg);
internal void get_adapter(IDXGIFactory4 *factory, IDXGIAdapter1 **return_adapter, bool high_perf = false);

// descriptor heap

struct descriptor_heap
{
    ID3D12DescriptorHeap *heap;
    D3D12_DESCRIPTOR_HEAP_TYPE type;
    u32 size;
    u32 increment_size;

    D3D12_CPU_DESCRIPTOR_HANDLE cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu;
};

void descriptor_heap_init(descriptor_heap *heap, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 size);

// swapchain

struct swapchain
{
    IDXGISwapChain4 *swap;
    ID3D12Resource *render_targets[FRAME_COUNT];

    u32 width;
    u32 height;
};

void swapchain_init(swapchain *swap);

internal void load_pipeline();

enum root_signature_range_type
{
    PushConstants = 100,
    CBV = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
    SRV = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
    UAV = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
    Sampler = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER
};

enum root_signature_param_type
{
    Table = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
    Constant = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
    // note: if using root descriptor use the ones below
    CBV_Param = D3D12_ROOT_PARAMETER_TYPE_CBV,
    SRV_Param = D3D12_ROOT_PARAMETER_TYPE_SRV,
    UAV_Param = D3D12_ROOT_PARAMETER_TYPE_UAV,
};

struct root_signature_type
{
    root_signature_range_type range;
    root_signature_param_type param = Table;
};

void root_signature_init(root_signature_type *types, u32 types_len);

// sampler

enum sampler_address
{
    Wrap = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
    Mirror = D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
    Clamp = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
    Border = D3D12_TEXTURE_ADDRESS_MODE_BORDER
};

enum sampler_filter
{
    Linear = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
    Nearest = D3D12_FILTER_MIN_MAG_MIP_POINT,
    Anisotropic = D3D12_FILTER_ANISOTROPIC
};

void sampler_init(sampler_address address, sampler_filter filter, b8 mips);

// graphics pipeline

enum depth_func
{
    Less = D3D12_COMPARISON_FUNC_LESS,
    None = D3D12_COMPARISON_FUNC_NEVER,
    Greater = D3D12_COMPARISON_FUNC_GREATER,
};

struct pipeline_desc {
    bool wireframe = false;

    bool depth = false;
    depth_func func = Less;
    DXGI_FORMAT dsv_format = DXGI_FORMAT_D32_FLOAT;
    gfx_shader *vertex;
    gfx_shader *pixel;
};

struct pipeline_state {
    ID3D12PipelineState *state;
    ID3D12RootSignature *root_signature;
};

internal void record_bundle(pipeline_state *pso);

internal void gfx_pipeline_init(pipeline_state *pso, pipeline_desc desc);
internal void gfx_pipeline_rebuild(pipeline_state *pso, pipeline_desc desc);

internal void load_assets();

b32 on_update(pipeline_desc *previous_desc, b32 wireframe);
void on_render();
void on_destroy();

internal void populate_command_list(ID3D12GraphicsCommandList *command_list);
internal void wait_for_previous_frame();
internal void wait_for_gpu();
internal void move_to_next_frame();

// comands

void command_set_pipeline_state(ID3D12GraphicsCommandList *cmd, pipeline_state *pso);

struct scene_constant_buffer;
struct dx12_t {
    // Pipeline objects.
    D3D12_VIEWPORT viewport;
    D3D12_RECT scissor_rect;

    ID3D12Resource *depth_stencil;

    ID3D12CommandAllocator    *bundle_allocator;
    ID3D12GraphicsCommandList *bundle;

    pipeline_state state;

    // App resources.
    ID3D12Resource *vertex_buffer;
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;

    ID3D12Resource *index_buffer;
    D3D12_INDEX_BUFFER_VIEW index_buffer_view;

    u8                        *cbv_data_begin;
    ID3D12Resource            *constant_buffer;
    scene_constant_buffer     *constant_buffer_data;
    D3D12_GPU_VIRTUAL_ADDRESS constant_data_gpu_addr;

    ID3D12Resource *texture;
};

global_variable dx12_t g_dx12 = {};
