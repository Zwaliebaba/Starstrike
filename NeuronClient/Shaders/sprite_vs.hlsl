
cbuffer CBSprite : register(b0)
{
  float4x4 gViewProj;
};

struct VSIn
{
  float3 pos   : POSITION;
  float2 uv    : TEXCOORD;
  float4 color : COLOR;
};

struct PSIn
{
  float4 pos   : SV_POSITION;
  float2 uv    : TEXCOORD;
  float4 color : COLOR;
};

PSIn main(VSIn v)
{
  PSIn o;
  o.pos   = mul(float4(v.pos, 1.0f), gViewProj);
  o.uv    = v.uv;
  o.color = v.color;
  return o;
}
