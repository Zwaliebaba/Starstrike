#ifndef __OPENGL_DIRECTX_MATRIX_STACK
#define __OPENGL_DIRECTX_MATRIX_STACK

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

#endif // __OPENGL_DIRECTX_MATRIX_STACK
