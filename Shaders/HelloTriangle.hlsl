struct PositionColor {
    float4 sv_pos : SV_POSITION;
    float4 color : COLOR;
};


void MainVS(
    float4 position : POSITION,
    float4 color : COLOR,
    out PositionColor output)
{
    output.sv_pos = position;
    output.color = color;
}


void MainPS(
    PositionColor input,
    out float4 color : SV_TARGET)
{
    color = input.color;
}