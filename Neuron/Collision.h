#pragma once

#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <vector>
#include <memory>

class Surface;

struct BVHTriangle
{
  XMFLOAT3 v0, v1, v2;
  BoundingBox bounds;

  void ComputeBounds();
};

struct BVHNode
{
  BoundingBox bounds;
  std::unique_ptr<BVHNode> left;
  std::unique_ptr<BVHNode> right;
  std::vector<int> triangleIndices;// Only for leaf nodes

  bool IsLeaf() const { return !left && !right; }
};

class Collision
{
public:
  Collision() = default;
  ~Collision() = default;

  // Build BVH from surface mesh data
  void Build(Surface *surface);

  // Test if this BVH intersects another (with world transforms)
  bool Intersects(const Collision &other, const XMMATRIX &thisWorld, const XMMATRIX &otherWorld) const;

  // Get root bounding box
  const BoundingBox *GetBounds() const;

private:
  static constexpr int MAX_TRIANGLES_PER_LEAF = 4;
  static constexpr int MAX_DEPTH = 20;

  std::unique_ptr<BVHNode> root;
  std::vector<BVHTriangle> triangles;

  std::unique_ptr<BVHNode> BuildRecursive(std::vector<int> &indices, int depth);

  bool IntersectNodes(const BVHNode *nodeA, const BVHNode *nodeB, const std::vector<BVHTriangle> &trianglesB, const XMMATRIX &worldA, const XMMATRIX &worldB) const;

  static bool TriangleTriangleIntersect(const XMVECTOR &a0, const XMVECTOR &a1, const XMVECTOR &a2, const XMVECTOR &b0, const XMVECTOR &b1, const XMVECTOR &b2);
};
