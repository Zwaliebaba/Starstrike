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

void MatrixStack::Translate(float x, float y, float z)
{
	XMFLOAT4X4 mat;
	XMStoreFloat4x4(&mat, XMMatrixTranslation(x, y, z));
	Multiply(mat);
}

void MatrixStack::Scale(float x, float y, float z)
{
	XMFLOAT4X4 mat;
	XMStoreFloat4x4(&mat, XMMatrixScaling(x, y, z));
	Multiply(mat);
}

void MatrixStack::RotateAxis(float angleDegrees, float x, float y, float z)
{
	XMVECTOR axis = XMVectorSet(x, y, z, 0);
	XMFLOAT4X4 mat;
	XMStoreFloat4x4(&mat, XMMatrixRotationAxis(axis, XMConvertToRadians(angleDegrees)));
	Multiply(mat);
}

void MatrixStack::LookAtRH(float eyeX, float eyeY, float eyeZ,
                            float atX, float atY, float atZ,
                            float upX, float upY, float upZ)
{
	XMVECTOR eye = XMVectorSet(eyeX, eyeY, eyeZ, 0);
	XMVECTOR at = XMVectorSet(atX, atY, atZ, 0);
	XMVECTOR up = XMVectorSet(upX, upY, upZ, 0);
	XMFLOAT4X4 mat;
	XMStoreFloat4x4(&mat, XMMatrixLookAtRH(eye, at, up));
	Load(mat);
}

void MatrixStack::PerspectiveFovRH(float fovAngleYDegrees, float aspect, float nearZ, float farZ)
{
	XMFLOAT4X4 mat;
	XMStoreFloat4x4(&mat, XMMatrixPerspectiveFovRH(XMConvertToRadians(fovAngleYDegrees), aspect, nearZ, farZ));
	Load(mat);
}

void MatrixStack::OrthoOffCenterRH(float left, float right, float bottom, float top, float nearZ, float farZ)
{
	XMFLOAT4X4 mat;
	XMStoreFloat4x4(&mat, XMMatrixOrthographicOffCenterRH(left, right, bottom, top, nearZ, farZ));
	Load(mat);
}
