// SpriteVertexShader.hlsl — 2D sprite/UI vertex shader for SpriteBatch (Phase 3)
//
// Input: screen-space position (float2), packed color (R8G8B8A8_UNORM), texcoord.
// Transforms position by ProjectionMatrix (orthographic) — no WorldMatrix needed.
// No lighting, no normals, no fog.

#include "PerFrameConstants.hlsli"

struct VSInput
{
    float2 Position : POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.Position = mul(float4(input.Position, 0.0f, 1.0f), ProjectionMatrix);
    output.Color    = input.Color;
    output.TexCoord = input.TexCoord;
    return output;
}
