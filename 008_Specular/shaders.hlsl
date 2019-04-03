cbuffer CBScene
{
	float4x4 VP;
	float4 light_dir;
	float4 ambient_color;
	float4 eye_dir;
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
	float3 inv_eye = normalize(mul(eye_dir, IW).xyz);
	float3 half_le = normalize(inv_light + inv_eye);
	float diffuse = clamp(dot(input.normal.xyz, inv_light), 0.0, 1.0);
	float specular = pow(clamp(dot(input.normal.xyz, half_le), 0.0, 1.0), 50.0);

	output.position = mul(mul(input.position, W), VP);
	output.color = input.color * float4(diffuse, diffuse, diffuse, 1.0) + ambient_color + float4(specular, specular, specular, 1.0);

	return output;
}

float4 PS(PSInput input) : SV_TARGET
{
	return input.color;
}
