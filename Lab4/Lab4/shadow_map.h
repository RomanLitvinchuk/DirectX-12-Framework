#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <d3dx12.h>
#include <SimpleMath.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX::SimpleMath;

class ShadowMap
{
public:
    ShadowMap(UINT initWidth, UINT initHeight);
    ShadowMap(const ShadowMap& rhs) = delete;
    ShadowMap& operator=(const ShadowMap& rhs) = delete;

    UINT Width()const;
    UINT Height()const;
    ID3D12Resource* Resource();
    CD3DX12_GPU_DESCRIPTOR_HANDLE Srv()const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE Dsv(int index)const;

    D3D12_VIEWPORT Viewport()const;
    D3D12_RECT ScissorRect()const;

    void BuildDescriptors(
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
        CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
        CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);

    void OnResize(UINT newWidth, UINT newHeight);
    int GetNumCascades() const;
private:
    void BuildDescriptors();
    void BuildResource();

private:
    const int NUM_CASCADES = 3;
    ComPtr<ID3D12Device> device = nullptr;
    D3D12_VIEWPORT viewport;
    D3D12_RECT scissorRect;
    UINT width = 0;
    UINT height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_R24G8_TYPELESS;
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvCpuHandle[3];
    ComPtr<ID3D12Resource> shadowMap = nullptr;
};

struct ShadowConstants {
    Matrix lightViewProj;
    Matrix shadowTransform;
    Vector4 cascadeDistances;
};

struct CascadeData {
    Matrix viewProjMats[3];
    Matrix shadowTransform[3];
    float distances[3];
};

