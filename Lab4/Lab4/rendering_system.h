#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "light.h"
#include <SimpleMath.h>
#include <memory>

class GBuffer;
class PostProcess;
class SSAO;

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;

class RenderingSystem {
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

	void CreatePPDefaultRS();
	void CreatePPTonemappingPSO();

	void CreatePPVignettePSO();

	void CreatePPOutputPSO();

	void GenerateTreeLights(std::vector<LightConstants>& lightsArray, Vector3 treeBasePosition, float treeHeight, float treeBaseRadius, int count);

public:
	ComPtr<ID3D12RootSignature> opaqueRS_ = nullptr;
	ComPtr<ID3D12RootSignature> lightRS_ = nullptr;

	ComPtr<ID3D12PipelineState> opaquePSO_ = nullptr;
	ComPtr<ID3D12PipelineState> lightPSO_ = nullptr;

	ComPtr<ID3DBlob> opaqueVS_ = nullptr;
	ComPtr<ID3DBlob> opaquePS_ = nullptr;

	ComPtr<ID3DBlob> fullscreenTriangleVS_ = nullptr;
	ComPtr<ID3DBlob> lightPS_ = nullptr;

	ComPtr<ID3D12RootSignature> bulbRS_ = nullptr;
	ComPtr<ID3D12PipelineState> bulbPSO_ = nullptr;
	ComPtr<ID3DBlob> bulbVS_ = nullptr;
	ComPtr<ID3DBlob> bulbPS_ = nullptr;

	ComPtr<ID3DBlob> HS_ = nullptr;
	ComPtr<ID3DBlob> DS_ = nullptr;

	ComPtr<ID3DBlob> tessVS_ = nullptr;

	ComPtr<ID3D12RootSignature> streamOutputRS_ = nullptr;
	ComPtr<ID3D12PipelineState> streamOutputPSO_ = nullptr;

	ComPtr<ID3D12PipelineState> bakedPSO_ = nullptr;
	ComPtr<ID3DBlob> bakedVS_ = nullptr;

	ComPtr<ID3D12RootSignature> wireframeRS_;
	ComPtr<ID3D12PipelineState> wireframePSO_;
	ComPtr<ID3DBlob> wireframeVS_ = nullptr;
	ComPtr<ID3DBlob> wireframePS_ = nullptr;

	ComPtr<ID3D12RootSignature> particleRS_ = nullptr;
	ComPtr<ID3D12PipelineState> particlePSO_ = nullptr;
	ComPtr<ID3DBlob> particleVS_ = nullptr;
	ComPtr<ID3DBlob> particleGS_ = nullptr;
	ComPtr<ID3DBlob> particlePS_ = nullptr;

	ComPtr<ID3D12RootSignature> particlesUpdateRS_ = nullptr;
	ComPtr<ID3D12PipelineState> particlesUpdatePSO_ = nullptr;
	ComPtr<ID3DBlob> particleUpdateCS_ = nullptr;

	ComPtr<ID3D12RootSignature> particlesEmitRS_ = nullptr;
	ComPtr<ID3D12PipelineState> particlesEmitPSO_ = nullptr;
	ComPtr<ID3DBlob> particleEmitCS_ = nullptr;

	ComPtr<ID3D12RootSignature> shadowRS_ = nullptr;
	ComPtr<ID3D12PipelineState> shadowPSO_ = nullptr;
	ComPtr<ID3DBlob> shadowVS_ = nullptr;

	ComPtr<ID3D12RootSignature> SsaoRS_ = nullptr;
	ComPtr<ID3D12PipelineState> SsaoPSO_ = nullptr;
	ComPtr<ID3DBlob> SsaoPS_ = nullptr;

	ComPtr<ID3D12RootSignature> SsaoBlurRS_ = nullptr;
	ComPtr<ID3D12PipelineState> SsaoBlurPSO_ = nullptr;
	ComPtr<ID3DBlob> SsaoBlurPS_ = nullptr;

	ComPtr<ID3D12RootSignature> billboardRS_ = nullptr;
	ComPtr<ID3D12PipelineState> billboardPSO_ = nullptr;
	ComPtr<ID3DBlob> billboardVS_ = nullptr;
	ComPtr<ID3DBlob> billboardPS_ = nullptr;

	ComPtr<ID3D12RootSignature> pp_defaultRS_ = nullptr;
	ComPtr<ID3D12PipelineState> pp_tonemappingPSO_ = nullptr;
	ComPtr<ID3DBlob> pp_tonemappingPS_ = nullptr;

	ComPtr<ID3D12PipelineState> pp_vignettePSO_ = nullptr;
	ComPtr<ID3DBlob> pp_vignettePS_ = nullptr;

	ComPtr<ID3D12PipelineState> pp_outputPSO_ = nullptr;
	ComPtr<ID3DBlob> pp_outputPS_ = nullptr;


	std::unique_ptr<GBuffer> g_buffer = nullptr;
	std::unique_ptr<PostProcess> post_process = nullptr;
	std::unique_ptr<SSAO> ssao = nullptr;

	std::vector<LightConstants> sceneLights_;
	ComPtr<ID3D12DescriptorHeap> samplerHeap = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout_;
	std::vector<D3D12_INPUT_ELEMENT_DESC> bakedLayout_;
	std::vector<D3D12_INPUT_ELEMENT_DESC> wireframeLayout_;

	RenderingSystem(int width, int height, ID3D12Resource* noiseTexture);
};
