#include "pch.h"
#include "ShapeMeshCache.h"
#include "d3d12_backend.h"
#include "UploadRingBuffer.h"
#include "opengl_directx_internals.h"
#include "ShapeStatic.h"

using namespace DirectX;
using namespace OpenGLD3D;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

ShapeMeshCache& ShapeMeshCache::Get()
{
    static ShapeMeshCache s_instance;
    return s_instance;
}

// ---------------------------------------------------------------------------
// CountTotalTriangles — DFS over fragment tree
// ---------------------------------------------------------------------------

UINT ShapeMeshCache::CountTotalTriangles(const ShapeFragmentData* _frag)
{
    UINT total = _frag->m_numTriangles;
    int numChildren = _frag->m_childFragments.Size();
    for (int i = 0; i < numChildren; ++i)
        total += CountTotalTriangles(_frag->m_childFragments.GetData(i));
    return total;
}

// ---------------------------------------------------------------------------
// WriteFragmentVertices — DFS, writes 3 CustomVertex per triangle
// ---------------------------------------------------------------------------

void ShapeMeshCache::WriteFragmentVertices(const ShapeFragmentData* _frag,
                                           CustomVertex* _dst,
                                           UINT& _currentVertex,
                                           std::vector<FragmentGPURange>& _ranges)
{
    FragmentGPURange range;
    range.vertexStart = _currentVertex;
    range.vertexCount = _frag->m_numTriangles * 3;

    int norm = 0;
    for (unsigned int i = 0; i < _frag->m_numTriangles; ++i)
    {
        const VertexPosCol* vertA = &_frag->m_vertices[_frag->m_triangles[i].v1];
        const VertexPosCol* vertB = &_frag->m_vertices[_frag->m_triangles[i].v2];
        const VertexPosCol* vertC = &_frag->m_vertices[_frag->m_triangles[i].v3];

        const LegacyVector3& normalVec = _frag->m_normals[norm];
        constexpr unsigned char alpha = 255;

        // Vertex A
        {
            CustomVertex& v = _dst[_currentVertex++];
            v.x  = _frag->m_positions[vertA->m_posId].x;
            v.y  = _frag->m_positions[vertA->m_posId].y;
            v.z  = _frag->m_positions[vertA->m_posId].z;
      v.nx = -normalVec.x;
      v.ny = -normalVec.y;
      v.nz = -normalVec.z;
            v.r8 = _frag->m_colours[vertA->m_colId].r;
            v.g8 = _frag->m_colours[vertA->m_colId].g;
            v.b8 = _frag->m_colours[vertA->m_colId].b;
            v.a8 = alpha;
            v.u  = 0.0f;
            v.v  = 0.0f;
            v.u2 = 0.0f;
            v.v2 = 0.0f;
        }

        // Vertex B
        {
            CustomVertex& v = _dst[_currentVertex++];
            v.x  = _frag->m_positions[vertB->m_posId].x;
            v.y  = _frag->m_positions[vertB->m_posId].y;
            v.z  = _frag->m_positions[vertB->m_posId].z;
            v.nx = -normalVec.x;
            v.ny = -normalVec.y;
            v.nz = -normalVec.z;
            v.r8 = _frag->m_colours[vertB->m_colId].r;
            v.g8 = _frag->m_colours[vertB->m_colId].g;
            v.b8 = _frag->m_colours[vertB->m_colId].b;
            v.a8 = alpha;
            v.u  = 0.0f;
            v.v  = 0.0f;
            v.u2 = 0.0f;
            v.v2 = 0.0f;
        }

        // Vertex C
        {
            CustomVertex& v = _dst[_currentVertex++];
            v.x  = _frag->m_positions[vertC->m_posId].x;
            v.y  = _frag->m_positions[vertC->m_posId].y;
            v.z  = _frag->m_positions[vertC->m_posId].z;
            v.nx = -normalVec.x;
            v.ny = -normalVec.y;
            v.nz = -normalVec.z;
            v.r8 = _frag->m_colours[vertC->m_colId].r;
            v.g8 = _frag->m_colours[vertC->m_colId].g;
            v.b8 = _frag->m_colours[vertC->m_colId].b;
            v.a8 = alpha;
            v.u  = 0.0f;
            v.v  = 0.0f;
            v.u2 = 0.0f;
            v.v2 = 0.0f;
        }

        norm++;
    }

    // Store range for this fragment's ordinal
    if (_frag->m_fragmentIndex >= 0)
    {
        if (static_cast<int>(_ranges.size()) <= _frag->m_fragmentIndex)
            _ranges.resize(_frag->m_fragmentIndex + 1);
        _ranges[_frag->m_fragmentIndex] = range;
    }

    // Recurse into children
    int numChildren = _frag->m_childFragments.Size();
    for (int i = 0; i < numChildren; ++i)
        WriteFragmentVertices(_frag->m_childFragments.GetData(i), _dst, _currentVertex, _ranges);
}

