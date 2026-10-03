#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "texture.h"

using Microsoft::WRL::ComPtr;

class SSAO {
private:
	MyTexture SSAOTexture_A;
	MyTexture SSAOTexture_B;
	ComPtr<ID3D12DescriptorHeap> srvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> samplerHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> rtvHeap = nullptr;

	ComPtr<ID3D12Device> device = nullptr;

	void CreateHeaps();
	void CreateTexture(int width, int height);
	void CreateRTV();
	void CreateSRV(ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture);
	void CreateSamplers();

	void BarriersToDefault(ComPtr<ID3D12GraphicsCommandList> commandList);
	void ResetTextures();
public:
	SSAO(int width, int height, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture);
	void OnResize(int width, int height, ID3D12Resource* depthTexture, ID3D12Resource* normalTexture, ID3D12Resource* noiseTexture);
	void ClearSSAO(ComPtr<ID3D12GraphicsCommandList> commandList);

	MyTexture& GetTextureA();
	MyTexture& GetTextureB();
	ComPtr<ID3D12DescriptorHeap> GetSrvHeap();
	ComPtr<ID3D12DescriptorHeap> GetSamplerHeap();
	ComPtr<ID3D12DescriptorHeap> GetRtvHeap();
};

struct SsaoConstants
{
	float screenWidth;
	float screenHeight;
	float randomTextureSize;
	float sampleRadius;
	float ssaoScale;
	float ssaoBias;
	float ssaoIntensity;
	float padding;
};

struct BlurConstants {
	float screenWidth;
	float screenHeight;
	float blurType; //0.0f - horizontal blur, else - vertical blur
	float padding;
};
