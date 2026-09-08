#include "DX12App.h"
#include <filesystem>
#include "DDSTextureLoader.h"
#include "model_parser.h"

#define RESERVED_TEXTURES 1

void DX12App::LoadTextures()
{
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	UINT index = RESERVED_TEXTURES + 1;

	for (auto& entry : std::filesystem::directory_iterator(L"textures"))
	{
		auto path = entry.path();
		if (path.extension() != L".dds") continue;

		std::wstring name = path.stem().wstring();
		std::transform(name.begin(), name.end(), name.begin(), ::towlower);

		auto tex = std::make_unique<Texture>();
		tex->name_ = std::string(name.begin(), name.end());
		tex->filepath = path.wstring();
		if (tex->name_.find("noise") != std::string::npos) {
			tex->srvHeapIndex = 1;
			tex->isSRGB = false;
		}
		else tex->srvHeapIndex = index++;

		ThrowIfFailed(CreateDDSTextureFromFile12(
			device.Get(),
			commandList.Get(),
			tex->filepath.c_str(),
			tex->Resource,
			tex->UploadHeap));

		std::wcout << L"Loaded texture: [" << name << L"] to index: " << tex->srvHeapIndex << std::endl;

		sceneData.textures[name] = std::move(tex);
	}

	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* lists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, lists);
	FlushCommandQueue();
}

void DX12App::Parsing() {
	ModelParser parser;
	parser.ParseFile("models/sponza.obj", Matrix::Identity, 1, sceneData);

	Matrix Transform = Matrix::CreateScale(0.2f) * Matrix::CreateRotationX(-3.14 / 2) * Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
	parser.ParseFile("models/Christmas Tree Color mm.obj", Transform, 1, sceneData);

	Transform = Matrix::CreateScale(25.0f) * Matrix::CreateTranslation(100.0f, 500.0f, 0.0f);
	parser.ParseFile("models/Sketchfab.fbx", Transform, 1, sceneData);

	Transform = Matrix::CreateScale(30.0f) * Matrix::CreateTranslation(400.0f, 200.0f, 0.0f);
	parser.ParseFile("models/HydraMoonSimpleCube.fbx", Transform, 1, sceneData);

	Transform = Matrix::CreateScale(30.0f) * Matrix::CreateTranslation(700.0f, 0.0f, 0.0f);
	parser.ParseFile("models/Minecraft Tree.obj", Transform, 1, sceneData);
}



void ModelParser::ParseFile(const std::string& filename, const Matrix& transform, UINT instanceCount, SceneData& sceneData) {
	const aiScene* scene = TryToImportFile(filename);
	std::cout << "Parsing: " << filename << " | Num Meshes: " << std::to_string(scene->mNumMeshes) << std::endl;
	int materialOffset = static_cast<int>(sceneData.materials.size());
	ParseNode(filename, scene->mRootNode, scene, transform, materialOffset, instanceCount, sceneData);
}

const aiScene* ModelParser::TryToImportFile(const std::string& filename) {
	const aiScene* scene = aiImportFile(
		filename.c_str(),
		aiProcessPreset_TargetRealtime_MaxQuality |
		aiProcess_Triangulate |
		aiProcess_CalcTangentSpace);

	if (!scene) {
		std::cout << "Error loading " << filename << std::endl;
	}
	return scene;
}

void ModelParser::ParseNode(const std::string& filename, aiNode* node, const aiScene* scene, const Matrix& transform, int materialOffset, UINT instanceCount, SceneData& sceneData) {
	for (int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ParseMesh(filename, scene, mesh, transform, materialOffset, instanceCount, sceneData);
	}

	for (int i = 0; i < node->mNumChildren; i++) {
		ParseNode(filename, node->mChildren[i], scene, transform, materialOffset, instanceCount, sceneData);
	}
}

