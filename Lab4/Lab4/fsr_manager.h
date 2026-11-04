#pragma once


#include <Windows.h>
#include <d3d12.h>
#include <ffx_api.h>

struct FSRFrameResources
{
	ID3D12GraphicsCommandList* commandList = nullptr;
	ID3D12Resource* color = nullptr;
	ID3D12Resource* depth = nullptr;
	ID3D12Resource* motionVectors = nullptr;
	ID3D12Resource* output = nullptr;

	float jitterX = 0.0f;
	float jitterY = 0.0f;
	float deltaTime = 16.67f;
	float nearPlane = 1.0f;
	float farPlane = 100000.0f;
	float verticalFov = 1.04719755f;
	bool reset = false;
};

enum class AntiAliasingMode
{
	None = 0,
	TAA,
	FSR_NativeAA
};


class FSRManager
{
public:
	bool Load();
	void Unload();

	bool IsLoaded() const;

	bool Initialize(UINT renderWidth, UINT renderHeight, UINT displayWidth, UINT displayHeight);
	bool GetJitterOffset(float& outX, float& outY);
	bool Dispatch(const FSRFrameResources& frame);

private:
	void Shutdown();

	HMODULE loader = nullptr;
	PfnFfxCreateContext createContext = nullptr;
	PfnFfxDestroyContext destroyContext = nullptr;
	PfnFfxDispatch dispatch = nullptr;
	PfnFfxQuery query = nullptr;
	PfnFfxConfigure configure = nullptr;

	ffxContext upscaleContext = nullptr;
	UINT renderWidth = 0;
	UINT renderHeight = 0;

	UINT displayWidth = 0;
	UINT displayHeight = 0;

	int32_t jitterPhaseCount = 0;
	int32_t jitterIndex = 0;
	float jitterX = 0.0f;
	float jitterY = 0.0f;
};
