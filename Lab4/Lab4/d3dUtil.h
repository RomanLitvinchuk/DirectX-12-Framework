#ifndef D3DUTIL_H_
#define D3DUTIL_H_
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <SimpleMath.h>
#include <d3dx12.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::SimpleMath;

struct EJTexture;

class d3dUtil {
public:
	static ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* CmdList,
		const void* initData, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer);
	static UINT CalcConstantBufferSize(UINT byteSize);
	static ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);
	static DXGI_FORMAT MakeSRGB(DXGI_FORMAT format);
	static bool SingleBarrier(EJTexture* texture, D3D12_RESOURCE_STATES newState, CD3DX12_RESOURCE_BARRIER& outBarrier);
	static void AddBarrier(EJTexture* texture, D3D12_RESOURCE_STATES newState, std::vector<CD3DX12_RESOURCE_BARRIER>& vectorBarriers);
	static void SwapTextures(ComPtr<ID3D12GraphicsCommandList> commandList, EJTexture*& writeTexture, EJTexture*& readTexture);
private:
};


#endif //D3DUTIL_H_
