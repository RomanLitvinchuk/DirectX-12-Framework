cbuffer cbPerObject : register(b0)
{
    float4x4 mView;
    float4x4 mProj;
    float gTime;
    float pad[3];
}

cbuffer cbMaterial : register(b1)
{
    
    float4x4 mMatTransform;
    
    float4 gDiffuseColor;
    float4 gAmbientColor;
    float4 gSpecularColor;
    float4 gEmissiveColor;
    float4 gTransparentColor;

    float gShininess;
    float gOpacity;
    float gRefractionIndex;
    int hasNormalTexture;
    
    int gIsLion;
    int gIsTree;
    int gHasDisplacementTexture;
    int gHasShadowTexture;
    
    int hasBillboardTexture;
    int3 padding;
}

Texture2D DiffuseMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D DisplacementMap : register(t2);

struct InstanceData
{
    row_major float4x4 mWorld;
    row_major float4x4 mTexTransform;
    row_major float4x4 mInvTWorld;
};

StructuredBuffer<InstanceData> Instances : register(t3);

SamplerState Sampler : register(s0);

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

struct VS_TESS_OUTPUT
{
    float4 pos : POSITION;
    float4 Wpos : WORLDPOS;
    float3 normal : NORMAL0;
    float3 normalW : NORMAL1;
    float2 uv : TEXCOORD0;
    float3 tangentW : TANGENT;
};

struct DS_OUTPUT
{
    //float4 pos : SV_POSITION;
    float3 worldPos : WORLDPOS;
    float3 normal : NORMAL0;
    float3 normalW : NORMAL1;
    float2 uv : TEXCOORD0;
    float3 tangentW : TANGENT;
};

float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 tangentW)
{
    float3 normalT = 2.0f * normalMapSample - 1.0f;

    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);

    float3 bumpedNormalW = mul(normalT, TBN);

    return bumpedNormalW;
}

VS_TESS_OUTPUT TessVS(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_TESS_OUTPUT output;
    if (gIsLion == 1)
    {
        
        float offset = sin(gTime * 8.0f + input.pos.y * 0.7f) * 1.0f;
        input.pos += input.normal * offset;
    }
    float4x4 mWorld = Instances[instanceID].mWorld;
    float4x4 mInvTWorld = Instances[instanceID].mInvTWorld;
    float4x4 mTexTransform = Instances[instanceID].mTexTransform;
    output.Wpos = mul(float4(input.pos, 1.0f), mWorld);
    output.pos = float4(input.pos, 1.0f);
    output.normal = input.normal;
    output.normalW = mul(input.normal, (float3x3) mInvTWorld);
    output.tangentW = mul(input.tangent, (float3x3) mInvTWorld);
    float4 texC = mul(float4(input.uv, 0.0f, 1.0f), mTexTransform);
    output.uv = mul(texC, mMatTransform).xy;
    return output;
}

struct gBufferOutput
{
    float4 Diffuse : SV_Target0;
    float4 Normal : SV_Target1;
};

gBufferOutput TessPS(DS_OUTPUT input)
{
    gBufferOutput ret;
    ret.Diffuse = DiffuseMap.Sample(Sampler, input.uv.xy);
    
    if (hasNormalTexture == 1)
    {
        input.normalW = normalize(input.normalW);
        float4 normalSample = NormalMap.Sample(Sampler, input.uv.xy);
        float3 bumpedNormalW = NormalSampleToWorldSpace(normalSample.xyz, input.normalW, input.tangentW);
        ret.Normal = float4(bumpedNormalW.x, bumpedNormalW.y, bumpedNormalW.z, 0.0f);
    }
    else
    {
        ret.Normal = float4(input.normalW.x, input.normalW.y, input.normalW.z, 0.0f);
    }
    return ret;
}