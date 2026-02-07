#include "pch.h"
#include "Collision.h"
#include "Solid.h"
#include <algorithm>

using namespace DirectX;

void BVHTriangle::ComputeBounds()
{
    XMVECTOR minV = XMVectorMin(XMVectorMin(XMLoadFloat3(&v0), XMLoadFloat3(&v1)), XMLoadFloat3(&v2));
    XMVECTOR maxV = XMVectorMax(XMVectorMax(XMLoadFloat3(&v0), XMLoadFloat3(&v1)), XMLoadFloat3(&v2));
    BoundingBox::CreateFromPoints(bounds, minV, maxV);
}

void Collision::Build(Surface* surface)
{
    if (!surface)
        return;

    triangles.clear();
    root.reset();

    VertexSet* vset = surface->GetVertexSet();
    if (!vset)
        return;

    int npolys = surface->NumPolys();
    Poly* polys = surface->GetPolys();

    // Extract triangles from polygons
    for (int i = 0; i < npolys; i++)
    {
        Poly* p = polys + i;

        if (p->nverts == 3)
        {
            BVHTriangle tri;
            Vec3& v0 = vset->loc[p->verts[0]];
            Vec3& v1 = vset->loc[p->verts[2]];  // Note: winding order swap
            Vec3& v2 = vset->loc[p->verts[1]];

            tri.v0 = { v0.x, v0.y, v0.z };
            tri.v1 = { v1.x, v1.y, v1.z };
            tri.v2 = { v2.x, v2.y, v2.z };
            tri.ComputeBounds();
            triangles.push_back(tri);
        }
        else if (p->nverts == 4)
        {
            // Split quad into two triangles
            BVHTriangle tri1, tri2;
            Vec3& v0 = vset->loc[p->verts[0]];
            Vec3& v1 = vset->loc[p->verts[1]];
            Vec3& v2 = vset->loc[p->verts[2]];
            Vec3& v3 = vset->loc[p->verts[3]];

            tri1.v0 = { v0.x, v0.y, v0.z };
            tri1.v1 = { v2.x, v2.y, v2.z };
            tri1.v2 = { v1.x, v1.y, v1.z };
            tri1.ComputeBounds();

            tri2.v0 = { v0.x, v0.y, v0.z };
            tri2.v1 = { v3.x, v3.y, v3.z };
            tri2.v2 = { v2.x, v2.y, v2.z };
            tri2.ComputeBounds();

            triangles.push_back(tri1);
            triangles.push_back(tri2);
        }
    }

    if (triangles.empty())
        return;

    // Create index list for all triangles
    std::vector<int> indices(triangles.size());
    for (size_t i = 0; i < triangles.size(); i++)
        indices[i] = static_cast<int>(i);

    // Build tree recursively
    root = BuildRecursive(indices, 0);
}

std::unique_ptr<BVHNode> Collision::BuildRecursive(std::vector<int>& indices, int depth)
{
    if (indices.empty())
        return nullptr;

    auto node = std::make_unique<BVHNode>();

    // Compute combined bounds of all triangles in this node
    XMVECTOR minV = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0);
    XMVECTOR maxV = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0);

    for (int idx : indices)
    {
        const auto& tri = triangles[idx];
        XMVECTOR center = XMLoadFloat3(&tri.bounds.Center);
        XMVECTOR extents = XMLoadFloat3(&tri.bounds.Extents);
        minV = XMVectorMin(minV, XMVectorSubtract(center, extents));
        maxV = XMVectorMax(maxV, XMVectorAdd(center, extents));
    }

    BoundingBox::CreateFromPoints(node->bounds, minV, maxV);

    // If few enough triangles or max depth reached, make this a leaf
    if (indices.size() <= MAX_TRIANGLES_PER_LEAF || depth >= MAX_DEPTH)
    {
        node->triangleIndices = std::move(indices);
        return node;
    }

    // Find longest axis to split on
    XMFLOAT3 extent;
    XMStoreFloat3(&extent, XMVectorSubtract(maxV, minV));

    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > (axis == 0 ? extent.x : extent.y)) axis = 2;

    // Sort triangles by their centroid along the split axis
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        const auto& ta = triangles[a];
        const auto& tb = triangles[b];
        float ca = (axis == 0) ? ta.bounds.Center.x : (axis == 1) ? ta.bounds.Center.y : ta.bounds.Center.z;
        float cb = (axis == 0) ? tb.bounds.Center.x : (axis == 1) ? tb.bounds.Center.y : tb.bounds.Center.z;
        return ca < cb;
    });

    // Split in the middle
    size_t mid = indices.size() / 2;
    std::vector<int> leftIndices(indices.begin(), indices.begin() + mid);
    std::vector<int> rightIndices(indices.begin() + mid, indices.end());

    node->left = BuildRecursive(leftIndices, depth + 1);
    node->right = BuildRecursive(rightIndices, depth + 1);

    return node;
}

