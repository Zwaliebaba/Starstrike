#include "pch.h"

#include "QuadBatcher.h"
#include "opengl_directx.h"
#include "UploadRingBuffer.h"

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

QuadBatcher& QuadBatcher::Get()
{
    static QuadBatcher s_instance;
    return s_instance;
}

// ---------------------------------------------------------------------------
// Emit — decompose quad into 2 triangles
// ---------------------------------------------------------------------------

void QuadBatcher::Emit(
    const OpenGLD3D::CustomVertex& _v0, const OpenGLD3D::CustomVertex& _v1,
    const OpenGLD3D::CustomVertex& _v2, const OpenGLD3D::CustomVertex& _v3)
{
    // Triangle 1: v0, v1, v2
    // Triangle 2: v0, v2, v3
    // Matches glVertex3f_impl's GL_QUADS decomposition.
    m_vertices.push_back(_v0);
    m_vertices.push_back(_v1);
    m_vertices.push_back(_v2);
    m_vertices.push_back(_v0);
    m_vertices.push_back(_v2);
    m_vertices.push_back(_v3);
}

// ---------------------------------------------------------------------------
// Flush — upload accumulated vertices and issue a single draw call
// ---------------------------------------------------------------------------

void QuadBatcher::Flush()
{
    if (m_vertices.empty())
        return;

    const UINT vertexCount = static_cast<UINT>(m_vertices.size());
    const UINT vbSize = vertexCount * sizeof(OpenGLD3D::CustomVertex);

    // Upload vertices to ring buffer
    auto alloc = Graphics::GetCurrentUploadBuffer().Allocate(vbSize, sizeof(float));
    memcpy(alloc.cpuPtr, m_vertices.data(), vbSize);

    D3D12_VERTEX_BUFFER_VIEW vbView{};
    vbView.BufferLocation = alloc.gpuAddr;
    vbView.SizeInBytes = vbSize;
    vbView.StrideInBytes = sizeof(OpenGLD3D::CustomVertex);

    // PrepareDrawState reads current GL state for PSO, CB, textures, viewport.
    OpenGLD3D::PrepareDrawState(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    auto* cmdList = Graphics::Core::Get().GetCommandList();
    cmdList->IASetVertexBuffers(0, 1, &vbView);
    cmdList->DrawInstanced(vertexCount, 1, 0, 0);

    // Track stats (CB upload bytes are already counted inside PrepareDrawState)
    OpenGLD3D::RecordBatchedDraw(vbSize);

    m_vertices.clear();
}
