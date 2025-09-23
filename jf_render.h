#pragma pack(push, 1)
struct vertex
{
    glm::vec3 position[3];
    glm::vec3 color[4];
};

struct scene_constant_buffer
{
    glm::mat4 world_matrix;        // 64 bytes
    glm::mat4 view_matrix;         // 64 bytes
    glm::mat4 projection_matrix;   // 64 bytes
    glm::vec4 padding[4];          // Padding so the constant buffer is 256-byte aligned.
};
#pragma pack(pop)
static_assert((sizeof(scene_constant_buffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

struct render_t
{
    // Scene constants, updated per-frame
    float rotation_angle;

    // These computed values will be loaded into a ConstantBuffer
    // during Render
    glm::mat4 world_matrix;
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;

    pipeline_desc desc;
};

// In this simple sample, we know that there are two draw calls
// and we will update the scene constants for each draw call.
global_variable u32 n_draw_calls = 2;

void scene_init();
void scene_update(bool wireframe);

global_variable render_t g_render = {};
