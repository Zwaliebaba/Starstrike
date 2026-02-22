#ifndef __OPENGL_DIRECTX_INTERNALS_H
#define __OPENGL_DIRECTX_INTERNALS_H

namespace OpenGLD3D {

	struct CustomVertex {
		float x, y, z;
		float nx, ny, nz;
		union
		{
			UINT32 diffuse; // BGRA packed (matches DXGI_FORMAT_B8G8R8A8_UNORM)
			struct
			{
				unsigned char b8,g8,r8,a8;
			};
		};
		float u, v;
		float u2, v2;
	};

	void __stdcall glActiveTextureD3D(int _target);
	void __stdcall glMultiTexCoord2fD3D(int _target, float _x, float _y);

	// Bind constants, textures, samplers, PSO, and viewport for a draw call.
	// Called by both issueDrawCall and display list replay.
	void PrepareDrawState(D3D_PRIMITIVE_TOPOLOGY topology);
}

#endif // __OPENGL_DIRECTX_INTERNALS_H