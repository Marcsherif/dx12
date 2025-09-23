#ifdef DX12
    #include <d3d12.h>
    #include <dxgi1_6.h>
    #include <dxgidebug.h>
    #include <D3Dcompiler.h>

    #pragma comment (lib, "gdi32")
    #pragma comment (lib, "user32")
    #pragma comment (lib, "dxguid")
    #pragma comment (lib, "dxgi")
    #pragma comment (lib, "d3d12")
    #pragma comment (lib, "d3dcompiler")

    #include "jf_dx12.h"
#elif OPENGL
    #error "There is only a DX12 renderer at the moment"
#elif METAL
    #error "There is only a DX12 renderer at the moment"
#else
    #error "No renderer defined"
#endif

struct gfx_context {
    IDXGIFactory7* factory;
    IDXGIAdapter1* adapter;
    ID3D12Device* device;

    ID3D12CommandQueue* command_queue;

    ID3D12CommandAllocator    *command_allocator[FRAME_COUNT];
    ID3D12GraphicsCommandList *command_list;

    descriptor_heap rtv_heap;
    descriptor_heap dsv_heap;
    descriptor_heap cbv_srv_uav_heap;
    descriptor_heap sampler_heap;

    swapchain swap;

    // Synchronization objects.
    HANDLE fence_event;
    ID3D12Fence *fence;
    u64 fence_value[FRAME_COUNT];
    u32 frame_index;
};

struct gfx_frame {
    ID3D12CommandAllocator    *command_allocator;
    ID3D12GraphicsCommandList *command_list;

    ID3D12Resource* backbuffer_resource;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle;

    u32 frame_index;
};

global_variable gfx_context gfx = {};

//
// Generic Backend Structs

struct gfx_defaults;
struct gfx_buffer;
struct gfx_buffer_info;

// typedef gfx_shader u8*;
enum gfx_shader_type {
    GFX_VERTEX_SHADER,
    GFX_PIXEL_SHADER,
};

//
// Public Functions

// setup and misc functions
void gfx_init();
// void gfx_shutdown(void);
// bool gfx_isvalid(void);
// void gfx_reset_state_cache(void);
// gfx_trace_hooks sg_install_trace_hooks(const sg_trace_hooks* trace_hooks);
// void gfx_push_debug_group(const char* name);
// void gfx_pop_debug_group(void);
// bool gfx_add_commit_listener(sg_commit_listener listener);
// bool gfx_remove_commit_listener(sg_commit_listener listener);


// resource creation, destruction and updating
// gfx_buffer sg_make_buffer(gfx_buffer_info* info);
// gfx_image sg_make_image(const sg_image_desc* desc);
// gfx_sampler sg_make_sampler(const sg_sampler_desc* desc);
// gfx_shader sg_make_shader(const sg_shader_desc* desc);
gfx_shader *gfx_create_shader(wchar_t *path, char* entrypoint, gfx_shader_type type);
void gfx_pipeline_init(pipeline_desc desc);
// gfx_attachments sg_make_attachments(const sg_attachments_desc* desc);
// void gfx_destroy_buffer(sg_buffer buf);
// void gfx_destroy_image(sg_image img);
// void gfx_destroy_sampler(sg_sampler smp);
// void gfx_destroy_shader(sg_shader shd);
// void gfx_destroy_pipeline(sg_pipeline pip);
// void gfx_destroy_attachments(sg_attachments atts);
// void gfx_update_buffer(sg_buffer buf, const sg_range* data);
// void gfx_update_image(sg_image img, const sg_image_data* data);
// int gfx_append_buffer(sg_buffer buf, const sg_range* data);
// bool gfx_query_buffer_overflow(sg_buffer buf);
// bool gfx_query_buffer_will_overflow(sg_buffer buf, size_t size);
#if 0
// render and compute functions
void gfx_begin_pass(const sg_pass* pass);
void gfx_apply_viewport(int x, int y, int width, int height, bool origin_top_left);
// void gfx_apply_viewportf(float x, float y, float width, float height, bool origin_top_left);
void gfx_apply_scissor_rect(int x, int y, int width, int height, bool origin_top_left);
// void gfx_apply_scissor_rectf(float x, float y, float width, float height, bool origin_top_left);
void gfx_apply_pipeline(sg_pipeline pip);
// void gfx_apply_bindings(const sg_bindings* bindings);
void gfx_apply_uniforms(int ub_slot, const sg_range* data);
void gfx_draw(int base_element, int num_elements, int num_instances);
// void gfx_dispatch(int num_groups_x, int num_groups_y, int num_groups_z);
void gfx_end_pass(void);
void gfx_commit(void);
#endif

