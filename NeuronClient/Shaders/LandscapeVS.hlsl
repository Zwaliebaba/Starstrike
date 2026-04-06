// Dedicated landscape vertex shader — single-pass (base + overlay combined).
// Replaces the uber shader for terrain rendering.

#include "PerFrameConstants.hlsli"

struct VSInput
{
    float3 Position  : POSITION;
    float3 Normal    : NORMAL;
    float4 Color     : COLOR;
    float2 TexCoord0 : TEXCOORD0;
    float2 TexCoord1 : TEXCOORD1;
};

struct VSOutput
{
    float4 Position  : SV_POSITION;
    float4 Color     : COLOR0;
    float2 TexCoord0 : TEXCOORD0;
    float  FogFactor : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // Transform position
    float4 worldPos = mul(float4(input.Position, 1.0f), WorldMatrix);
    output.Position = mul(worldPos, ProjectionMatrix);

    // Pass through overlay UVs
    output.TexCoord0 = input.TexCoord0;

    // ---- Lighting (always on for landscape) ----
    float3 normal = normalize(mul(input.Normal, (float3x3) WorldMatrix));

    // Color-material: vertex colour drives ambient + diffuse
    float4 matAmbient = input.Color;
    float4 matDiffuse = input.Color;

    float3 ambient = GlobalAmbient.rgb * matAmbient.rgb;
    float3 diffuseAccum = float3(0, 0, 0);

    // Landscape uses lights 0 and 1 only, no specular (MatSpecular is black
    // in the main pass, negligible grey in the overlay — skip entirely).
    [unroll]
    for (uint i = 0; i < 2; i++)
    {
        if (!Lights[i].Enabled)
            continue;

        float3 lightDir;
        if (Lights[i].Position.w == 0.0f) // directional
            lightDir = normalize(-Lights[i].Position.xyz);
        else
        {
            float3 toLight = Lights[i].Position.xyz - worldPos.xyz;
            lightDir = normalize(toLight);
        }

        ambient += Lights[i].Ambient.rgb * matAmbient.rgb;

        float NdotL = max(dot(normal, lightDir), 0.0f);
        diffuseAccum += Lights[i].Diffuse.rgb * NdotL;
    }

    float3 finalColor = ambient + diffuseAccum * matDiffuse.rgb;
    output.Color = float4(saturate(finalColor), matDiffuse.a);

    // ---- Fog (pre-compute factor in VS) ----
    float dist = length(worldPos.xyz);
    float range = FogEnd - FogStart;
    output.FogFactor = (range > 0.0001f) ? saturate((FogEnd - dist) / range) : 1.0f;

    return output;
}
