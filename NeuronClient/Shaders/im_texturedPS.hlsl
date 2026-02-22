
Texture2D    gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer CBPerDraw : register(b0)
{
  float4x4 gWorldViewProj;
  float4x4 gWorld;
  float4 gLightDir[2];
  float4 gLightColor[2];
  float4 gAmbientColor;
  float4 gFogColor;
  float4 gCameraPos;
  float  gFogStart;
  float  gFogEnd;
  float  gAlphaClipThreshold;
  int    gNumLights;
  int    gLightingEnabled;
  int    gFogEnabled;
  float2 _pad0;
};

struct PSInput
{
  float4 pos       : SV_POSITION;
  float4 color     : COLOR;
  float2 texcoord  : TEXCOORD;
  float3 normalWS  : NORMAL;
  float3 posWS     : TEXCOORD1;
};

float4 main(PSInput input) : SV_Target
{
  float4 color = gTexture.Sample(gSampler, input.texcoord) * input.color;

  if (gLightingEnabled)
  {
    float3 N = normalize(input.normalWS);
    float3 lit = gAmbientColor.rgb;
    for (int i = 0; i < gNumLights; i++)
    {
      float NdotL = max(dot(N, gLightDir[i].xyz), 0.0f);
      lit += NdotL * gLightColor[i].rgb;
    }
    color.rgb *= lit;
  }

  if (gFogEnabled)
  {
    float dist = length(input.posWS - gCameraPos.xyz);
    float fogFactor = saturate((gFogEnd - dist) / (gFogEnd - gFogStart));
    color.rgb = lerp(gFogColor.rgb, color.rgb, fogFactor);
  }

  if (color.a <= gAlphaClipThreshold)
    discard;

  return color;
}
