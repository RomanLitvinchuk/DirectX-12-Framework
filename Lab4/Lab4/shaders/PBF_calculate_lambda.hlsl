#include "PBF_structures.hlsli"

StructuredBuffer<CSParticle> particles : register(t0);
RWStructuredBuffer<CSParticle> particlesRW : register(u0);
StructuredBuffer<uint> neighbors : register(t1);

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

float Poly6Kernel(float rSq)
{
    if (rSq >= smoothingRadiusSq)
        return 0.0f;
    return poly6Kernel * pow(smoothingRadiusSq - rSq, 3);
}

float3 SpikyGradient(float3 diff, float r)
{
    if (r >= smoothingRadius || r < 0.0001f)
        return float3(0.0f, 0.0f, 0.0f);
    
    float factor = spikyKernel * pow(smoothingRadius - r, 2);
    return factor * normalize(diff);
}


[numthreads(256, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint i = DTid.x;
    if (i >= numParticles)
        return;
    
    CSParticle p = particles[i];
    float density = 0.0f;
    for (uint k = 0; k < p.neighborCount; k++)
    {
        uint j = neighbors[p.neighborOffset + k];
        CSParticle neighbor = particles[j];
        float3 diff = p.predictedPos - neighbor.predictedPos;
        float rSq = dot(diff, diff);
        density = Poly6Kernel(rSq);
    }
    
    p.density = density;
    float C = density / restDensity - 1.0f;
    float3 gradC = float3(0.0f, 0.0f, 0.0f);
    float sumGradSq = 0.0f;
    for (k = 0; k < p.neighborCount; k++)
    {
        uint j = neighbors[p.neighborOffset + k];
        CSParticle neighbor = particles[j];
        float3 diff = p.predictedPos - neighbor.predictedPos;
        float r = length(diff);
        
        float3 gradW = SpikyGradient(diff, r);
        gradC += gradW;
        sumGradSq += dot(gradW, gradW);
    }
    p.lambda = -C / (sumGradSq + epsilon);
    
    particlesRW[i] = p;
}