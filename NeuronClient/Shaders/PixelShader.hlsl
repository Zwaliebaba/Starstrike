// Uber pixel shader — fixed-function emulation for OpenGL-on-D3D12

#include "PerFrameConstants.hlsli"

// Texture env mode constants (match GL defines remapped to shader constants)
#define TEXENV_MODULATE 0
#define TEXENV_REPLACE  1
#define TEXENV_DECAL    2
#define TEXENV_COMBINE  3

// Alpha test func constants
#define FUNC_LEQUAL  0
#define FUNC_GREATER 1

Texture2D Texture0 : register(t0);
Texture2D Texture1 : register(t1);
SamplerState Sampler0 : register(s0);
SamplerState Sampler1 : register(s1);

struct PSInput
{
    float4 Position  : SV_POSITION;
    float4 Color     : COLOR0;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
    float  FogDist   : TEXCOORD2;
    float3 WorldPos  : TEXCOORD3;
};

float4 ApplyTexEnv(float4 texColor, float4 fragColor, uint mode)
{
    if (mode == TEXENV_REPLACE)
        return float4(texColor.rgb, texColor.a);
    else if (mode == TEXENV_DECAL)
        return float4(lerp(fragColor.rgb, texColor.rgb, texColor.a), fragColor.a);
    else // MODULATE (default)
        return texColor * fragColor;
}

float4 main(PSInput input) : SV_TARGET
{
    float4 color = input.Color;

    // Apply texture 0
    if (TexturingEnabled0)
    {
        float4 texColor = Texture0.Sample(Sampler0, input.TexCoord0);
        color = ApplyTexEnv(texColor, color, TexEnvMode0);
    }

    // Apply texture 1 (multitexture)
    if (TexturingEnabled1)
    {
        float4 texColor = Texture1.Sample(Sampler1, input.TexCoord1);
        color = ApplyTexEnv(texColor, color, TexEnvMode1);
    }

    // Fog
    if (FogEnabled)
    {
        float fogFactor = 1.0f;
        float dist = input.FogDist;
        float range = FogEnd - FogStart;
        if (range > 0.0001f)
            fogFactor = saturate((FogEnd - dist) / range);
        color.rgb = lerp(FogColor.rgb, color.rgb, fogFactor);
    }

    // Alpha test
    if (AlphaTestEnabled)
    {
        bool pass = true;
        if (AlphaTestFunc == FUNC_LEQUAL)
            pass = (color.a <= AlphaTestRef);
        else if (AlphaTestFunc == FUNC_GREATER)
            pass = (color.a > AlphaTestRef);
        if (!pass)
            discard;
    }

    // --- Screen fade (applied last) ---
    color.rgb *= (1.0f - FadeAlpha);

    return color;
}
