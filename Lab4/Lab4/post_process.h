#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "texture.h"


using namespace Microsoft::WRL;

class PostProcess {

private:
	MyTexture hdrTextureA;
	MyTexture hdrTextureB;
	MyTexture ldrTextureA;
	MyTexture ldrTextureB;
	ComPtr<ID3D12DescriptorHeap> srvHeap;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;

	ComPtr<ID3D12Device> device = nullptr;

	void CreateHeaps();
	void CreateTextures(int width, int height);
	void CreateSRV();
	void CreateRTV();
	void BarriersToDefault(ComPtr<ID3D12GraphicsCommandList> commandList);
	void ResetTextures();
public:
	void ClearPostProcess(ComPtr<ID3D12GraphicsCommandList> commandList);
	void OnResize(int width, int height);

	PostProcess(int width, int height);

	MyTexture& GetHdrTextureA();
	MyTexture& GetHdrTextureB();
	MyTexture& GetLdrTextureA();
	MyTexture& GetLdrTextureB();
	ComPtr<ID3D12DescriptorHeap> GetSrvHeap();
	ComPtr<ID3D12DescriptorHeap> GetRtvHeap();

};
