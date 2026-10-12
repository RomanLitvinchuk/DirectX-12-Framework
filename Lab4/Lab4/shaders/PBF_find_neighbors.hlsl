#include "PBF_structures.hlsli"

StructuredBuffer<CSParticle> particles : register(t0);
StructuredBuffer<uint2> particleHashes : register(t1);
StructuredBuffer<uint> hashGridOffset : register(t2);

#define MAX_NEIGHBORS 64
RWStructuredBuffer<uint> neighborsBuffer : register(u0);
RWStructuredBuffer<CSParticle> particlesOut : register(u1);

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

int3 getGridCoord(float3 pos)
{
    return int3(pos / cellSize);
}


[numthreads(256, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint i = DTid.x;
    if (i >= numParticles)
        return;
    
    CSParticle p = particles[i];
    int3 myGrid = getGridCoord(p.predictedPos);
    
    uint neighborCount = 0;
    
    for (int z = -1; z <= 1; z++)
    {
        for (int y = -1; y <= 1; y++)
        {
            for (int x = -1; x <= 1; x++)
            {
                int3 cellCoord = myGrid + int3(x, y, z);
                uint cellHash = HashCoord(cellCoord);
                
                uint currentParticleIdx = hashGridOffset[cellHash];
                while (currentParticleIdx != 0xFFFFFFFF)
                {
                    uint j = currentParticleIdx;
                    if (j != i)
                    {
                        float3 neighborPos = particles[j].predictedPos;
                        float3 diff = p.predictedPos - neighborPos;
                        float distSq = dot(diff, diff);
                        
                        if (distSq < smoothingRadiusSq)
                        {
                            if (neighborCount < MAX_NEIGHBORS)
                            {
                                uint flatIndex = (i * MAX_NEIGHBORS) + neighborCount;
                                neighborsBuffer[flatIndex] = j;
                                
                                neighborCount++;
                            }
                        }
                    }
                    currentParticleIdx = particleHashes[j].y;
                }

            }

        }

    }
    p.neighborCount = neighborCount;
    p.neighborOffset = i * MAX_NEIGHBORS;
    
    particlesOut[i] = p;

}