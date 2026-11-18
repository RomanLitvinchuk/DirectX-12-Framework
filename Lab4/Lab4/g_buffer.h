#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "EJTexture.h"

using Microsoft::WRL::ComPtr;

struct EJDescriptorHeaps;

class GBuffer {
private:

	EJTexture diffuseTex;
	EJTexture normalTex;
	EJTexture depthTex;

	EJDescriptorHeaps* heaps;
	ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;

	ComPtr<ID3D12Device> device = nullptr;

	void CreateHeaps();
	void CreateTextures(int width, int height);
	void CreateSRV();
	void CreateRTVandDSV();
	void ResetTextures();
public:

	GBuffer(int width, int height, EJDescriptorHeaps* heaps);

	void TransitToOpaqueRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList);
	void TransitToLightsRenderingState(ComPtr<ID3D12GraphicsCommandList> commandList);

	void OnResize(int width, int height);
	void ClearGBuffer(ComPtr<ID3D12GraphicsCommandList> commandList);

	EJTexture& GetDiffuseTex();
	EJTexture& GetNormalTex();
	EJTexture& GetDepthTex();
	ComPtr<ID3D12DescriptorHeap> GetSrvHeap();
	ID3D12DescriptorHeap* GetRtvHeap();
	ID3D12DescriptorHeap* GetDsvHeap();
};
