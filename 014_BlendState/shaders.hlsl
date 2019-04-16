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

Texture2D tex1;
Texture2D tex2;
SamplerState ss;

struct VSInput
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

PSInput VS(VSInput input)
{
	PSInput output;

	output.position = mul(mul(input.position, W), VP);
	output.color = input.color;
	output.uv = input.uv;

	return output;
}

float4 PS(PSInput input) : SV_TARGET
{
	if(IW[0][0] < 1.0)
	{
		return float4(input.color.xyz, IW[0][0]);
	}
	else
	{
		return tex1.Sample(ss, input.uv)* tex1.Sample(ss, input.uv) * input.color;
	}
}
