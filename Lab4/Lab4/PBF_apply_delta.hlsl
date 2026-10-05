#include "PBF_structures.hlsli"

RWStructuredBuffer<CSParticle> particles : register(u0);
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

[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint i = dispatchThreadID.x;
    if (i >= numParticles)
        return;
    
    CSParticle p = particles[i];
    
    float3 deltaP = float3(0.0f, 0.0f, 0.0f);
    
    for (uint k = 0; k < p.neighborCount; k++)
    {
        uint j = neighbors[p.neighborOffset + k];
        CSParticle neighbor = particles[j];
        
        float3 diff = p.predictedPos - neighbor.predictedPos;
        float rSq = dot(diff, diff);
        float r = sqrt(rSq);
        
        float3 gradW = SpikyGradient(diff, r);
        
        float w_r = Poly6Kernel(rSq);
        
        float w_dq = Poly6Kernel(deltaQ * deltaQ);
        
        float sCorr = 0.0f;
        if (w_dq > 1e-6f) 
        {
            sCorr = -sCorrK * pow(w_r / w_dq, sCorrN);
        }
        
        float scalar = (p.lambda + neighbor.lambda + sCorr) / restDensity;
        
        deltaP += scalar * gradW;
    }
    
    p.predictedPos += deltaP;
    
    
    float3 normal = float3(0.0f, 0.0f, 0.0f);
    bool collided = false;

    if (p.predictedPos.x < boundaryMin[0])
    {
        p.predictedPos.x = boundaryMin[0];
        normal.x = 1.0f;
        collided = true;
    }
    else if (p.predictedPos.x > boundaryMax[0])
    {
        p.predictedPos.x = boundaryMax[0];
        normal.x = -1.0f;
        collided = true;
    }

    if (p.predictedPos.y < boundaryMin[1])
    {
        p.predictedPos.y = boundaryMin[1];
        normal.y = 1.0f;
        collided = true;
    }
    else if (p.predictedPos.y > boundaryMax[1])
    {
        p.predictedPos.y = boundaryMax[1];
        normal.y = -1.0f;
        collided = true;
    }

    if (p.predictedPos.z < boundaryMin[2])
    {
        p.predictedPos.z = boundaryMin[2];
        normal.z = 1.0f;
        collided = true;
    }
    else if (p.predictedPos.z > boundaryMax[2])
    {
        p.predictedPos.z = boundaryMax[2];
        normal.z = -1.0f;
        collided = true;
    }

    if (collided)
    {
        float restitution = 0.5f; 
        float vDotN = dot(p.velocity, normal);
        
        if (vDotN < 0.0f)
        {
            p.velocity -= (1.0f + restitution) * vDotN * normal;
        }
    }
    
    particles[i] = p;
}