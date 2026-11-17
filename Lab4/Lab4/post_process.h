#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "EJTexture.h"

struct EJDescriptorHeaps;

using Microsoft::WRL::ComPtr;

class PostProcess {

public:
	void ClearPostProcess(ComPtr<ID3D12GraphicsCommandList> commandList);
	void OnResize(int width, int height);

	PostProcess(int width, int height, EJDescriptorHeaps* heaps);

	EJTexture& GetHdrTextureA();
	EJTexture& GetHdrTextureB();
	EJTexture& GetLdrTextureA();
	EJTexture& GetLdrTextureB();
	//MyTexture& GetFsrOutput();
	ComPtr<ID3D12DescriptorHeap> GetSrvHeap();
	ID3D12DescriptorHeap* GetRtvHeap();

private:
	EJTexture hdrTextureA;
	EJTexture hdrTextureB;
	EJTexture ldrTextureA;
	EJTexture ldrTextureB;
	//MyTexture fsrOutput;
	ComPtr<ID3D12DescriptorHeap> srvHeap;
	EJDescriptorHeaps* heaps = nullptr;

	ComPtr<ID3D12Device> device = nullptr;

	void CreateHeaps();
	void CreateTextures(int width, int height);
	void CreateSRV();
	void CreateRTV();
	void CreateUAV();
	void BarriersToDefault(ComPtr<ID3D12GraphicsCommandList> commandList);
	void ResetTextures();

};
