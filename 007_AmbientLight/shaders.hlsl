cbuffer CBScene
{
	float4x4 VP;
	float4 light_dir;
	float4 ambient_color;
};

cbuffer CBModel
{
	float4x4 W;
	float4x4 IW;
};

struct VSInput
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 color : COLOR;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

PSInput VS(VSInput input)
{
	PSInput output;

	float3 inv_light = normalize(mul(light_dir, IW).xyz);
	float diffuse = clamp(dot(input.normal.xyz, inv_light), 0.0, 1.0);

	output.position = mul(mul(input.position, W), VP);
	output.color = input.color * float4(diffuse, diffuse, diffuse, 1.0) + ambient_color;

	return output;
}

float4 PS(PSInput input) : SV_TARGET
{
	return input.color;
}
