
Texture2D    gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PSInput
{
  float4 pos      : SV_POSITION;
  float4 color    : COLOR;
  float2 texcoord : TEXCOORD;
  float3 normal   : NORMAL;
};

float4 main(PSInput input) : SV_Target
{
  return gTexture.Sample(gSampler, input.texcoord) * input.color;
}
