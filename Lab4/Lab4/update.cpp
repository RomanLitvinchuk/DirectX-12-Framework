#include "DX12App.h"

void DX12App::OnMouseDown(HWND hWnd) {
	SetCapture(hWnd);
}

void DX12App::OnMouseUp() {
	ReleaseCapture();
}

void DX12App::Update() {
	camera.UpdateCameraPos(m_key_states, gt);
	camera.UpdateViewMatrix();
	UpdateMatricesData();
	UpdateParticleData();
	UpdateFrustumData();
	UpdateCameraConstants();
	UpdateObjectsBuffer();
	UpdateHullBuffer();
	UpdateTextureAnimation();
	UpdateTreesLights();

	UpdateCascades();
	UpdateShadowData();
}

void DX12App::UpdateMatricesData() {
	Matrices matricesData;
	matricesData.Proj = camera.mProj_.Transpose();
	matricesData.View = camera.mView_.Transpose();
	Matrix invView = camera.mView_.Transpose();
	invView = invView.Invert();
	matricesData.invView = invView;
	Matrix invProj = camera.mProj_.Transpose();
	invProj = invProj.Invert();
	matricesData.invProj = invProj;
	matricesBuffer->CopyData(0, matricesData);
}

void DX12App::UpdateParticleData() {
	ParticleConstants particleData;
	particleData.deltaTime = gt.DeltaTime();
	particleData.CameraPos = camera.mCameraPos;
	particleData.particlesCount = PARTICLE_COUNT;
	particleConstantsBuffer->CopyData(0, particleData);
}

void DX12App::UpdateFrustumData() {
	if (camera.isFrustumCullingEnabled) {
		XMVECTOR pos = XMLoadFloat3(&camera.mCameraPos);
		XMVECTOR target = pos + XMLoadFloat3(&camera.mCameraTarget);
		XMVECTOR up = XMLoadFloat3(&camera.mCameraUp);
		XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

		BoundingFrustum viewFrustum;
		BoundingFrustum::CreateFromMatrix(viewFrustum, camera.xmProj);

		XMVECTOR determinant;
		XMMATRIX invView = XMMatrixInverse(&determinant, view);

		viewFrustum.Transform(camera.frustum, invView);
	}
}

void DX12App::UpdateObjectsBuffer() {
	ObjectConstants obj;
	Matrix TWorld = camera.mWorld_.Transpose();
	obj.View = camera.mView_.Transpose();
	obj.Proj = camera.mProj_.Transpose();
	obj.gTime = gt.TotalTime();
	objectsUploadBuffer->CopyData(0, obj);
}

void DX12App::UpdateCameraConstants() {
	Matrix ViewProj = camera.mView_ * camera.mProj_;
	Matrix InvViewProj = ViewProj.Invert();
	ViewProj = ViewProj.Transpose();
	InvViewProj = InvViewProj.Transpose();
	CameraConstants camConst;
	camConst.invViewProj = InvViewProj;
	camConst.cameraPos = camera.mCameraPos;
	cameraBuffer->CopyData(0, camConst);
}

void DX12App::UpdateHullBuffer() {
	HullBuffer hullConst;
	hullConst.CameraPos = camera.mCameraPos;
	hullConst.gMinTess = 1;
	hullConst.gMaxTess = 5;
	hullConst.gMinDist = 10.0f;
	hullConst.gMaxDist = 200.0f;
	hullBuffer->CopyData(0, hullConst);
}

void DX12App::UpdateTextureAnimation() {
	for (int i = 0; i < sceneData.materials.size(); ++i) {
		if (sceneData.materials[i].isTree == 1) {
			float tu = sceneData.materials[i].MatTransform(1, 0);
			float tv = sceneData.materials[i].MatTransform(1, 1);
			tu += 0.1f * gt.DeltaTime();
			tv += 0.02f * gt.DeltaTime();

			if (tu >= 1.0f)
				tu -= 1.0f;
			if (tv >= 1.0f)
				tv -= 1.0f;
			sceneData.materials[i].MatTransform(1, 0) = tu;
			sceneData.materials[i].MatTransform(1, 1) = tv;
		}
		materialBuffer->CopyData(i, sceneData.materials[i]);
	}
}

void DX12App::UpdateTreesLights() {
	static float timeAccumulator = 0.0f;
	timeAccumulator += gt.DeltaTime();

	if (timeAccumulator >= 1.0f) {
		timeAccumulator = 0.0f;

		for (int i = 0; i < renderSystem->sceneLights_.size(); i++) {
			if (renderSystem->sceneLights_[i].lightType == 1) {
				renderSystem->sceneLights_[i].lightColor.x = static_cast<float>(rand()) / RAND_MAX;
				renderSystem->sceneLights_[i].lightColor.y = static_cast<float>(rand()) / RAND_MAX;
				renderSystem->sceneLights_[i].lightColor.z = static_cast<float>(rand()) / RAND_MAX;
			}
			lightBuffer->CopyData(i, renderSystem->sceneLights_[i]);
		}
	}
}

void DX12App::UpdateShadowData() {
	int numCascades = shadowMap->GetNumCascades();
	for (int i = 0; i < numCascades; ++i) {
		ShadowConstants shadowData = {};
		shadowData.lightViewProj = cascades.viewProjMats[i];
		shadowData.shadowTransform_ = cascades.shadowTransform[i];
		shadowData.cascadeDistances = Vector4(cascades.distances[0], cascades.distances[1], cascades.distances[2], 0.0f);
		shadowBuffer->CopyData(i, shadowData);
	}
}