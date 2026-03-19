#include "PerFrameConstants.hlsli"

struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 Position  : SV_POSITION;
    float4 Color     : COLOR0;
    float2 TexCoord  : TEXCOORD0;
    float  FogDist   : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 eyePos    = mul(float4(input.Position, 1.0f), WorldMatrix);
    output.Position  = mul(eyePos, ProjectionMatrix);
    output.Color     = MatDiffuse;
    output.TexCoord  = input.TexCoord;
    output.FogDist   = length(eyePos.xyz);
    return output;
}