// getting information
// gfx_desc sg_query_desc(void);
// gfx_backend sg_query_backend(void);
// gfx_features sg_query_features(void);
// gfx_limits sg_query_limits(void);
// gfx_pixelformat_info sg_query_pixelformat(sg_pixel_format fmt);
// int gfx_query_row_pitch(sg_pixel_format fmt, int width, int row_align_bytes);
// int gfx_query_surface_pitch(sg_pixel_format fmt, int width, int height, int row_align_bytes);
// // get current state of a resource (INITIAL, ALLOC, VALID, FAILED, INVALID)
// gfx_resource_state sg_query_buffer_state(sg_buffer buf);
// gfx_resource_state sg_query_image_state(sg_image img);
// gfx_resource_state sg_query_sampler_state(sg_sampler smp);
// gfx_resource_state sg_query_shader_state(sg_shader shd);
// gfx_resource_state sg_query_pipeline_state(sg_pipeline pip);
// gfx_resource_state sg_query_attachments_state(sg_attachments atts);
// // get runtime information about a resource
// gfx_buffer_info sg_query_buffer_info(sg_buffer buf);
// gfx_image_info sg_query_image_info(sg_image img);
// gfx_sampler_info sg_query_sampler_info(sg_sampler smp);
// gfx_shader_info sg_query_shader_info(sg_shader shd);
// gfx_pipeline_info sg_query_pipeline_info(sg_pipeline pip);
// gfx_attachments_info sg_query_attachments_info(sg_attachments atts);
// // get desc structs matching a specific resource (NOTE that not all creation attributes may be provided)
// gfx_buffer_desc sg_query_buffer_desc(sg_buffer buf);
// gfx_image_desc sg_query_image_desc(sg_image img);
// gfx_sampler_desc sg_query_sampler_desc(sg_sampler smp);
// gfx_shader_desc sg_query_shader_desc(sg_shader shd);
// gfx_pipeline_desc sg_query_pipeline_desc(sg_pipeline pip);
// gfx_attachments_desc sg_query_attachments_desc(sg_attachments atts);
// // get resource creation desc struct with their default values replaced
// gfx_buffer_desc sg_query_buffer_defaults(const sg_buffer_desc* desc);
// gfx_image_desc sg_query_image_defaults(const sg_image_desc* desc);
// gfx_sampler_desc sg_query_sampler_defaults(const sg_sampler_desc* desc);
// gfx_shader_desc sg_query_shader_defaults(const sg_shader_desc* desc);
// gfx_pipeline_desc sg_query_pipeline_defaults(const sg_pipeline_desc* desc);
// gfx_attachments_desc sg_query_attachments_defaults(const sg_attachments_desc* desc);
// // assorted query functions
// size_t gfx_query_buffer_size(sg_buffer buf);
// gfx_buffer_usage sg_query_buffer_usage(sg_buffer buf);
// gfx_image_type sg_query_image_type(sg_image img);
// int gfx_query_image_width(sg_image img);
// int gfx_query_image_height(sg_image img);
// int gfx_query_image_num_slices(sg_image img);
// int gfx_query_image_num_mipmaps(sg_image img);
// gfx_pixel_format sg_query_image_pixelformat(sg_image img);
// gfx_image_usage sg_query_image_usage(sg_image img);
// int gfx_query_image_sample_count(sg_image img);

//
// Generic Backend Functions

// internal void gfx_setup(const sg_desc* desc);
// internal void gfx_discard(void);
// internal void gfx_reset_state_cache(void);

// internal sg_resource_state gfx_create_buffer(_sg_buffer_t* buf, const sg_buffer_desc* desc);
// internal void gfx_discard_buffer(_sg_buffer_t* buf);

// internal sg_resource_state gfx_create_image(_sg_image_t* img, const sg_image_desc* desc);
// internal void gfx_discard_image(_sg_image_t* img);

// internal sg_resource_state gfx_create_sampler(_sg_sampler_t* smp, const sg_sampler_desc* desc);
// internal void gfx_discard_sampler(_sg_sampler_t* smp);

// internal sg_resource_state gfx_create_shader(_sg_shader_t* shd, const sg_shader_desc* desc);
// internal void gfx_discard_shader(_sg_shader_t* shd);

// internal sg_resource_state gfx_create_pipeline(_sg_pipeline_t* pip, const sg_pipeline_desc* desc);
// internal void gfx_discard_pipeline(_sg_pipeline_t* pip);

// internal sg_resource_state gfx_create_attachments(_sg_attachments_t* atts, const sg_attachments_desc* desc);
// internal void gfx_discard_attachments(_sg_attachments_t* atts);

// internal void gfx_begin_pass(const sg_pass* pass);
// internal void gfx_end_pass(void);

// internal void gfx_apply_viewport(int x, int y, int w, int h, bool origin_top_left);
// internal void gfx_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left);
// internal void gfx_apply_pipeline(_sg_pipeline_t* pip);
// internal bool gfx_apply_bindings(_sg_bindings_ptrs_t* bnd);
// internal void gfx_apply_uniforms(int ub_slot, const sg_range* data);

// internal void gfx_draw(int base_element, int num_elements, int num_instances);
// internal void gfx_dispatch(int num_groups_x, int num_groups_y, int num_groups_z);
// internal void gfx_commit(void);

// internal void gfx_update_buffer(_sg_buffer_t* buf, const sg_range* data);
// internal void gfx_append_buffer(_sg_buffer_t* buf, const sg_range* data, bool new_frame);

// internal void gfx_update_image(_sg_image_t* img, const sg_image_data* data);
// internal void gfx_push_debug_group(const char* name);