void ModelParser::ParseMesh(const std::string& filename, const aiScene* scene, aiMesh* mesh, const Matrix& transform, int materialOffset, UINT instanceCount, SceneData& sceneData) {
	size_t vertexOffset = sceneData.vertices.size();
	UINT baseVertex = static_cast<UINT>(sceneData.vertices.size());
	UINT startIndex = static_cast<UINT>(sceneData.indices.size());

	ParseVertices(mesh, transform, sceneData);
	ParseIndices(mesh, vertexOffset, sceneData);

	sceneData.meshIndexCounts.push_back(mesh->mNumFaces * 3);
	Submesh submesh;
	submesh.name_ = filename;
	submesh.indexCount = mesh->mNumFaces * 3;
	submesh.startIndiceIndex = startIndex;
	submesh.startVerticeIndex = 0;
	submesh.materialIndex = mesh->mMaterialIndex + materialOffset;
	submesh.mWorld = transform;

	if (filename.find("Minecraft Tree") != std::string::npos) {
		submesh.startIndiceIndexLOD1 = static_cast<UINT>(sceneData.indices.size());
		submesh.startVerticeIndexLOD1 = 0;

		for (int i = 0; i < mesh->mNumFaces; i += 2) {
			aiFace face = mesh->mFaces[i];
			for (int j = 0; j < face.mNumIndices; ++j) {
				sceneData.indices.push_back(face.mIndices[j] + vertexOffset);
			}
		}
		submesh.indexCountLOD1 = static_cast<UINT>(sceneData.indices.size() - submesh.startIndiceIndexLOD1);
		submesh.hasLOD1 = submesh.indexCountLOD1 > 0;

	}

	DirectX::BoundingBox::CreateFromPoints(
		submesh.box,
		mesh->mNumVertices,
		&sceneData.vertices[baseVertex].pos,
		sizeof(Vertex)
	);


	if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < scene->mNumMaterials) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		ExtractMaterialData(filename, mesh->mMaterialIndex + materialOffset, material, sceneData);
	}

	ParseSubmeshInstances(submesh, instanceCount);
	sceneData.submeshes.push_back(submesh);
}

void ModelParser::ParseVertices(aiMesh* mesh, const Matrix& transform, SceneData& sceneData) {
	for (int i = 0; i < mesh->mNumVertices; ++i) {
		Vertex vertex;

		ParseVertexPosition(mesh, i, transform, vertex);
		ParseVertexNormal(mesh, i, transform, vertex);
		ParseVertexUV(mesh, i, vertex);
		ParseVertexTangentAndBitangent(mesh, i, vertex);
		sceneData.vertices.push_back(vertex);
	}
}

void ModelParser::ParseVertexPosition(aiMesh* mesh, int index, const Matrix& transform, Vertex& vertex)
{
	Vector3 pos(mesh->mVertices[index].x, mesh->mVertices[index].y, mesh->mVertices[index].z);
	vertex.pos = Vector3::Transform(pos, transform);
}

void ModelParser::ParseVertexNormal(aiMesh* mesh, int index, const Matrix& transform, Vertex& vertex)
{
	if (mesh->HasNormals()) {
		Vector3 normal(mesh->mNormals[index].x, mesh->mNormals[index].y, mesh->mNormals[index].z);
		vertex.normal = Vector3::TransformNormal(normal, transform);
		vertex.normal.Normalize();
	}
	else {
		vertex.normal = Vector3(0.0f, 1.0f, 0.0f);
	}
}

void ModelParser::ParseVertexUV(aiMesh* mesh, int index, Vertex& vertex)
{
	if (mesh->HasTextureCoords(0)) {
		vertex.uv.x = mesh->mTextureCoords[0][index].x;
		vertex.uv.y = 1.0f - mesh->mTextureCoords[0][index].y;
	}
	else {
		vertex.uv = Vector2(0.0f, 0.0f);
	}
}

void ModelParser::ParseVertexTangentAndBitangent(aiMesh* mesh, int index, Vertex& vertex)
{
	if (mesh->mTangents) {
		vertex.tangent = { mesh->mTangents[index].x, mesh->mTangents[index].y, mesh->mTangents[index].z };
	}

	if (mesh->mBitangents) {
		vertex.biNormal = { mesh->mBitangents[index].x, mesh->mBitangents[index].y, mesh->mBitangents[index].z };
	}
}



