#include "d3dUtil.h"
#include <d3dx12.h>
#include "throw_if_failed.h"
#include "vertex.h"
#include <DirectXColors.h>
#include <iostream>
#include <d3dcompiler.h>

ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* CmdList,
	const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer) 
{
	ComPtr<ID3D12Resource> defaultBuffer;
	CD3DX12_HEAP_PROPERTIES heapPr = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC bs = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	ThrowIfFailed(device->CreateCommittedResource(&heapPr, D3D12_HEAP_FLAG_NONE, &bs,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(defaultBuffer.GetAddressOf())));
	heapPr = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(device->CreateCommittedResource(&heapPr, D3D12_HEAP_FLAG_NONE, &bs,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	CmdList->ResourceBarrier(1, &barrier);
	UpdateSubresources<1>(CmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	CmdList->ResourceBarrier(1, &barrier);
	return defaultBuffer;
}

UINT d3dUtil::CalcConstantBufferSize(UINT byteSize) {
	return (byteSize + 255) & ~255;
}

ComPtr<ID3DBlob> d3dUtil::CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target) {
	UINT compileFlags = 0;
#if defined(DEBUG)||defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	HRESULT hr = S_OK;
	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags,
		0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);
	return byteCode;
}

DXGI_FORMAT d3dUtil::MakeSRGB(DXGI_FORMAT format)
{
	switch (format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	case DXGI_FORMAT_BC1_UNORM:
		return DXGI_FORMAT_BC1_UNORM_SRGB;
	case DXGI_FORMAT_BC2_UNORM:
		return DXGI_FORMAT_BC2_UNORM_SRGB;
	case DXGI_FORMAT_BC3_UNORM:
		return DXGI_FORMAT_BC3_UNORM_SRGB;
	case DXGI_FORMAT_BC7_UNORM:
		return DXGI_FORMAT_BC7_UNORM_SRGB;
	default:
		return format;
	}
}

bool d3dUtil::Barrier(MyTexture* texture, D3D12_RESOURCE_STATES newState, CD3DX12_RESOURCE_BARRIER& outBarrier)
{
	if (texture->currentState == newState)
	{
		return false;
	}

	outBarrier = CD3DX12_RESOURCE_BARRIER::Transition(texture->Resource.Get(), texture->currentState, newState);
	texture->currentState = newState;
	return true;
}

void d3dUtil::SwapTextures(ComPtr<ID3D12GraphicsCommandList> commandList, MyTexture*& writeTexture, MyTexture*& readTexture)
{
	std::swap(writeTexture, readTexture);
	std::vector<CD3DX12_RESOURCE_BARRIER> barriers;
	barriers.reserve(2);
	CD3DX12_RESOURCE_BARRIER tempBarrier;
	if (d3dUtil::Barrier(writeTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, tempBarrier))
	{
		barriers.push_back(tempBarrier);
	}
	if (!barriers.empty())
	{
		commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
	}
}


