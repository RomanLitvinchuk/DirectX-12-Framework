#pragma once
#include <memory>
#include <wrl.h>
#include "d3d12.h"
#include "texture.h"

using Microsoft::WRL::ComPtr;

struct MyTexture;

class History 
{
public:
	History(int screenWidth, int screenHeight);

	void OnResize(int screenWidt, int screenHeight);
	MyTexture& GetHistoryA();
	MyTexture& GetLightTexture();
	MyTexture& GetCurrentBlurredTexture();
	MyTexture& GetTempBlurredTexture();
	MyTexture& GetMotionTextureA();
	MyTexture& GetMotionTextureB();
	ID3D12DescriptorHeap* GetSrvHeap();
	ID3D12DescriptorHeap* GetSamplerHeap();
	void LightBarrier(ComPtr<ID3D12GraphicsCommandList> commandList);
	void MotionBarrier(ComPtr<ID3D12GraphicsCommandList> commandList);
	void ClearTextures(ComPtr<ID3D12GraphicsCommandList> commandList);
private:
	MyTexture historyA;
	MyTexture lightTexture;
	MyTexture currentFrameBlurred;
	MyTexture tempFrameBlurred;
	MyTexture motionTextureA;
	MyTexture motionTextureB;
	ComPtr<ID3D12DescriptorHeap> srvHeap;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
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