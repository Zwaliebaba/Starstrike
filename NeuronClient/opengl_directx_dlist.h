#ifndef OPENGL_DIRECTX_DLIST_H
#define OPENGL_DIRECTX_DLIST_H

#include <vector>
#include <memory>
#include "opengl_directx_internals.h"
#include "opengl_directx_matrix_stack.h"
#include <d3d12.h>

namespace OpenGLD3D {

	// --- Display list commands ---

	class DLCommand {
	public:
		virtual ~DLCommand() = default;
		virtual void Execute() = 0;
	};

	class DLCommandDraw : public DLCommand {
	public:
		DLCommandDraw(D3D_PRIMITIVE_TOPOLOGY topology, UINT startVertex, UINT vertexCount);
		void Execute() override;
	private:
		D3D_PRIMITIVE_TOPOLOGY m_topology;
		UINT m_startVertex;
		UINT m_vertexCount;
	};

	class DLCommandLoadMatrix : public DLCommand {
	public:
		DLCommandLoadMatrix(MatrixStack* stack, const DirectX::XMFLOAT4X4& matrix);
		void Execute() override;
	private:
		MatrixStack* m_stack;
		DirectX::XMFLOAT4X4 m_matrix;
	};

	class DLCommandMultiplyMatrix : public DLCommand {
	public:
		DLCommandMultiplyMatrix(MatrixStack* stack, const DirectX::XMFLOAT4X4& matrix);
		void Execute() override;
	private:
		MatrixStack* m_stack;
		DirectX::XMFLOAT4X4 m_matrix;
	};

	class DLCommandPushMatrix : public DLCommand {
	public:
		DLCommandPushMatrix(MatrixStack* stack);
		void Execute() override;
	private:
		MatrixStack* m_stack;
	};

	class DLCommandPopMatrix : public DLCommand {
	public:
		DLCommandPopMatrix(MatrixStack* stack);
		void Execute() override;
	private:
		MatrixStack* m_stack;
	};

	// --- Display list ---

	class DisplayList {
	public:
		DisplayList(std::vector<std::unique_ptr<DLCommand>>&& commands,
					std::vector<CustomVertex>&& vertices);
		~DisplayList();
		void Draw();

	private:
		com_ptr<ID3D12Resource> m_vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vbView;
		std::vector<std::unique_ptr<DLCommand>> m_commands;
	};
}

#endif // OPENGL_DIRECTX_DLIST_H
