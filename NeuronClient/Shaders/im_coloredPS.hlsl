
struct PSInput
{
  float4 pos      : SV_POSITION;
  float4 color    : COLOR;
  float2 texcoord : TEXCOORD;
  float3 normal   : NORMAL;
};

float4 main(PSInput input) : SV_Target
{
  return input.color;
}
