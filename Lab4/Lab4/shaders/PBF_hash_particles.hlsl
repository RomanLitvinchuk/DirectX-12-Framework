#include "PBF_structures.hlsli"

StructuredBuffer<CSParticle> particles : register(t0);
RWStructuredBuffer<uint> hashGrid : register(u0);
RWStructuredBuffer<uint> hashGridAtomics : register(u1);
RWStructuredBuffer<uint2> particleHashes : register(u2);

cbuffer PBFConstants : register(b0)
{
    float dt;
    float gravity;
    float restDensity;
    float gasConstant;

    float smoothingRadius;
    float smoothingRadiusSq;
    float poly6Kernel;
    float spikyKernel;

    float epsilon;
    float vorticityEpsilon;
    float xsphVescosityC;
    float deltaQ;

    float sCorrK;
    float sCorrN;
    float padding1[2];

    float boundaryMin[3];
    float padding2;

    float boundaryMax[3];
    float padding3;

    unsigned int numParticles;
    unsigned int solverIterations;
    unsigned int gridResolution;
    float cellSize;
}

uint HashCoord(int3 coord)
{
    return (coord.x * 73856093 ^ coord.y * 19349663 ^ coord.z * 83492791 % gridResolution);
}

int3 GetGridCoord(float3 pos)
{
    return int3(pos / cellSize);
}



[numthreads(256, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint i = DTid.x;
    if (i >= numParticles)
        return;
    
    float3 pos = particles[i].predictedPos;
    int3 gridCoord = GetGridCoord(pos);
    uint hash = HashCoord(gridCoord);
    
    uint offset = InterlockedAdd(hashGridAtomics[hash], 1);
    particleHashes[i] = uint2(hash, offset);
}