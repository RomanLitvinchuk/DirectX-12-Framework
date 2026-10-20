
Texture2D historyFrame : register(t0);
Texture2D currentFrame : register(t1);
Texture2D motionTexture : register(t2);


SamplerState s_PointClamp : register(s0);
SamplerState s_LinearClamp : register(s1);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};


float4 PS(PS_INPUT input) : SV_Target
{
    
    float2 velocityUV = motionTexture.Sample(s_PointClamp, input.TexCoord);
    float2 reprojectedUV = input.TexCoord + velocityUV;
    
    float4 currentColor = currentFrame.Sample(s_PointClamp, input.TexCoord);
    float4 historyColor = historyFrame.Sample(s_LinearClamp, reprojectedUV);
   
    float4 result = currentColor * 0.15 + historyColor * 0.85;
    return result;
}