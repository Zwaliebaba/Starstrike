#include "pch.h"
#include "opengl_directx_matrix_stack.h"

using namespace DirectX;
using namespace OpenGLD3D;

MatrixStack::MatrixStack()
	: m_dirty(true)
{
	XMStoreFloat4x4(&m_current, XMMatrixIdentity());
}

void MatrixStack::LoadIdentity()
{
	XMStoreFloat4x4(&m_current, XMMatrixIdentity());
	m_dirty = true;
}

void MatrixStack::Load(const XMFLOAT4X4& m)
{
	m_current = m;
	m_dirty = true;
}

void MatrixStack::Multiply(const XMFLOAT4X4& m)
{
	// OpenGL post-multiplies: Result = Current * M
	// But D3D9 code used MultMatrixLocal which is pre-multiply: Result = M * Current
	// We preserve the same semantic: local pre-multiply
	XMMATRIX current = XMLoadFloat4x4(&m_current);
	XMMATRIX mat = XMLoadFloat4x4(&m);
	XMStoreFloat4x4(&m_current, XMMatrixMultiply(mat, current));
	m_dirty = true;
}

void MatrixStack::Push()
{
	m_stack.push(m_current);
}

void MatrixStack::Pop()
{
	DEBUG_ASSERT(!m_stack.empty());
	m_current = m_stack.top();
	m_stack.pop();
	m_dirty = true;
}
