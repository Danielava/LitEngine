cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 normal : COLOR0;
    float2 uv : TEXCOORD0;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
    float3 normal : COLOR0;
    float2 uv : TEXCOORD0;
};

PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);

	pos = mul(pos, model);
	pos = mul(pos, view);
	pos = mul(pos, projection);
	output.pos = pos;

    //float3x3 normalMat = transpose(inverse(float3x3(model)));
	/*
		You would know it's a world normal because no matter how you'd spin the model around, (e.g) the model part facing camera should always be the same color
	*/
    output.normal = normalize(mul(input.normal, (float3x3)model)); //To worldNormal
    //output.normal = input.normal;
	
    output.uv = input.uv;

	return output;
}
