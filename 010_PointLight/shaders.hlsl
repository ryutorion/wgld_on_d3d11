cbuffer CBScene
{
	float4x4 VP;
};

cbuffer CBLight
{
	float4 light_pos;
	float4 ambient_color;
	float4 eye_dir;
};

cbuffer CBModelVS
{
	float4x4 W;
};

cbuffer CBModelPS
{
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
	float3 normal : NORMAL;
	float3 world_position: POSITION;
};

PSInput VS(VSInput input)
{
	PSInput output;

	output.position = mul(mul(input.position, W), VP);
	output.color = input.color;
	output.normal = input.normal;
	output.world_position = mul(input.position, W);

	return output;
}

float4 PS(PSInput input) : SV_TARGET
{
	float3 light_dir = light_pos.xyz - input.world_position;
	float3 inv_light = normalize(mul(float4(light_dir, 0.0), IW).xyz);
	float3 inv_eye = normalize(mul(eye_dir, IW).xyz);
	float3 half_le = normalize(inv_light + inv_eye);
	float diffuse = clamp(dot(input.normal.xyz, inv_light), 0.0, 1.0) + 0.2;
	float specular = pow(clamp(dot(input.normal.xyz, half_le), 0.0, 1.0), 50.0);

	return input.color * float4(diffuse, diffuse, diffuse, 1.0) + ambient_color + float4(specular, specular, specular, 1.0);
}
