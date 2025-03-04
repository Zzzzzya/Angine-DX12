struct PSInput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};
 
PSInput MainVS(float4 position : POSITION, float4 color : COLOR)
{
	PSInput result;
 
	result.position = position;
	result.color = color;
 
	return result;
}
 
float4 MainPS(PSInput input) : SV_TARGET
{
	return input.color;
}