void ModelParser::ParseIndices(aiMesh* mesh, size_t vertexOffset, SceneData& sceneData) {
	for (int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace face = mesh->mFaces[i];
		for (int j = 0; j < face.mNumIndices; ++j) {
			sceneData.indices.push_back(face.mIndices[j] + vertexOffset);
		}
	}
}

void ModelParser::ParseSubmeshInstances(Submesh& submesh, UINT instanceCount) {
	submesh.InstanceCount = instanceCount;
	int gridSize = static_cast<int>(std::ceil(std::sqrt(submesh.InstanceCount)));

	float spacing = 4.0f;

	for (int i = 0; i < submesh.InstanceCount; i++) {
		MeshInstanceData instance;

		if (submesh.InstanceCount > 1) {
			int gridX = i % gridSize;
			int gridZ = i / gridSize;

			float offsetX = (gridX - gridSize / 2.0f) * spacing;
			float offsetZ = (gridZ - gridSize / 2.0f) * spacing;

			Matrix gridTranslation = Matrix::CreateTranslation(offsetX, 0.0f, offsetZ);

			instance.World_ = submesh.mWorld * gridTranslation;
		}
		else {
			instance.World_ = submesh.mWorld;
		}

		instance.TexTransform_ = submesh.mTexTransform;

		Matrix invWorld = instance.World_.Invert();
		instance.InvTWorld_ = invWorld.Transpose();

		submesh.instances.push_back(instance);
	}
}

void ModelParser::ExtractMaterialData(const std::string& filename, int GlobalMaterialIndex, aiMaterial* material, SceneData& sceneData) {
	if (sceneData.materials.size() <= GlobalMaterialIndex) {
		sceneData.materials.resize(GlobalMaterialIndex + 1);
		sceneData.materialNames.resize(GlobalMaterialIndex + 1); 
	}

	MaterialConstants& matConst = sceneData.materials[GlobalMaterialIndex];
	matConst = {};
	matConst.MatTransform = Matrix::Identity;

	ParseMaterialConstants(material, matConst);

	FindDiffuseTexture(matConst, material, sceneData);
	FindDisplacementOrHeightTexture(matConst, material, sceneData);
	FindNormalTexture(matConst, material, sceneData);

	FindSketchfabMaterials(filename, matConst, sceneData);
	FindCubeMaterials(filename, matConst, sceneData);
	FindTreeMaterials(filename, matConst, sceneData);

	sceneData.materials[GlobalMaterialIndex] = matConst;
}

void ModelParser::ParseMaterialConstants(aiMaterial* material, MaterialConstants& matConst) {
	aiColor3D color = { 1.0f, 1.0f, 1.0f };
	float parameter = 0.0f;
	if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
		matConst.DiffuseColor = { color.r, color.g, color.b, 1.0f };
	}
	if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
		matConst.AmbientColor = { color.r, color.g, color.b, 1.0f };
	}
	if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
		matConst.SpecularColor = { color.r, color.g, color.b, 1.0f };
	}
	if (material->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
		matConst.EmissiveColor = { color.r, color.g, color.b, 1.0f };
	}
	if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, color) == AI_SUCCESS) {
		matConst.TransparentColor = { color.r, color.g, color.b, 1.0f };
	}
	if (material->Get(AI_MATKEY_SHININESS, parameter) == AI_SUCCESS) {
		matConst.Shininess = parameter;
	}
	if (material->Get(AI_MATKEY_OPACITY, parameter) == AI_SUCCESS) {
		matConst.Opacity = parameter;
	}
}

void ModelParser::FindDiffuseTexture(MaterialConstants& matConst, aiMaterial* material, SceneData& sceneData)
{
	aiString texPath;
	bool foundPath = false;
	if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) foundPath = true;

	if (foundPath) {
		std::filesystem::path p(texPath.C_Str());
		std::wstring wName = p.stem().wstring();
		std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);
		std::string sName(wName.begin(), wName.end());
		std::cout << "   Looking for: " << sName << std::endl;

		if (wName.find(L"lion") != std::wstring::npos) {
			matConst.isLion = 1;
		}
		if (wName.find(L"christmas tree color mm_0") != std::wstring::npos) {
			matConst.isTree = 1;
		}

		if (sceneData.textures.count(wName)) {
			matConst.diffuseTextureIndex = sceneData.textures[wName]->srvHeapIndex;
			sceneData.textures[wName]->isSRGB = true;
		}
		else {
			std::cout << "DO NOT FIND DIFFUSE TEXTURE" << std::endl;
		}
	}
}

