#ifndef DX12APP_
#define DX12APP_

#include <Windows.h>
#include <d3d12.h>
#include <DirectXHelpers.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <comdef.h>
#include <DescriptorHeap.h>
#include <d3dx12.h>
#include "throw_if_failed.h"
#include "game_timer.h"
#include "upload_buffer.h"
#include "object_constants.h"
#include "g_buffer.h"
#include "rendering_system.h"
#include "light.h"
#include <unordered_map>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "camera.h"
#include <DirectXCollision.h>
#include "octree.h"
#include "particle.h"
#include "shadow_map.h"
#include "ssao.h"
#include "singletone_device.h"
#include "scene_data.h"

using namespace Microsoft::WRL;
using namespace DirectX;

class DX12App
{
public:
	void InitializeDevice();
	void InitializeCommandObjects();
	void CreateSwapChain(HWND hWnd);
	void CreateRTVAndDSVDescriptorHeaps();
	void CreateCBVDescriptorHeap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetBackBuffer() const;
	ID3D12Resource* CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;
	void CreateRTV();
	void CreateDSV();
	void LoadTextures();
	void CreateSRV();
	void CreateSamplerHeap();

	void OnResize();
	void SetViewport();
	void SetScissor();

	void CalculateGameStats(HWND hWnd);

	void sortLODs();
	void Draw();
	void DrawShadows();
	void DrawSSAO();
	void BlurSSAO();
	void DrawToGBuffer();
	void DrawLights();
	void DrawNYBalls();
	void DrawToStreamOutput();
	void DrawWireframe();
	void DrawParticles();
	void ComputeParticles();
	void EmitParticles();
	void InitEmitter();

	void DrawPPTonemap(MyTexture* readHDR);
	void DrawPPVignette(MyTexture* readLDR, MyTexture* writeLDR);
	void DrawPPOutput(MyTexture* readLDR);

	void FlushCommandQueue();

	void InitProjectionMatrix();
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	void OnMouseDown(HWND hWnd);
	void OnMouseUp();

	void Update();

	void InitUploadBuffers();
	void FillUploadBuffers();
	void InitUAVBuffers();
	void CreateConstantBufferView();
	void CreateStructuredBuffersSRV();

	void CompileShaders();

	void CreateSOBuffers();

	void BuildBulbGeometry();
	void BuildBoxGeometry();
	void InitRenderSystem();

	void InitShadowMap();
	std::vector<Vector3> GetFrustumCornersWorldSpace(Matrix invViewProj);
	void UpdateCascades();

	void Parsing();
	void BuildOctree();

	ComPtr<ID3D12Device> GetDevice() const { return device; }
	Camera& GetCamera() { return camera; }
	GameTimer& GetTimer() { return gt; }
	bool IsDeviceCreated() { return device != nullptr; }
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return commandList; }

	void SetClientWH(int newWidth, int newHeight) {
		clientWidth = newWidth;
		clientHeight = newHeight;
	}

	bool m_key_states[256] = { false };

	int GetClientWidth() { return clientWidth; }
	int GetClientHeight() { return clientHeight; }

