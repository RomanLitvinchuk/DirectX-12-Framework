#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "light.h"
#include <SimpleMath.h>
#include <memory>
#include "d3dx12.h"

class GBuffer;
class PostProcess;
class SSAO;
class History;

using Microsoft::WRL::ComPtr;
using namespace DirectX::SimpleMath;

class RenderingSystem {
public:
	ComPtr<ID3D12RootSignature> opaqueRS = nullptr;
	ComPtr<ID3D12PipelineState> opaquePSO = nullptr;

	ComPtr<ID3D12RootSignature> lightRS = nullptr;
	ComPtr<ID3D12PipelineState> lightPSO = nullptr;

	ComPtr<ID3D12RootSignature> bulbRS = nullptr;
	ComPtr<ID3D12PipelineState> bulbPSO = nullptr;

	ComPtr<ID3D12RootSignature> streamOutputRS = nullptr;
	ComPtr<ID3D12PipelineState> streamOutputPSO = nullptr;

	ComPtr<ID3D12PipelineState> bakedPSO = nullptr;

	ComPtr<ID3D12RootSignature> wireframeRS;
	ComPtr<ID3D12PipelineState> wireframePSO;

	ComPtr<ID3D12RootSignature> particleRS = nullptr;
	ComPtr<ID3D12PipelineState> particlePSO = nullptr;

	ComPtr<ID3D12RootSignature> particlesUpdateRS = nullptr;
	ComPtr<ID3D12PipelineState> particlesUpdatePSO = nullptr;

	ComPtr<ID3D12RootSignature> particlesEmitRS = nullptr;
	ComPtr<ID3D12PipelineState> particlesEmitPSO = nullptr;

	ComPtr<ID3D12RootSignature> shadowRS = nullptr;
	ComPtr<ID3D12PipelineState> shadowPSO = nullptr;

	ComPtr<ID3D12RootSignature> SsaoRS = nullptr;
	ComPtr<ID3D12PipelineState> SsaoPSO = nullptr;

	ComPtr<ID3D12RootSignature> SsaoBlurRS = nullptr;
	ComPtr<ID3D12PipelineState> SsaoBlurPSO = nullptr;

	ComPtr<ID3D12RootSignature> billboardRS = nullptr;
	ComPtr<ID3D12PipelineState> billboardPSO = nullptr;

	ComPtr<ID3D12RootSignature> TAAResolveRS = nullptr;
	ComPtr<ID3D12PipelineState> TAAResolvePSO = nullptr;

	ComPtr<ID3D12RootSignature> ppDefaultRS = nullptr;
	ComPtr<ID3D12PipelineState> ppTonemappingPSO = nullptr;
	ComPtr<ID3D12PipelineState> ppVignettePSO = nullptr;
	ComPtr<ID3D12PipelineState> ppOutputPSO = nullptr;


	std::unique_ptr<GBuffer> gBuffer = nullptr;
	std::unique_ptr<PostProcess> postProcess = nullptr;
	std::unique_ptr<SSAO> ssao = nullptr;
	std::unique_ptr<History> history = nullptr;

	bool bVignette = true;

	std::vector<LightConstants> sceneLights;
	ComPtr<ID3D12DescriptorHeap> samplerHeap = nullptr;

	RenderingSystem(int width, int height, ID3D12Resource* noiseTexture);

private:
	ComPtr<ID3D12Device> device = nullptr;

	void BuildLayouts();
	void CompileShaders();

	void CreateOpaqueRS();
	void CreateOpaquePSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateLightRS();
	void CreateLightPSO();

	void CreateBulbRS();
	void CreateBulbPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateStreamOutputRS();
	void CreateStreamOutputPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);
	void CreateBakedPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateWireframeRS();
	void CreateWireframePSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateParticleRS();
	void CreateParticlePSO();

	void CreateParticlesUpdateRS();
	void CreateParticlesUpdatePSO();

	void CreateParticlesEmitRS();
	void CreateParticlesEmitPSO();

	void CreateShadowRS();
	void CreateShadowPSO(std::vector<D3D12_INPUT_ELEMENT_DESC>& layout);

	void CreateSSAORS();
	void CreateSSAOPSO();

	void CreateSSAOBlurRS();
	void CreateSSAOBlurPSO();

	void CreateBillboardRS();
	void CreateBillboardPSO();

	void CreateTAAResolveRS();
	void CreateTAAResolvePSO();

	void CreatePPDefaultRS();
	void CreatePPTonemappingPSO();

	void CreatePPVignettePSO();

	void CreatePPOutputPSO();

	void GenerateTreeLights(std::vector<LightConstants>& lightsArray, Vector3 treeBasePosition, float treeHeight, float treeBaseRadius, int count);

	ComPtr<ID3D12RootSignature> CreateRootSignature(const CD3DX12_ROOT_SIGNATURE_DESC& rootDesc);

	ComPtr<ID3DBlob> opaqueVS = nullptr;
	ComPtr<ID3DBlob> opaquePS = nullptr;

	ComPtr<ID3DBlob> fullscreenTriangleVS = nullptr;
	ComPtr<ID3DBlob> lightPS = nullptr;

	ComPtr<ID3DBlob> bulbVS = nullptr;
	ComPtr<ID3DBlob> bulbPS = nullptr;

	ComPtr<ID3DBlob> HullShader = nullptr;
	ComPtr<ID3DBlob> DomainShader = nullptr;

	ComPtr<ID3DBlob> tessVS = nullptr;

	ComPtr<ID3DBlob> bakedVS = nullptr;

	ComPtr<ID3DBlob> wireframeVS = nullptr;
	ComPtr<ID3DBlob> wireframePS = nullptr;

	ComPtr<ID3DBlob> particleVS = nullptr;
	ComPtr<ID3DBlob> particleGS = nullptr;
	ComPtr<ID3DBlob> particlePS = nullptr;
	ComPtr<ID3DBlob> particleUpdateCS = nullptr;
	ComPtr<ID3DBlob> particleEmitCS = nullptr;

	ComPtr<ID3DBlob> shadowVS = nullptr;

	ComPtr<ID3DBlob> SsaoPS = nullptr;
	ComPtr<ID3DBlob> SsaoBlurPS = nullptr;

	ComPtr<ID3DBlob> billboardVS = nullptr;
	ComPtr<ID3DBlob> billboardPS = nullptr;

	ComPtr<ID3DBlob> TAAResolvePS = nullptr;

	ComPtr<ID3DBlob> ppTonemappingPS = nullptr;
	ComPtr<ID3DBlob> ppVignettePS = nullptr;
	ComPtr<ID3DBlob> ppOutputRS = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> bakedLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> wireframeLayout;
};

