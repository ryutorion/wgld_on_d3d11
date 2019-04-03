cbuffer CBScene
{
	float4x4 WVP;
};

float4 VS( float4 position : POSITION ) : SV_POSITION
{
	return mul(position, WVP);
}

float4 PS( float4 position : SV_POSITION ) : SV_TARGET
{
	return float4(1.0, 1.0, 1.0, 1.0);
}
