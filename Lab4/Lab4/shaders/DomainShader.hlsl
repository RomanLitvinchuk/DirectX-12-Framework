struct DS_OUTPUT
{
    //float4 pos : SV_POSITION;
    float3 worldPos : WORLDPOS;
    float3 normal : NORMAL0;
    float3 normalW : NORMAL1;
    float2 uv : TEXCOORD0;
    float3 tangentW : TANGENT;
};

struct HS_CONTROL_POINT_OUTPUT
{
    float4 pos : POSITION;
    float4 Wpos : WORLDPOS;
    float3 normal : NORMAL0;
    float3 normalW : NORMAL1;
    float2 uv : TEXCOORD0;
    float3 tangentW : TANGENT;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
};

Texture2D DiffuseMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D DisplacementMap : register(t2);

SamplerState Sampler : register(s0);

//cbuffer cbDisplacement : register(b2)
//{
    //float gDisplacementScale;
    //float gDisplacementBias;
//};

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

[domain("tri")]
DS_OUTPUT DS(
    HS_CONSTANT_DATA_OUTPUT input,
    float3 bary : SV_DomainLocation,
    const OutputPatch<HS_CONTROL_POINT_OUTPUT, 3> patch)
{
    DS_OUTPUT output;

    float3 worldPos =
        bary.x * patch[0].Wpos.xyz +
        bary.y * patch[1].Wpos.xyz +
        bary.z * patch[2].Wpos.xyz;

    float3 normalW =
        bary.x * patch[0].normalW +
        bary.y * patch[1].normalW +
        bary.z * patch[2].normalW;

    float3 tangentW =
        bary.x * patch[0].tangentW +
        bary.y * patch[1].tangentW +
        bary.z * patch[2].tangentW;

    float2 uv =
        bary.x * patch[0].uv +
        bary.y * patch[1].uv +
        bary.z * patch[2].uv;
    
    float3 normal =
        bary.x * patch[0].normal +
        bary.y * patch[1].normal +
        bary.z * patch[2].normal;
    
        

    normalW = normalize(normalW);
    tangentW = normalize(tangentW);

    if (gHasDisplacementTexture == 1)
    {
        float displacement = DisplacementMap.SampleLevel(Sampler, uv, 0).r;
        float displacementScale = 10.0;
        float displacementBias = -0.05;
        displacement = displacement * displacementScale + displacementBias;

        worldPos += normalW * displacement;
    }

    //output.pos = mul(float4(worldPos, 1.0f), mViewProj);

    output.normal = normal;
    output.normalW = normalW;
    output.tangentW = tangentW;
    output.uv = uv;
    output.worldPos = worldPos;

    return output;
}