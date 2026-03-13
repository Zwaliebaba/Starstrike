#pragma once

#include <DirectXMath.h>
#include <stack>

namespace OpenGLD3D {

	class MatrixStack {
	public:
		MatrixStack();

		void LoadIdentity();
		void Load(const DirectX::XMFLOAT4X4& m);
		void Multiply(const DirectX::XMFLOAT4X4& m);
		void Push();
		void Pop();

		// Convenience methods — combine XMMatrix* construction + Multiply
		void Translate(float x, float y, float z);
		void Scale(float x, float y, float z);
		void RotateAxis(float angleDegrees, float x, float y, float z);

		// Setup methods — replace the current matrix entirely (Load, not Multiply)
		void LookAtRH(float eyeX, float eyeY, float eyeZ,
		              float atX, float atY, float atZ,
		              float upX, float upY, float upZ);
		void PerspectiveFovRH(float fovAngleYDegrees, float aspect, float nearZ, float farZ);
		void OrthoOffCenterRH(float left, float right, float bottom, float top,
		                      float nearZ = -1.0f, float farZ = 1.0f);

		const DirectX::XMFLOAT4X4& GetTop() const { return m_current; }
		DirectX::XMMATRIX GetTopXM() const { return DirectX::XMLoadFloat4x4(&m_current); }

		bool IsDirty() const { return m_dirty; }
		void ClearDirty() { m_dirty = false; }
		void MarkDirty() { m_dirty = true; }

	private:
		DirectX::XMFLOAT4X4 m_current;
		std::stack<DirectX::XMFLOAT4X4> m_stack;
		bool m_dirty = true;
	};

};

