// Dedicated landscape pixel shader — single-pass (base + overlay combined).
//
// Combines:
//   Pass 1 — lit vertex colour (opaque base terrain)
//   Pass 2 — triangle-outline overlay texture, additively blended on top
//
// The two legacy passes used different materials but the same geometry.
// This shader reproduces the visual result in a single draw:
//   final = baseColour + overlayTexture * overlayColour
//
// OverlayTint (set via cbuffer) carries the overlay material's brightness
// and alpha, replacing the per-draw material change of the old two-pass path.

#include "PerFrameConstants.hlsli"

Texture2D    OverlayTexture : register(t0);
SamplerState OverlaySampler : register(s0);

struct PSInput
{
    float4 Position  : SV_POSITION;
    float4 Color     : COLOR0;
    float2 TexCoord0 : TEXCOORD0;
    float  FogFactor : TEXCOORD1;
};

float4 main(PSInput input) : SV_TARGET
{
    // Base terrain colour (vertex-lit)
    float3 baseColor = input.Color.rgb;

    // Overlay: sample triangle-outline texture and add on top
    float4 overlayTex = OverlayTexture.Sample(OverlaySampler, input.TexCoord0);
    float3 overlay = overlayTex.rgb * overlayTex.a * input.Color.rgb;

    float3 color = baseColor + overlay;

    // Fog
    color = lerp(FogColor.rgb, color, input.FogFactor);

    // Screen fade
    color *= (1.0f - FadeAlpha);

    return float4(color, 1.0f);
}
