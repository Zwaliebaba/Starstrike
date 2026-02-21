
cbuffer CBPerDraw : register(b0)
{
  float4x4 gWorldViewProj;
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
  float4 pos      : SV_POSITION;
  float4 color    : COLOR;
  float2 texcoord : TEXCOORD;
  float3 normal   : NORMAL;
};

PSInput main(VSInput input)
{
  PSInput output;
  output.pos      = mul(float4(input.pos, 1.0f), gWorldViewProj);
  output.color    = input.color;
  output.texcoord = input.texcoord;
  output.normal   = input.normal;
  return output;
}
