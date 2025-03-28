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

void MainPS(PSInput input, out float4 color : SV_TARGET)
{
    color = input.color;
    return;
}