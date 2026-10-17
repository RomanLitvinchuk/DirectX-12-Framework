#include "DX12App.h"

std::vector<Vector3> DX12App::GetFrustumCornersWorldSpace(Matrix invViewProj) {
	std::vector<Vector3> frustumCorners;
	frustumCorners.reserve(8);
	for (int x = 0; x < 2; ++x) {
		for (int y = 0; y < 2; ++y) {
			for (int z = 0; z < 2; ++z) {
				Vector4 pt = Vector4::Transform(Vector4(
					2.0f * x - 1.0f,
					2.0f * y - 1.0f,
					z,
					1.0f), invViewProj);
				frustumCorners.push_back(Vector3(pt.x / pt.w, pt.y / pt.w, pt.z / pt.w));
			}
		}
	}
	return frustumCorners;
}

void DX12App::UpdateCascades()
{
	int numCascades = shadowMap->GetNumCascades();

	float nearZ = 1.0f;
	float farZ = 1000.0f;

	std::vector<float> cascadeSplits(numCascades + 1);
	float lambda = 0.5f;


	Vector3 lightDir = renderSystem->sceneLights[0].lightDirection;
	lightDir.Normalize();
	Vector3 up = (fabsf(lightDir.y) > 0.99f) ? Vector3::Right : Vector3::Up;

	cascades.distances[0] = 300.0f;
	cascades.distances[1] = 1000.0f;
	cascades.distances[2] = 10000.0f;

	float cascadeNears[] = { nearZ, cascades.distances[0], cascades.distances[1] };
	float cascadeFars[] = { cascades.distances[0], cascades.distances[1], cascades.distances[2] };

	for (int i = 0; i < numCascades; ++i) {
		float cascadeNear = cascadeNears[i];
		float cascadeFar = cascadeFars[i];
		float aspectRatio = static_cast<float>(clientWidth) / clientHeight;
		Matrix subProj = Matrix::CreatePerspectiveFieldOfView(XMConvertToRadians(60.0f), aspectRatio, cascadeNear, cascadeFar);
		Matrix subViewProj = camera.mView_ * subProj;
		Matrix invSubViewProj = subViewProj.Invert();
		std::vector<Vector3> corners = GetFrustumCornersWorldSpace(invSubViewProj);

		Vector3 center = Vector3::Zero;
		for (const auto& v : corners) {
			center += v;
		}
		center /= corners.size();

		Vector3 lightPos = center - lightDir * cascadeFar;
		Matrix lightView = Matrix::CreateLookAt(center, center + lightDir, up);

		float minX = (std::numeric_limits<float>::max)();
		float maxX = std::numeric_limits<float>::lowest();
		float minY = (std::numeric_limits<float>::max)();
		float maxY = std::numeric_limits<float>::lowest();
		float minZ = (std::numeric_limits<float>::max)();
		float maxZ = std::numeric_limits<float>::lowest();

		for (const auto& v : corners) {
			Vector3 trf = Vector3::Transform(v, lightView);
			minX = (std::min)(minX, trf.x);
			maxX = (std::max)(maxX, trf.x);
			minY = (std::min)(minY, trf.y);
			maxY = (std::max)(maxY, trf.y);
			minZ = (std::min)(minZ, trf.z);
			maxZ = (std::max)(maxZ, trf.z);
		}

		float zMult = 10.0f;
		minZ = (minZ < 0) ? minZ * zMult : minZ / zMult;
		maxZ = (maxZ < 0) ? maxZ / zMult : maxZ * zMult;


		Matrix lightProj = Matrix::CreateOrthographicOffCenter(minX, maxX, minY, maxY, minZ, maxZ);
		cascades.viewProjMats[i] = lightView * lightProj;

		Matrix T(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);


		Matrix shadowTransform = lightView * lightProj * T;
		cascades.shadowTransform[i] = shadowTransform;

	}
}