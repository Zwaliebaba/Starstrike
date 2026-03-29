// SpritePixelShader.hlsl — 2D sprite/UI pixel shader for SpriteBatch (Phase 3)
//
// Samples texture × vertex color.  Applies FadeAlpha from SceneConstants.
// No lighting, no fog, no alpha test.  Blend mode is controlled by PSO.

#include "PerFrameConstants.hlsli"

Texture2D    SpriteTexture : register(t0);
SamplerState SpriteSampler : register(s0);

struct PSInput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    float4 color = texColor * input.Color;

    // Screen fade (applied last) — matches TreePixelShader / uber pixel shader
    color.rgb *= (1.0f - FadeAlpha);

    return color;
}
