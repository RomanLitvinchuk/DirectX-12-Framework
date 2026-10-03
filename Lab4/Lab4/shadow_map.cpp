#include "shadow_map.h"
#include "throw_if_failed.h"
#include "singletone_device.h"

ShadowMap::ShadowMap(UINT initWidth, UINT initHeight)
{
    device = SingletonDevice::GetDevice().Get();
    width = initWidth;
    height = initHeight;
    viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
    scissorRect = { 0, 0, (int)width, (int)height };
    BuildResource();
}

void ShadowMap::BuildResource()
{
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.DepthOrArraySize = NUM_CASCADES;
    texDesc.MipLevels = 1;
    texDesc.Format = format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    
    auto heapDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &heapDefault,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&shadowMap)));
    shadowMap.Get()->SetName(L"Shadow Map Texture");
}

ID3D12Resource* ShadowMap::Resource()
{
    return shadowMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::Srv()const
{
    return srvGpuHandle;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::Dsv(int index)const
{
    return dsvCpuHandle[index];
}

D3D12_VIEWPORT ShadowMap::Viewport() const
{
    return viewport;
}

D3D12_RECT ShadowMap::ScissorRect() const
{
    return scissorRect;
}

void ShadowMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
    srvCpuHandle = hCpuSrv;
    srvGpuHandle = hGpuSrv;
    UINT dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    for (int i = 0; i < NUM_CASCADES; ++i) {
        dsvCpuHandle[i] = hCpuDsv;
        hCpuDsv.Offset(1, dsvDescriptorSize);
    }

    BuildDescriptors();
}

void ShadowMap::BuildDescriptors() {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.ArraySize = NUM_CASCADES;
    srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2DArray.PlaneSlice = 0;

    device->CreateShaderResourceView(shadowMap.Get(), &srvDesc, srvCpuHandle);


    for (int i = 0; i < NUM_CASCADES; i++) {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.FirstArraySlice = i;
        dsvDesc.Texture2DArray.ArraySize = 1;
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.Texture2DArray.MipSlice = 0;

        device->CreateDepthStencilView(shadowMap.Get(), &dsvDesc, dsvCpuHandle[i]);
    }
}

void ShadowMap::OnResize(UINT newWidth, UINT newHeight)
{
    if ((width != newWidth) || (height != newHeight))
    {
        width = newWidth;
        height = newHeight;

        viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
        scissorRect = { 0, 0, (int)width, (int)height };

        BuildResource();

        BuildDescriptors();
    }
}

UINT ShadowMap::Width() const {
    return width;
}

UINT ShadowMap::Height() const {
    return height;
}

int ShadowMap::GetNumCascades() const 
{ 
    return NUM_CASCADES; 
}