private:
	void EnableDebug();
	void GetVisibleObjects();
	void DrawLOD0ToGBuffer(UINT& currentInstanceOffset, Submesh& sm);
	void DrawBakedMeshToGBuffer(UINT& currentInstanceOffset, Submesh& sm);
	void DrawLOD1ToGBuffer(UINT& currentInstanceOffset, Submesh& sm);
	void DrawBillboardsToGBuffer(UINT& currentInstanceOffset, Submesh& sm);

	void UpdateParticleData();
	void UpdateMatricesData();
	void UpdateFrustumData();
	void UpdateTextureAnimation();
	void UpdateTreesLights();
	void UpdateShadowData();
	void UpdateHullBuffer();
	void UpdateObjectsBuffer();
	void UpdateCameraConstants();

	GameTimer gt;
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int clientWidth = 1424;
	int clientHeight = 750;
	ComPtr<IDXGIFactory4> DXGIFactory = nullptr;
	ComPtr<ID3D12Device> device = nullptr;
	ComPtr<ID3D12Fence> fence;
	UINT64 currentFence = 0;
	UINT rtvDescriptorSize = 0;
	UINT dsvDescriptorSize = 0;
	UINT cbvDescriptorSize = 0;
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels_;
	
	ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	
	ComPtr<IDXGISwapChain> swapChain = nullptr;

	ComPtr<ID3D12DescriptorHeap> rtvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> cbvSrvHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> uavHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> samplerHeap = nullptr;
	int currentBackBuffer = 0;

	ComPtr<ID3D12Resource> swapChainBuffer[2];
	ComPtr<ID3D12Resource> dsvBuffer = nullptr;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

	ComPtr<ID3D12Resource> vertexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
	D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1];

	ComPtr<ID3D12Resource> indexBufferGPU = nullptr;
	ComPtr<ID3D12Resource> indexBufferUploader = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	std::unique_ptr<UploadBuffer<ObjectConstants>> objectsUploadBuffer = nullptr;
	std::unique_ptr<UploadBuffer<Matrices>> matricesBuffer = nullptr;
	std::unique_ptr<UploadBuffer<ParticleConstants>> particleConstantsBuffer = nullptr;
	std::unique_ptr<UploadBuffer<MaterialConstants>> materialBuffer = nullptr;
	std::unique_ptr<UploadBuffer<LightConstants>> lightBuffer = nullptr;
	std::unique_ptr<UploadBuffer<CameraConstants>> cameraBuffer = nullptr;
	std::unique_ptr<UploadBuffer<HullBuffer>> hullBuffer = nullptr;
	std::unique_ptr<UploadBuffer<MeshInstanceData>> instanceBuffer = nullptr;
	std::unique_ptr<UploadBuffer<WireframeInstanceData>> wireframeInstanceBuffer = nullptr;
	std::unique_ptr<UploadBuffer<ShadowConstants>> shadowBuffer = nullptr;
	std::unique_ptr<UploadBuffer<SsaoConstants>> ssaoBuffer = nullptr;
	std::unique_ptr<UploadBuffer<BlurConstants>> ssaoBlurBuffer = nullptr;

	std::unique_ptr<UploadBuffer<uint32_t>> deadParticlesListUpload = nullptr;
	std::unique_ptr<UploadBuffer<uint32_t>> deadParticlesCounterUpload = nullptr;
	std::unique_ptr<UploadBuffer<uint32_t>> sortParticlesCounterUpload = nullptr;
	ComPtr<ID3D12Resource> RWParticleBuffer = nullptr;
	ComPtr<ID3D12Resource> ParticleDeadList = nullptr;
	ComPtr<ID3D12Resource> ParticleSortList = nullptr;
	ComPtr<ID3D12Resource> deadParticlesCounterBuffer = nullptr;
	ComPtr<ID3D12Resource> sortParticlesCounterBuffer = nullptr;
	const UINT PARTICLE_COUNT = 16384;
	std::vector<Particle> particles;
	Emitter emitter;


	std::vector<MeshInstanceData> instances;

	POINT mouseLastPos;
	Camera camera;
	BVH octree;

	std::vector<UINT> visibleIndices;
	bool treeIsVisible;

	std::vector<int> meshesMaterialIndex;

	SceneData sceneData;

	const float LOD_DISTANCE = 600.0f * 600.0f;
	const float BILLBOARD_DISTANCE = 900.0f * 900.0f;

	Submesh streamOutputMesh;
	ComPtr<ID3D12Resource> defaultTexture;
	std::vector<std::string> materialNames;

	RenderingSystem* renderSystem = nullptr;

	ComPtr<ID3D12Resource> sphereVertexBuffer = nullptr;
	ComPtr<ID3D12Resource> sphereIndexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW sphereVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW sphereIndexBufferView;
	UINT sphereIndexCount = 0;

	ComPtr<ID3D12Resource> wireframeVertexBuffer = nullptr;
	ComPtr<ID3D12Resource> wireframeIndexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW wireframeVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW wireframeIndexBufferView;

	ComPtr<ID3D12Resource> streamOutputBuffer = nullptr;
	ComPtr<ID3D12Resource> filledSizeBuffer = nullptr;
	ComPtr<ID3D12Resource> readbackBuffer = nullptr;
	D3D12_STREAM_OUTPUT_BUFFER_VIEW streamOutputBufferView = {};
	bool isFirstFrame = true;

	std::unique_ptr<ShadowMap> shadowMap;
	float SHADOW_MAP_SIZE = 8192.0f;
	CascadeData cascades;
};

#endif //DX12APP_