// ---------------------------------------------------------------------------
// EnsureUploaded — uploads shape geometry to a default-heap vertex buffer
// ---------------------------------------------------------------------------

void ShapeMeshCache::EnsureUploaded(const ShapeStatic* _shape)
{
    if (!_shape || !_shape->m_rootFragment)
        return;

    // Already cached?
    if (m_cache.contains(_shape))
        return;

    UINT totalTriangles = CountTotalTriangles(_shape->m_rootFragment);
    if (totalTriangles == 0)
        return;

    UINT totalVertices = totalTriangles * 3;
    SIZE_T vertexSize  = sizeof(CustomVertex);
    SIZE_T bufferSize  = static_cast<SIZE_T>(totalVertices) * vertexSize;

    // --- Assemble CPU-side vertex data ---
    std::vector<CustomVertex> cpuVertices(totalVertices);
    std::vector<FragmentGPURange> ranges;
    ranges.resize(_shape->m_numFragments);

    UINT currentVertex = 0;
    WriteFragmentVertices(_shape->m_rootFragment, cpuVertices.data(), currentVertex, ranges);
    DEBUG_ASSERT(currentVertex == totalVertices);

    // --- Create default-heap vertex buffer ---
    auto* device  = Graphics::Core::Get().GetD3DDevice();
    auto* cmdList = Graphics::Core::Get().GetCommandList();

    D3D12_HEAP_PROPERTIES defaultHeap = {};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width            = bufferSize;
    bufferDesc.Height           = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels        = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ShapeMeshGPU gpu;
    check_hresult(device->CreateCommittedResource(
        &defaultHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON, nullptr,
        IID_GRAPHICS_PPV_ARGS(gpu.vertexBuffer)));

#if defined(_DEBUG)
    // Name the resource for PIX/debug
    if (_shape->m_name)
    {
        wchar_t nameBuf[128];
        swprintf_s(nameBuf, L"ShapeMesh:%hs", _shape->m_name);
        gpu.vertexBuffer->SetName(nameBuf);
    }
#endif

    // --- Upload via ring buffer ---
    auto alloc = Graphics::GetCurrentUploadBuffer().Allocate(bufferSize, sizeof(float));
    memcpy(alloc.cpuPtr, cpuVertices.data(), bufferSize);

    // Transition COMMON → COPY_DEST
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = gpu.vertexBuffer.get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList->ResourceBarrier(1, &barrier);

    cmdList->CopyBufferRegion(gpu.vertexBuffer.get(), 0,
                              Graphics::GetCurrentUploadBuffer().GetResource(),
                              alloc.offset, bufferSize);

    // Transition COPY_DEST → VERTEX_AND_CONSTANT_BUFFER
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    cmdList->ResourceBarrier(1, &barrier);

    // --- Fill VB view ---
    gpu.vbView.BufferLocation = gpu.vertexBuffer->GetGPUVirtualAddress();
    gpu.vbView.SizeInBytes    = static_cast<UINT>(bufferSize);
    gpu.vbView.StrideInBytes  = static_cast<UINT>(vertexSize);
    gpu.fragments             = std::move(ranges);

    m_cache.emplace(_shape, std::move(gpu));

    DebugTrace("ShapeMeshCache: uploaded '{}' - {} triangles, {} bytes\n",
               _shape->m_name ? _shape->m_name : "<unnamed>",
               totalTriangles,
               static_cast<unsigned int>(bufferSize));
}

// ---------------------------------------------------------------------------
// GetGPUData
// ---------------------------------------------------------------------------

const ShapeMeshGPU* ShapeMeshCache::GetGPUData(const ShapeStatic* _shape) const
{
    auto it = m_cache.find(_shape);
    return (it != m_cache.end()) ? &it->second : nullptr;
}

// ---------------------------------------------------------------------------
// Release / Shutdown
// ---------------------------------------------------------------------------

void ShapeMeshCache::Release(const ShapeStatic* _shape)
{
    m_cache.erase(_shape);
}

void ShapeMeshCache::Shutdown()
{
    m_cache.clear();
    DebugTrace("ShapeMeshCache shut down\n");
}
