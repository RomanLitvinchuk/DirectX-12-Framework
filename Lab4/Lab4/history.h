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
	MyTexture& GetHistoryB();
private:
	MyTexture historyA;
	MyTexture historyB;
	ComPtr<ID3D12DescriptorHeap> srvHeap;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12Device> device = nullptr;

	void CreateDescriptorHeaps();
	void CreateTextures(int screenWidth, int screenHeight);
	void CreateSRV();
	void CreateRTV();
	void ResetTextures();
};