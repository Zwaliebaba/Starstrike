
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

struct VSInput
{
  float3 pos      : POSITION;
  float4 color    : COLOR;
  float2 texcoord : TEXCOORD;
  float3 normal   : NORMAL;
};

struct PSInput
{
  float4 pos       : SV_POSITION;
  float4 color     : COLOR;
  float2 texcoord  : TEXCOORD;
  float3 normalWS  : NORMAL;
  float3 posWS     : TEXCOORD1;
};

PSInput main(VSInput input)
{
  PSInput output;
  output.pos       = mul(float4(input.pos, 1.0f), gWorldViewProj);
  output.color     = input.color;
  output.texcoord  = input.texcoord;
  output.normalWS  = mul(input.normal, (float3x3)gWorld);
  output.posWS     = mul(float4(input.pos, 1.0f), gWorld).xyz;
  return output;
}
