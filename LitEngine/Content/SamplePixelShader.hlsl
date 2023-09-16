Texture2D<float4> Albedo : register(t0);
SamplerState Sampler : register(s0);

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 color = Albedo.Sample(Sampler, input.uv).xyz; //Causes a crash atm..
	//return float4(input.color, 1.0f);
    return float4(color, 1.0f);
}
