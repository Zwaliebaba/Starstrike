#include "pch.h"
#include "opengl_directx_dlist_dev.h"
#include "opengl_directx_dlist.h"

using namespace OpenGLD3D;

DisplayListRecorder::DisplayListRecorder(unsigned id)
	: m_id(id)
{
}

void DisplayListRecorder::RecordDraw(D3D_PRIMITIVE_TOPOLOGY topology,
									 const CustomVertex* vertices, unsigned vertexCount)
{
	if (vertexCount == 0)
		return;

	UINT startVertex = static_cast<UINT>(m_vertices.size());

	for (unsigned i = 0; i < vertexCount; i++)
		m_vertices.push_back(vertices[i]);

	m_commands.push_back(std::make_unique<DLCommandDraw>(topology, startVertex, vertexCount));
}

void DisplayListRecorder::RecordLoadTransform(MatrixStack* stack, const DirectX::XMFLOAT4X4& mat)
{
	m_commands.push_back(std::make_unique<DLCommandLoadMatrix>(stack, mat));
}

void DisplayListRecorder::RecordMultMatrix(MatrixStack* stack, const DirectX::XMFLOAT4X4& mat)
{
	m_commands.push_back(std::make_unique<DLCommandMultiplyMatrix>(stack, mat));
}

void DisplayListRecorder::RecordPushMatrix(MatrixStack* stack)
{
	m_commands.push_back(std::make_unique<DLCommandPushMatrix>(stack));
}

void DisplayListRecorder::RecordPopMatrix(MatrixStack* stack)
{
	m_commands.push_back(std::make_unique<DLCommandPopMatrix>(stack));
}

DisplayList* DisplayListRecorder::Compile()
{
	return new DisplayList(std::move(m_commands), std::move(m_vertices));
}

