#ifndef PBF_STRUCTURES_HLSLI_
#define PBF_STRUCTURES_HLSLI_

struct CSParticle
{
    float3 position;
    float density;
    float3 velocity;
    float lambda;
    float3 predictedPos;
    uint neighborOffset;
    uint neighborCount;
};


#endif //PBF_STRUCTURES_HLSLI_