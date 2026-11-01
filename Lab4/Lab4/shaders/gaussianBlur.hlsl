
Texture2D sourceTexture : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

cbuffer BlurParams : register(b0)
{
    int2 direction;
    int2 textureSize;
}


[numthreads(8, 8, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{ 
    if (DTid.x >= textureSize.x || DTid.y >= textureSize.y)
    {
        return;
    }
    
    float weights[5] =
    {
        0.06136,
        0.24477,
        0.38774,
        0.24477,
        0.06136
    };
    
    float4 color = 0;
    for (int i = -2; i <= 2; i++)
    {
        int2 uv = int2(DTid.xy) + direction * i;
        uv = clamp(uv, int2(0, 0), textureSize - 1);
        color += sourceTexture.Load(int3(uv, 0)) * weights[i + 2];
    }
    
    outputTexture[DTid.xy] = color;

}