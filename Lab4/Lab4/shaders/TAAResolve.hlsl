Texture2D currentFrame : register(t0);
Texture2D historyFrame : register(t1);

SamplerState s_PointClamp : register(s0);
SamplerState s_LinearClamp : register(s1);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};


float4 PS(PS_INPUT input) : SV_Target
{
    float4 currentColor = currentFrame.Sample(s_PointClamp, input.TexCoord);
    float4 historyColor = historyFrame.Sample(s_LinearClamp, input.TexCoord);
    
    float4 result = currentColor * 0.1 + historyColor * 0.9;
    return result;
}