void ModelParser::FindDisplacementOrHeightTexture(MaterialConstants& matConst, aiMaterial* material, SceneData& sceneData)
{
	bool foundPath = false;
	aiString dispPath;
	if (material->GetTexture(aiTextureType_DISPLACEMENT, 0, &dispPath) == AI_SUCCESS) foundPath = true;
	else if (material->GetTexture(aiTextureType_HEIGHT, 0, &dispPath) == AI_SUCCESS) foundPath = true;

	if (foundPath) {
		std::filesystem::path p(dispPath.C_Str());
		std::wstring wName = p.stem().wstring();
		std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);
		std::string sName(wName.begin(), wName.end());
		std::cout << "   Looking for: " << sName << std::endl;

		if (sceneData.textures.count(wName)) {
			matConst.normalTextureIndex = sceneData.textures[wName]->srvHeapIndex;
			matConst.hasNormalTexture = 1;
		}
		else {
			std::cout << "DO NOT FIND DISP TEXTURE" << std::endl;
		}
	}
}

void ModelParser::FindNormalTexture(MaterialConstants& matConst, aiMaterial* material, SceneData& sceneData) {
	bool foundPath = false;
	aiString normPath;
	if (material->GetTexture(aiTextureType_NORMALS, 0, &normPath) == AI_SUCCESS) foundPath = true;

	if (foundPath) {
		std::filesystem::path p(normPath.C_Str());
		std::wstring wName = p.stem().wstring();
		std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);
		std::string sName(wName.begin(), wName.end());
		std::cout << "   Looking for: " << sName << std::endl;

		if (sceneData.textures.count(wName)) {
			matConst.normalTextureIndex = sceneData.textures[wName]->srvHeapIndex;
			matConst.hasNormalTexture = 1;
		}
		else {
			std::cout << "DO NOT FIND NORMAL TEXTURE" << std::endl;
		}
	}
}

void ModelParser::FindSketchfabMaterials(const std::string& filename, MaterialConstants& matConst, SceneData& sceneData) {
	if (filename.find("Sketchfab") != std::string::npos) {
		std::wstring wName = L"Stone_Pathway_Normal";
		std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);
		if (sceneData.textures.count(wName)) {
			matConst.normalTextureIndex = sceneData.textures[wName]->srvHeapIndex;
			matConst.hasNormalTexture = 1;
		}
		wName = L"Stone_Pathway_Height";
		std::transform(wName.begin(), wName.end(), wName.begin(), ::towlower);
		if (sceneData.textures.count(wName)) {
			matConst.displacementTextureIndex = sceneData.textures[wName]->srvHeapIndex;
			matConst.hasDisplacementTexture = 1;
		}
	}
}

void ModelParser::FindTreeMaterials(const std::string& filename, MaterialConstants& matConst, SceneData& sceneData)
{
	if (filename.find("Minecraft Tree") != std::string::npos) {
		std::wstring wName = L"tree_billboard";
		if (sceneData.textures.count(wName)) {
			matConst.hasBillboardTexture = 1;
			matConst.billboardTextureIndex = sceneData.textures[wName]->srvHeapIndex;
			sceneData.textures[wName]->isSRGB = true;
		}
	}
}

void ModelParser::FindCubeMaterials(const std::string& filename, MaterialConstants& matConst, SceneData& sceneData)
{
	if (filename.find("HydraMoonSimpleCube") != std::string::npos) {
		std::wstring wName = L"white";
		if (sceneData.textures.count(wName)) {
			matConst.diffuseTextureIndex = sceneData.textures[wName]->srvHeapIndex;
			sceneData.textures[wName]->isSRGB = true;
		}
		wName = L"rainbow";
		if (sceneData.textures.count(wName)) {
			matConst.shadowTextureIndex = sceneData.textures[wName]->srvHeapIndex;
			matConst.hasShadowTexture = 1;
			sceneData.textures[wName]->isSRGB = true;
		}
	}
}
