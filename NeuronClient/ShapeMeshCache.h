#pragma once

class ShapeStatic;
class ShapeFragmentData;

namespace OpenGLD3D { struct CustomVertex; }

// ---------------------------------------------------------------------------
// Per-ShapeStatic GPU vertex buffer.
// Holds one contiguous vertex buffer containing all fragments' triangle data.
// Each fragment maps to a (vertexStart, vertexCount) range.
// Uploaded once when first rendered; valid for the lifetime of the ShapeStatic.
// ---------------------------------------------------------------------------

struct FragmentGPURange
{
    UINT vertexStart = 0;
    UINT vertexCount = 0;
};

struct ShapeMeshGPU
{
    com_ptr<ID3D12Resource>         vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW        vbView{};
    std::vector<FragmentGPURange>   fragments; // Indexed by fragment ordinal
};

// ---------------------------------------------------------------------------
// ShapeMeshCache — singleton cache mapping ShapeStatic* → ShapeMeshGPU.
//
// Follows the TreeRenderer pattern:
//   - On first render, walks the fragment tree, assembles CustomVertex data
//     (pos + normal + color), and uploads to a default-heap vertex buffer.
//   - Subsequent frames reuse the GPU buffer directly — zero per-frame vertex
//     upload for shape geometry.
//   - Shapes are loaded once and never modified, so the GPU buffer is valid
//     for the shape's lifetime.
// ---------------------------------------------------------------------------

class ShapeMeshCache
{
  public:
    [[nodiscard]] static ShapeMeshCache& Get();

    // Ensure the shape's geometry is uploaded to the GPU.  No-op if already cached.
    void EnsureUploaded(const ShapeStatic* _shape);

    // Returns the GPU data for a shape, or nullptr if not yet uploaded.
    [[nodiscard]] const ShapeMeshGPU* GetGPUData(const ShapeStatic* _shape) const;

    // Release GPU resources for a specific shape (e.g., on unload).
    void Release(const ShapeStatic* _shape);

    // Release all cached GPU resources (e.g., on shutdown / device lost).
    void Shutdown();

  private:
    ShapeMeshCache() = default;
    ~ShapeMeshCache() = default;

    // Recursively count total triangles across all fragments.
    static UINT CountTotalTriangles(const ShapeFragmentData* _frag);

    // Recursively write CustomVertex data for all fragments.
    static void WriteFragmentVertices(const ShapeFragmentData* _frag,
                                      OpenGLD3D::CustomVertex* _dst,
                                      UINT& _currentVertex,
                                      std::vector<FragmentGPURange>& _ranges);

    std::unordered_map<const ShapeStatic*, ShapeMeshGPU> m_cache;
};
