#ifndef PBF_CONSTANTS_
#define PBF_CONSTANTS_

struct PBFConstants {
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
};


#endif //PBF_CONSTANTS_
