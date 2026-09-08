#ifndef SCENE_DATA_H_
#define SCENE_DATA_H_
#include "vertex.h"
#include "submesh.h"
#include "materials.h"
#include "texture.h"

struct SceneData {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<Submesh> submeshes;
	std::vector<UINT> meshIndexCounts;

	std::vector<MaterialConstants> materials;
	std::vector<std::string> materialNames;

	std::unordered_map<std::wstring, std::unique_ptr<Texture>> textures;
};

#endif //SCENE_DATA_H_
