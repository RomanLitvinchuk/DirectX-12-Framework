#pragma once
#include <DirectXCollision.h>
#include <vector>
#include <memory>
#include <Windows.h>
#include "submesh.h"

struct BVHNode {
    BoundingBox bounds;
    std::vector<UINT> submeshIndices;
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;
    bool isLeaf = true;
};

class BVH {
public:
    void Build(const std::vector<Submesh>& submeshes);
    void GetVisibleObjects(const BoundingFrustum frustum,
        const std::vector<Submesh>& submeshes,
        std::vector<UINT>& outVisibleIndices) const;
    void GetAllNodes(std::vector<BVHNode*>& outNodes);

private:
    std::unique_ptr<BVHNode> root;
    UINT MAX_OBJECTS_PER_NODE = 16;
    void BuildRecursive(BVHNode* node, const std::vector<Submesh>& submeshes, std::vector<UINT>& indices);
    void GetVisibleObjectsRecursive(const BVHNode* node,
        const BoundingFrustum frustum,
        const std::vector<Submesh>& submeshes,
        std::vector<UINT>& outVisibleIndices) const;
    void CollectAllNodesRecursive(BVHNode* node, std::vector<BVHNode*>& outNodes);
};