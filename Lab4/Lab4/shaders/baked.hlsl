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
    int2 padding;
}

struct InstanceData
{
    row_major float4x4 mWorld;
    row_major float4x4 mTexTransform;
    row_major float4x4 mInvTWorld;
};
StructuredBuffer<InstanceData> Instances : register(t3);

struct BAKED_VS_INPUT
{
    float3 bakedPos : WORLDPOS;
    float3 localNormal : NORMAL0; 
    float3 normalW : NORMAL1;
    float2 uv : TEXCOORD0; 
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

VS_OUTPUT BakedVS(BAKED_VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    
    float4x4 mTexTransform = Instances[instanceID].mTexTransform;
    float4x4 mInvTWorld = Instances[instanceID].mInvTWorld;
    
    output.Wpos = float4(input.bakedPos, 1.0f);
    output.pos = mul(output.Wpos, mView);
    output.pos = mul(output.pos, mProj);
    
    //float3 normalW = mul(input.localNormal, (float3x3) mInvTWorld);
    //output.normalW = normalize(normalW);
    output.normal = input.localNormal;
    output.normalW = input.normalW;
    
    //output.tangentW = mul(input.tangent, (float3x3) mInvTWorld);
    output.tangentW = input.tangent;
    //float4 texC = mul(float4(input.uv, 0.0f, 1.0f), mTexTransform);
    //output.uv = mul(texC, mMatTransform).xy;
    output.uv = input.uv;
    output.prevClipPos = output.pos;
    output.currentClipPos = output.pos;
    
    return output;
}
