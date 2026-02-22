
Texture2D    gTex : register(t0);
SamplerState gSmp : register(s0);

struct PSIn
{
  float4 pos   : SV_POSITION;
  float2 uv    : TEXCOORD;
  float4 color : COLOR;
};

float4 main(PSIn p) : SV_Target
{
  float4 c = gTex.Sample(gSmp, p.uv) * p.color;
  clip(c.a - 0.001f);
  return c;
}
