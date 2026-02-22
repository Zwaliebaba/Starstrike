#ifndef OPENGL_DIRECTX_DLIST_DEV_H
#define OPENGL_DIRECTX_DLIST_DEV_H

#include "opengl_directx_internals.h"
#include "opengl_directx_dlist.h"

#include <vector>
#include <memory>

namespace OpenGLD3D {

	class DisplayList;
	class MatrixStack;

	// Records GL calls during glNewList/glEndList and produces a DisplayList.
	class DisplayListRecorder {
	public:
		DisplayListRecorder(unsigned id);

		unsigned GetId() const { return m_id; }

		// Record a draw
		void RecordDraw(D3D_PRIMITIVE_TOPOLOGY topology,
						const CustomVertex* vertices, unsigned vertexCount);

		// Record matrix operations
		void RecordLoadTransform(MatrixStack* stack, const DirectX::XMFLOAT4X4& mat);
		void RecordMultMatrix(MatrixStack* stack, const DirectX::XMFLOAT4X4& mat);
		void RecordPushMatrix(MatrixStack* stack);
		void RecordPopMatrix(MatrixStack* stack);

		// Compile into a DisplayList
		DisplayList* Compile();

	private:
		unsigned m_id;
		std::vector<std::unique_ptr<DLCommand>> m_commands;
		std::vector<CustomVertex> m_vertices;
	};
}

#endif // OPENGL_DIRECTX_DLIST_DEV_H
