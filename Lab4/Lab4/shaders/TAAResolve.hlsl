
Texture2D historyFrame : register(t0);
Texture2D currentFrame : register(t1);
Texture2D currentVelocityTexture : register(t2);
Texture2D previousVelocityTexture : register(t3);
Texture2D currentFrameBlurred : register(t4);

SamplerState s_PointClamp : register(s0);
SamplerState s_LinearClamp : register(s1);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};


float4 PS(PS_INPUT input) : SV_Target
{
    uint width, height;
    currentFrame.GetDimensions(width, height);
    
    float2 texelSize = 1.0f / float2(width, height);
    
    float2 currentVelocityUV = currentVelocityTexture.Sample(s_PointClamp, input.TexCoord);
    float2 reprojectedUV = input.TexCoord + currentVelocityUV;
    
    float4 currentColor = currentFrame.Sample(s_PointClamp, input.TexCoord);
    float4 currentColorBlurred = currentFrameBlurred.Sample(s_PointClamp, input.TexCoord);
    float4 historyColor = historyFrame.Sample(s_LinearClamp, reprojectedUV);
    
    float3 minColor = float3(9999.0f, 9999.0f, 9999.0f);
    float3 maxColor = float3(-9999.0f, -9999.0f, -9999.0f);
    
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float3 color = currentFrame.Sample(s_LinearClamp, input.TexCoord + float2(x, y) * texelSize).rgb;
            minColor = min(minColor, color);
            maxColor = max(maxColor, color);
        }

    }
    float3 historyColorClamped = clamp(historyColor.rgb, minColor, maxColor);
    
    float2 previousVelocityUV = previousVelocityTexture.Sample(s_LinearClamp, input.TexCoord + currentVelocityUV);
    float velocityLength = length(previousVelocityUV - currentVelocityUV);
    float velocityDisocclusion = saturate((velocityLength - 0.001) * 10.0);
   
    float3 accumulation = currentColor.rgb * 0.1 + historyColorClamped * 0.9;
    float3 result = lerp(accumulation, currentColorBlurred.rgb, velocityDisocclusion);
    return float4(result, currentColor.a);
}