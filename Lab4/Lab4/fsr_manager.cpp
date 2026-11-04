#include "fsr_manager.h"
#include <ffx_api.h>
#include <ffx_api_dx12.h>
#include <ffx_api_loader.h>
#include <ffx_api_types.h>
#include <ffx_upscale.h>
#include "singletone_device.h"

bool FSRManager::IsLoaded() const
{
	return loader != nullptr &&
		   createContext != nullptr &&
		   destroyContext != nullptr && 
		   dispatch != nullptr &&
		   query != nullptr &&
		   configure != nullptr;
}

bool FSRManager::Load()
{
	loader = LoadLibraryW(L"FSR\\amd_fidelityfx_loader_dx12.dll");
	if (!loader)
	{
		return false;
	}

	createContext = reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(loader, "ffxCreateContext"));
	destroyContext = reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(loader, "ffxDestroyContext"));
	dispatch = reinterpret_cast<PfnFfxDispatch>(GetProcAddress(loader, "ffxDispatch"));
	query = reinterpret_cast<PfnFfxQuery>(GetProcAddress(loader, "ffxQuery"));
	configure = reinterpret_cast<PfnFfxConfigure>(GetProcAddress(loader, "ffxConfigure"));

	if (!IsLoaded())
	{
		Unload();
		return false;
	}

	return true;
}

void FSRManager::Unload()
{
	Shutdown();
	if (loader)
	{
		FreeLibrary(loader);
		loader = nullptr;
	}
	createContext = nullptr;
	destroyContext = nullptr;
	dispatch = nullptr;
	query = nullptr;
	configure = nullptr;
}

bool FSRManager::Initialize(UINT renderWidth, UINT renderHeight, UINT displayWidth, UINT displayHeight)
{
	this->renderWidth = renderWidth;
	this->renderHeight = renderHeight;
	this->displayWidth = displayWidth;
	this->displayHeight = displayHeight;
	
	ffxCreateBackendDX12Desc createBackend = { 0 };
	createBackend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
	createBackend.device = SingletonDevice::GetDevice().Get();
	
	ffxCreateContextDescUpscale createUpscale = { 0 };
	createUpscale.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
	createUpscale.header.pNext = &createBackend.header;
	createUpscale.flags = FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;
	createUpscale.fpMessage = nullptr;
	createUpscale.maxRenderSize = {renderWidth, renderHeight};
	createUpscale.maxUpscaleSize = {displayWidth, displayHeight};
	ffxReturnCode_t retCode = createContext(&upscaleContext, &createUpscale.header, nullptr);
	if (retCode != FFX_API_RETURN_OK)
	{
		OutputDebugStringW(L"[FSR] Failed to create upscale context");
		upscaleContext = nullptr;
		return false;
	}

	ffxQueryDescUpscaleGetJitterPhaseCount jitterQuery{};
	jitterQuery.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTERPHASECOUNT;
	jitterQuery.renderWidth = renderWidth;
	jitterQuery.displayWidth = displayWidth;
	jitterQuery.pOutPhaseCount = &jitterPhaseCount;
	ffxReturnCode_t result = query(&upscaleContext, &jitterQuery.header);
	if (result != FFX_API_RETURN_OK)
	{
		OutputDebugStringW(L"[FSR] Failed to create jitter phase count");
		return false;
	}
	jitterIndex = 0;
	return true;
}

bool FSRManager::GetJitterOffset(float& outX, float& outY)
{	
	ffxQueryDescUpscaleGetJitterOffset jitterQuery{};
	jitterQuery.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTEROFFSET;
	jitterQuery.index = jitterIndex;
	jitterQuery.phaseCount = jitterPhaseCount;
	jitterQuery.pOutX = &outX;
	jitterQuery.pOutY = &outY;
	ffxReturnCode_t result = query(&upscaleContext, &jitterQuery.header);
	if (result != FFX_API_RETURN_OK)
	{
		OutputDebugStringW(L"[FSR] Failed to get jitter offset");
		return false;
	}
	jitterIndex = (jitterIndex + 1) % jitterPhaseCount;

	return true;
}

void FSRManager::Shutdown()
{
	if (upscaleContext && destroyContext)
	{
		destroyContext(&upscaleContext, nullptr);
		upscaleContext = nullptr;
	}
}

bool FSRManager::Dispatch(const FSRFrameResources& frame)
{
	ffxDispatchDescUpscale params = {};
	params.header.type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
	params.commandList = frame.commandList;
	params.color = ffxApiGetResourceDX12(frame.color, FFX_API_RESOURCE_STATE_COMPUTE_READ);
	params.depth = ffxApiGetResourceDX12(frame.depth, FFX_API_RESOURCE_STATE_COMPUTE_READ);
	params.motionVectors = ffxApiGetResourceDX12(frame.motionVectors, FFX_API_RESOURCE_STATE_COMPUTE_READ);
	params.output = ffxApiGetResourceDX12(frame.output, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
	params.renderSize = {renderWidth, renderHeight};
	params.upscaleSize = {displayWidth, displayHeight};
	params.jitterOffset.x = frame.jitterX;
	params.jitterOffset.y = frame.jitterY;
	params.motionVectorScale.x = static_cast<float>(renderWidth);
	params.motionVectorScale.y = static_cast<float>(renderHeight);
	params.frameTimeDelta = frame.deltaTime;
	params.preExposure = 1.0f;
	params.reset = frame.reset;
	params.cameraNear = frame.nearPlane;
	params.cameraFar = frame.farPlane;
	params.cameraFovAngleVertical = frame.verticalFov;
	params.enableSharpening = false;
	params.sharpness = 0.0f;
	ffxReturnCode_t result = dispatch(&upscaleContext, &params.header);
	return result == FFX_API_RETURN_OK;
}
