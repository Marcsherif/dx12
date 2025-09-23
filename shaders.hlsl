cbuffer SceneConstantBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
}

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

// Texture2D g_texture : register(t0);
// SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result = (PSInput)0;

    result.position = mul(position, World);
    result.position = mul(result.position, View);
    result.position = mul(result.position, Projection);
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
