#pragma once
#include <d3d12.h>
#include <wrl.h>

using namespace Microsoft::WRL;

struct GBufferTexture {
	ComPtr<ID3D12Resource> Resource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
};


class GBuffer {
private:

	GBufferTexture diffuseTex;
	GBufferTexture normalTex;
	GBufferTexture depthTex;

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

	GBufferTexture& GetDiffuseTex();
	GBufferTexture& GetNormalTex();
	GBufferTexture& GetDepthTex();
	ComPtr<ID3D12DescriptorHeap> GetSrvHeap();
	ComPtr<ID3D12DescriptorHeap> GetRtvHeap();
	ComPtr<ID3D12DescriptorHeap> GetDsvHeap();
};
