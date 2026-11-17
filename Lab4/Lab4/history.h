#pragma once
#include <memory>
#include <wrl.h>
#include "d3d12.h"
#include "EJTexture.h"

using Microsoft::WRL::ComPtr;

struct EJDescriptorHeaps;

class History 
{
public:
	History(int screenWidth, int screenHeight, EJDescriptorHeaps* heaps);

	void OnResize(int screenWidt, int screenHeight);
	EJTexture& GetHistoryA();
	EJTexture& GetLightTexture();
	EJTexture& GetCurrentBlurredTexture();
	EJTexture& GetTempBlurredTexture();
	EJTexture& GetMotionTextureA();
	EJTexture& GetMotionTextureB();
	ID3D12DescriptorHeap* GetSrvHeap();
	ID3D12DescriptorHeap* GetSamplerHeap();
	ID3D12DescriptorHeap* GetRtvHeap();
	void LightBarrier(ComPtr<ID3D12GraphicsCommandList> commandList);
	void MotionBarrier(ComPtr<ID3D12GraphicsCommandList> commandList);
	void ClearTextures(ComPtr<ID3D12GraphicsCommandList> commandList);
private:
	EJTexture historyA;
	EJTexture lightTexture;
	EJTexture currentFrameBlurred;
	EJTexture tempFrameBlurred;
	EJTexture motionTextureA;
	EJTexture motionTextureB;
	ComPtr<ID3D12DescriptorHeap> srvHeap;
	EJDescriptorHeaps* heaps;
	ComPtr<ID3D12DescriptorHeap> samplerHeap;
	ComPtr<ID3D12Device> device = nullptr;

	void CreateDescriptorHeaps();
	void CreateTextures(int screenWidth, int screenHeight);
	void CreateSRV();
	void CreateRTV();
	void CreateUAV();
	void CreateSamplers();
	void ResetTextures();
	void DefaultBarriers(ComPtr<ID3D12GraphicsCommandList> commandList);
};