#include "pch.h"
#include "opengl_directx_dlist.h"
#include "opengl_directx_internals.h"
#include "opengl_directx_matrix_stack.h"
#include "d3d12_backend.h"

using namespace OpenGLD3D;

DisplayList::DisplayList(std::vector<std::unique_ptr<DLCommand>>&& commands,
						 std::vector<CustomVertex>&& vertices)
	: m_commands(std::move(commands))
{
	m_vbView = {};

	if (!vertices.empty())
	{
		auto* device = g_backend.GetDevice();
		UINT vbSize = static_cast<UINT>(vertices.size() * sizeof(CustomVertex));

		// Create a default-heap vertex buffer
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = vbSize;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_COMMON, nullptr,
			IID_GRAPHICS_PPV_ARGS(m_vertexBuffer));

		// Upload via the ring buffer — note: this must happen while a command list is open
		auto alloc = g_backend.GetUploadBuffer().Allocate(vbSize, sizeof(CustomVertex));
		memcpy(alloc.cpuPtr, vertices.data(), vbSize);

		auto* cmdList = g_backend.GetCommandList();

		// Transition to copy dest
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = m_vertexBuffer.get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		cmdList->ResourceBarrier(1, &barrier);

		cmdList->CopyBufferRegion(m_vertexBuffer.get(), 0, g_backend.GetUploadBuffer().GetResource(), alloc.offset, vbSize);

		// Transition to vertex buffer
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		cmdList->ResourceBarrier(1, &barrier);

		m_vbView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vbView.SizeInBytes = vbSize;
		m_vbView.StrideInBytes = sizeof(CustomVertex);
	}
}

DisplayList::~DisplayList()
{
}

void DisplayList::Draw()
{
	if (m_vertexBuffer)
	{
		auto* cmdList = g_backend.GetCommandList();
		cmdList->IASetVertexBuffers(0, 1, &m_vbView);
	}

	for (auto& cmd : m_commands)
		cmd->Execute();
}

// --- Commands ---

DLCommandDraw::DLCommandDraw(D3D_PRIMITIVE_TOPOLOGY topology, UINT startVertex, UINT vertexCount)
	: m_topology(topology), m_startVertex(startVertex), m_vertexCount(vertexCount)
{
}

void DLCommandDraw::Execute()
{
	PrepareDrawState(m_topology);

	auto* cmdList = g_backend.GetCommandList();
	cmdList->DrawInstanced(m_vertexCount, 1, m_startVertex, 0);
}

DLCommandLoadMatrix::DLCommandLoadMatrix(MatrixStack* stack, const DirectX::XMFLOAT4X4& matrix)
	: m_stack(stack), m_matrix(matrix)
{
}

void DLCommandLoadMatrix::Execute()
{
	m_stack->Load(m_matrix);
}

DLCommandMultiplyMatrix::DLCommandMultiplyMatrix(MatrixStack* stack, const DirectX::XMFLOAT4X4& matrix)
	: m_stack(stack), m_matrix(matrix)
{
}

void DLCommandMultiplyMatrix::Execute()
{
	m_stack->Multiply(m_matrix);
}

DLCommandPushMatrix::DLCommandPushMatrix(MatrixStack* stack)
	: m_stack(stack)
{
}

void DLCommandPushMatrix::Execute()
{
	m_stack->Push();
}

DLCommandPopMatrix::DLCommandPopMatrix(MatrixStack* stack)
	: m_stack(stack)
{
}

void DLCommandPopMatrix::Execute()
{
	m_stack->Pop();
}
