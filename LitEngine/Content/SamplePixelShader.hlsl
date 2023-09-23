Texture2D<float4> Albedo : register(t0);
SamplerState Sampler : register(s0);

//Increase z will push light towards camera. 
//Increase y and light will move up
//Increase x and light will move right
#define LIGHT_DIRECTION normalize(float3(2.0f, 3.0f, 3.0f))
#define LIGHT_SOURCE_COLOR float3(1, 1, 1)
#define AMBIENT 0.05f

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
    float3 normal : COLOR0;
    float2 uv : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 color = Albedo.Sample(Sampler, input.uv).xyz;
    float3 normal = input.normal;
    //normal = (normal + 1) * 0.5;
    
    //Applying light
    float lightIntensity = AMBIENT + max(dot(normal, LIGHT_DIRECTION), 0);
    float3 lightColor = color * LIGHT_SOURCE_COLOR * lightIntensity;
    
    return float4(lightColor, 1.0f);
}