const DirectX::BoundingBox* Collision::GetBounds() const
{
    return root ? &root->bounds : nullptr;
}

bool Collision::Intersects(const Collision& other,
                     const XMMATRIX& thisWorld,
                     const XMMATRIX& otherWorld) const
{
    if (!root || !other.root)
        return false;

    return IntersectNodes(root.get(), other.root.get(), other.triangles, thisWorld, otherWorld);
}

bool Collision::IntersectNodes(const BVHNode* nodeA, const BVHNode* nodeB,
                         const std::vector<BVHTriangle>& trianglesB,
                         const XMMATRIX& worldA, const XMMATRIX& worldB) const
{
    // Transform bounds to world space and test overlap
    BoundingBox boundsA, boundsB;
    nodeA->bounds.Transform(boundsA, worldA);
    nodeB->bounds.Transform(boundsB, worldB);

    if (!boundsA.Intersects(boundsB))
        return false;  // Early out - bounding boxes don't overlap

    // Both are leaves - do triangle-triangle tests
    if (nodeA->IsLeaf() && nodeB->IsLeaf())
    {
        for (int idxA : nodeA->triangleIndices)
        {
            const auto& triA = triangles[idxA];
            XMVECTOR a0 = XMVector3Transform(XMLoadFloat3(&triA.v0), worldA);
            XMVECTOR a1 = XMVector3Transform(XMLoadFloat3(&triA.v1), worldA);
            XMVECTOR a2 = XMVector3Transform(XMLoadFloat3(&triA.v2), worldA);

            for (int idxB : nodeB->triangleIndices)
            {
                const auto& triB = trianglesB[idxB];
                XMVECTOR b0 = XMVector3Transform(XMLoadFloat3(&triB.v0), worldB);
                XMVECTOR b1 = XMVector3Transform(XMLoadFloat3(&triB.v1), worldB);
                XMVECTOR b2 = XMVector3Transform(XMLoadFloat3(&triB.v2), worldB);

                if (TriangleTriangleIntersect(a0, a1, a2, b0, b1, b2))
                    return true;  // First contact found - early exit
            }
        }
        return false;
    }

    // Recurse into children
    if (nodeA->IsLeaf())
    {
        // Descend into B's children
        if (nodeB->left && IntersectNodes(nodeA, nodeB->left.get(), trianglesB, worldA, worldB))
            return true;
        if (nodeB->right && IntersectNodes(nodeA, nodeB->right.get(), trianglesB, worldA, worldB))
            return true;
    }
    else if (nodeB->IsLeaf())
    {
        // Descend into A's children
        if (nodeA->left && IntersectNodes(nodeA->left.get(), nodeB, trianglesB, worldA, worldB))
            return true;
        if (nodeA->right && IntersectNodes(nodeA->right.get(), nodeB, trianglesB, worldA, worldB))
            return true;
    }
    else
    {
        // Both are internal nodes - descend into all child combinations
        if (nodeA->left && nodeB->left && IntersectNodes(nodeA->left.get(), nodeB->left.get(), trianglesB, worldA, worldB))
            return true;
        if (nodeA->left && nodeB->right && IntersectNodes(nodeA->left.get(), nodeB->right.get(), trianglesB, worldA, worldB))
            return true;
        if (nodeA->right && nodeB->left && IntersectNodes(nodeA->right.get(), nodeB->left.get(), trianglesB, worldA, worldB))
            return true;
        if (nodeA->right && nodeB->right && IntersectNodes(nodeA->right.get(), nodeB->right.get(), trianglesB, worldA, worldB))
            return true;
    }

    return false;
}

bool Collision::TriangleTriangleIntersect(
    const XMVECTOR& a0, const XMVECTOR& a1, const XMVECTOR& a2,
    const XMVECTOR& b0, const XMVECTOR& b1, const XMVECTOR& b2)
{
    // Use DirectXCollision's built-in triangle intersection test
    return TriangleTests::Intersects(a0, a1, a2, b0, b1, b2);
}
