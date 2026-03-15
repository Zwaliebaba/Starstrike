#include "PerFrameConstants.hlsli"

Texture2D    TreeTexture : register(t0);
SamplerState TreeSampler : register(s0);

struct PSInput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR0;
    float2 TexCoord : TEXCOORD0;
    float  FogDist  : TEXCOORD1;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = TreeTexture.Sample(TreeSampler, input.TexCoord);
    float4 color = texColor * input.Color;

    if (FogEnabled)
    {
        float fogFactor = saturate((FogEnd - input.FogDist) / (FogEnd - FogStart));
        color.rgb = lerp(FogColor.rgb, color.rgb, fogFactor);
    }

    // --- Screen fade (applied last) ---
    color.rgb *= (1.0f - FadeAlpha);

    return color;
}
