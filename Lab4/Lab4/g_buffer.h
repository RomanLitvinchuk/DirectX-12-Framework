#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "texture.h"

using Microsoft::WRL::ComPtr;

class GBuffer {
private:

	MyTexture diffuseTex;
	MyTexture normalTex;
	MyTexture depthTex;

	ComPtr<ID3D12DescriptorHeap> rtvDescpritorHeap;
	ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

	ComPtr<ID3D12Device> device = nullptr;

	void CreateHeaps();
	void CreateTextures(int width, int height);
	void CreateSRV();
	void CreateRTVandDSV();
	void ResetTextures();
public:

	GBuffer(int width, int height);

	void TransitToOpaqueRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList);
	void TransitToLightsRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList);

	void OnResize(int width, int height);
	void ClearGBuffer(ComPtr<ID3D12GraphicsCommandList> commandList);

	MyTexture& GetDiffuseTex();
	MyTexture& GetNormalTex();
	MyTexture& GetDepthTex();
	ComPtr<ID3D12DescriptorHeap> GetSrvHeap();
	ComPtr<ID3D12DescriptorHeap> GetRtvHeap();
	ComPtr<ID3D12DescriptorHeap> GetDsvHeap();
};
