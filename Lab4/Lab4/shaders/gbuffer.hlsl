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
    int gDiffuseTextureIndex;
    int normalTextureIndex;        
    
    int gDisplacementTextureIndex;
    int gHasDisplacementTexture;
    int gShadowTextureIndex;
    int gHasShadowTexture;
    
    int billboardTextureIndex;
    int hasbillboardTexture;
    int2 pad2;
}

cbuffer cbViewport : register(b3)
{
    float screenHeight;
    float screenWidth;
    float2 jitter;
    float2 prevJitter;
}

cbuffer cbPrevMatrices : register(b4)
{
    float4x4 prevView;
    float4x4 prevProj;
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

StructuredBuffer<InstanceData> CurrentInstances : register(t3);
StructuredBuffer<InstanceData> PreviousInstances : register(t4);

SamplerState Sampler : register(s0);

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 Wpos : WORLDPOS;
    float3 normal : NORMAL0;
    float3 normalW : NORMAL1;
    float2 uv : TEXCOORD0;
    float3 tangentW : TANGENT;
    float4 currentClipPos : TEXCOORD1;
    float4 prevClipPos : TEXCOORD2;
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

float2 ClipToUV(float4 clipPos)
{
    float2 ndc = clipPos.xy / clipPos.w;
    return ndc * float2(0.5f, -0.5f) + 0.5f;
}


VS_OUTPUT VS(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    if (gIsLion == 1)
    {
        
        float offset = sin(gTime * 8.0f + input.pos.y * 0.7f) * 1.0f;
        input.pos += input.normal * offset;
    }
    
    float4x4 mWorld = CurrentInstances[instanceID].mWorld;
    float4x4 mTexTransform = CurrentInstances[instanceID].mTexTransform;
    float4x4 mPrevWorld = PreviousInstances[instanceID].mWorld;
    
    
    float3 localPos = input.pos;
    float3 localNormal = input.normal;

    float4 texC = mul(float4(input.uv, 0.0f, 1.0f), mTexTransform);
    float2 finalUV = mul(texC, mMatTransform).xy;

    float3 normalW = mul(localNormal, (float3x3) CurrentInstances[instanceID].mInvTWorld);
    normalW = normalize(normalW);
    
    output.Wpos = mul(float4(localPos, 1.0f), mWorld);
    output.pos = mul(output.Wpos, mView);
    output.pos = mul(output.pos, mProj);
    output.pos.xy += jitter * output.pos.w;
    output.currentClipPos = output.pos;
    
    float4 prevWordPos = mul(float4(localPos, 1.0f), mPrevWorld);
    output.prevClipPos = mul(prevWordPos, prevView);
    output.prevClipPos = mul(output.prevClipPos, prevProj);
    output.prevClipPos.xy += prevJitter * output.prevClipPos.w;
    
    output.normal = localNormal;
    output.normalW = normalW;
    output.tangentW = mul(input.tangent, (float3x3)CurrentInstances[instanceID].mInvTWorld);
    output.uv = finalUV;
    return output;
} 

struct gBufferOutput
{
    float4 Diffuse : SV_Target0;
    float4 Normal : SV_Target1;
    float2 Motion : SV_Target2;
};

gBufferOutput PS(VS_OUTPUT input)
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
    ret.Normal = mul(ret.Normal, mView);
    
    float2 currentUV = ClipToUV(input.currentClipPos);
    float2 previousUV = ClipToUV(input.prevClipPos);
    ret.Motion = previousUV - currentUV;
    
    return ret;
}