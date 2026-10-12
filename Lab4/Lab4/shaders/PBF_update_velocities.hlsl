#include "PBF_structures.hlsli"

RWStructuredBuffer<CSParticle> particles : register(u0);
StructuredBuffer<uint> neighbors : register(t0);

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

float3 SpikyGradient(float3 diff, float r)
{
    if (r >= smoothingRadius || r < 0.0001f)
        return float3(0.0f, 0.0f, 0.0f);
    
    float factor = spikyKernel * pow(smoothingRadius - r, 2);
    return factor * normalize(diff);
}

float Poly6Kernel(float rSq)
{
    if (rSq >= smoothingRadiusSq)
        return 0.0f;
    return poly6Kernel * pow(smoothingRadiusSq - rSq, 3);
}

float3 ComputeVorticity(uint i, CSParticle p)
{
    float3 vorticity = float3(0.0f, 0.0f, 0.0f);
    
    for (uint k = 0; k < p.neighborCount; k++)
    {
        uint j = neighbors[p.neighborOffset + k];
        CSParticle neighbor = particles[j];
        
        float3 diff = p.position - neighbor.position;
        float rSq = dot(diff, diff);
        float r = sqrt(rSq);
        
        float3 velDiff = neighbor.velocity - p.velocity;
        
        float3 gradW = SpikyGradient(diff, r);
        
        vorticity += cross(velDiff, gradW);
    }
    
    return vorticity;
}

float3 ComputeVorticityGradient(uint i, CSParticle p, float3 omega_i)
{
    float3 gradOmega = float3(0.0f, 0.0f, 0.0f);
    float omega_i_mag = length(omega_i);
    
    for (uint k = 0; k < p.neighborCount; k++)
    {
        uint j = neighbors[p.neighborOffset + k];
        CSParticle neighbor = particles[j];
        
        float3 omega_j = ComputeVorticity(j, neighbor);
        float omega_j_mag = length(omega_j);
        
        float3 diff = p.position - neighbor.position;
        float rSq = dot(diff, diff);
        float r = sqrt(rSq);
        
        float3 gradW = SpikyGradient(diff, r);
        
        float omegaDiff = omega_j_mag - omega_i_mag;
        
        gradOmega += omegaDiff * gradW;
    }
    
    return gradOmega;
}

[numthreads(256, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint i = DTid.x;
    if (i >= numParticles)
        return;
    
    CSParticle p = particles[i];
    
    float3 newVel = (p.predictedPos - p.position) / dt;
    float3 omega_i = ComputeVorticity(i, p);
    float3 gradOmega = ComputeVorticityGradient(i, p, omega_i);
    
    float3 N = float3(0.0f, 0.0f, 0.0f);
    float gradOmegaMag = length(gradOmega);
    if (gradOmegaMag > 1e-6f)
    {
        N = gradOmega / gradOmegaMag;
    }
    float3 vorticityForce = vorticityEpsilon * cross(N, omega_i);
    newVel += vorticityForce * dt;
    float3 viscosityCorrection = float3(0.0f, 0.0f, 0.0f);
    for (uint k = 0; k < p.neighborCount; k++)
    {
        uint j = neighbors[p.neighborOffset + k];
        CSParticle neighbor = particles[j];
        
        float3 diff = p.position - neighbor.position;
        float rSq = dot(diff, diff);
        
        float3 velDiff = neighbor.velocity - newVel;
        
        float w = Poly6Kernel(rSq);
        viscosityCorrection += velDiff * w;
    }
    newVel += xsphVescosityC * viscosityCorrection;
    
    p.velocity = newVel;
    p.position = p.predictedPos;
    
    particles[i] = p;
}