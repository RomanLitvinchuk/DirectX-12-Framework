#ifndef MODEL_PARSER_H_
#define MODEL_PARSER_H_
#include "scene_data.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class ModelParser {
public:
	void ParseFile(const std::string& filename, 
		const Matrix& transform, 
		UINT instanceCount,
		SceneData& scene);
private:
	void ParseNode(const std::string& filename, aiNode* node, const aiScene* scene, const Matrix& transform, int materialOffset, UINT instanceCount, SceneData& sceneData);
	void ParseMesh(const std::string& filename, const aiScene* scene, aiMesh* mesh, const Matrix& transform, int materialOffset, UINT instanceCount, SceneData& sceneData);
	void ExtractMaterialData(const std::string& filename, int MaterialIndex, aiMaterial* material, SceneData& SceneData);

	const aiScene* TryToImportFile(const std::string& filename);
	void ParseVertices(aiMesh* mesh, const Matrix& transform, SceneData& sceneData);
	void ParseVertexTangentAndBitangent(aiMesh* mesh, int index, Vertex& vertex);
	void ParseVertexPosition(aiMesh* mesh, int index, const Matrix& transform, Vertex& vertex);
	void ParseVertexNormal(aiMesh* mesh, int index, const Matrix& transform, Vertex& vertex);
	void ParseVertexUV(aiMesh* mesh, int index, Vertex& vertex);
	void ParseIndices(aiMesh* mesh, size_t vertexOffset, SceneData& sceneData);
	void ParseSubmeshInstances(Submesh& submesh, UINT instanceCount);

	void ParseMaterialConstants(aiMaterial* material, MaterialConstants& matConst);

	void FindDiffuseTexture(MaterialConstants& matConst, aiMaterial* material, SceneData& sceneData);
	void FindDisplacementOrHeightTexture(MaterialConstants& matConst, aiMaterial* material, SceneData& sceneData);
	void FindNormalTexture(MaterialConstants& matConst, aiMaterial* material, SceneData& sceneData);

	void FindSketchfabMaterials(const std::string& filename, MaterialConstants& matConst, SceneData& sceneData);
	void FindTreeMaterials(const std::string& filename, MaterialConstants& matConst, SceneData& sceneData);
	void FindCubeMaterials(const std::string& filename, MaterialConstants& matConst, SceneData& sceneData);
};


#endif //MODEL_PARSER_H_
