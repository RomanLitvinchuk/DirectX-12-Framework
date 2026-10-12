
#include "PBF_structures.hlsli"

StructuredBuffer<CSParticle> particlesIn : register(t0);
RWStructuredBuffer<CSParticle> particlesOut : register(u0);

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


[numthreads(256, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint i = DTid.x;
    if (i >= numParticles)
        return;
    
    CSParticle p = particlesIn[i];
    float3 acceleration = float3(0, gravity, 0);
    p.velocity += acceleration * dt;
    p.predictedPos = p.position + p.velocity * dt;
    
    particlesOut[i] = p;
}