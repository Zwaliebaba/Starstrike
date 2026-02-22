#include "pch.h"
#include "ogl_extensions.h"
#include "opengl_directx_internals.h"
#include "d3d12_backend.h"

#include <map>

MultiTexCoord2fARB gglMultiTexCoord2fARB = NULL;
ActiveTextureARB gglActiveTextureARB = NULL;

glBindBufferARB				gglBindBufferARB = NULL;
glDeleteBuffersARB			gglDeleteBuffersARB = NULL;
glGenBuffersARB				gglGenBuffersARB = NULL;
glIsBufferARB				gglIsBufferARB = NULL;
glBufferDataARB				gglBufferDataARB = NULL;
glBufferSubDataARB			gglBufferSubDataARB = NULL;
glGetBufferSubDataARB		gglGetBufferSubDataARB = NULL;
glMapBufferARB				gglMapBufferARB = NULL;
glUnmapBufferARB			gglUnmapBufferARB = NULL;
glGetBufferParameterivARB	gglGetBufferParameterivARB = NULL;
glGetBufferPointervARB		gglGetBufferPointervARB = NULL;

ChoosePixelFormatARB gglChoosePixelFormatARB = NULL;

using namespace OpenGLD3D;

struct VBOEntry
{
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_VERTEX_BUFFER_VIEW vbView;
	UINT size;
};

static std::map<unsigned, VBOEntry> s_vertexBuffers;
static unsigned s_lastVertexBufferId = 0;
static VBOEntry* s_currentVBO = nullptr;

void __stdcall glGenBuffersD3D (GLsizei _howmany, GLuint *_buffers)
{
	for (unsigned i = 0; i < static_cast<unsigned>(_howmany); i++)
		_buffers[i] = ++s_lastVertexBufferId;
}

void __stdcall glDeleteBuffersD3D (GLsizei _howmany, const GLuint *_buffers)
{
	for (unsigned i = 0; i < static_cast<unsigned>(_howmany); i++) {
		unsigned id = _buffers[i];
		s_vertexBuffers.erase(id);
	}
}

void __stdcall glBindBufferD3D (GLenum _target, GLuint _bufferId)
{
	DEBUG_ASSERT( _target == GL_ARRAY_BUFFER_ARB );

	if (_bufferId > 0)
	{
		auto it = s_vertexBuffers.find(_bufferId);
		if (it == s_vertexBuffers.end())
		{
			s_vertexBuffers[_bufferId] = VBOEntry{};
			s_currentVBO = &s_vertexBuffers[_bufferId];
		}
		else
		{
			s_currentVBO = &it->second;
		}
	}
	else
	{
		s_currentVBO = nullptr;
	}
}

void __stdcall glBufferDataD3D (GLenum _target, GLsizeiptrARB _size, const GLvoid *_data, GLenum _usage)
{
	DEBUG_ASSERT( _target == GL_ARRAY_BUFFER_ARB );
	DEBUG_ASSERT( s_currentVBO != nullptr );

	auto* device = g_backend.GetDevice();
	auto* cmdList = g_backend.GetCommandList();

	// Release old buffer
	s_currentVBO->resource.Reset();

	// Create a default-heap vertex buffer
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = _size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_COMMON, nullptr,
		IID_PPV_ARGS(&s_currentVBO->resource));
	DEBUG_ASSERT(SUCCEEDED(hr));

	// Upload via ring buffer
	auto alloc = g_backend.GetUploadBuffer().Allocate(static_cast<SIZE_T>(_size), sizeof(float));
	memcpy(alloc.cpuPtr, _data, _size);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = s_currentVBO->resource.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(1, &barrier);

	cmdList->CopyBufferRegion(s_currentVBO->resource.Get(), 0,
		g_backend.GetUploadBuffer().GetResource(), alloc.offset, _size);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	cmdList->ResourceBarrier(1, &barrier);

	s_currentVBO->vbView.BufferLocation = s_currentVBO->resource->GetGPUVirtualAddress();
	s_currentVBO->vbView.SizeInBytes = static_cast<UINT>(_size);
	s_currentVBO->vbView.StrideInBytes = sizeof(CustomVertex);
	s_currentVBO->size = static_cast<UINT>(_size);
}

void InitialiseOGLExtensions()
{
	gglMultiTexCoord2fARB = glMultiTexCoord2fD3D;
	gglActiveTextureARB = glActiveTextureD3D;

	gglBindBufferARB			= glBindBufferD3D;
	gglDeleteBuffersARB			= glDeleteBuffersD3D;
	gglGenBuffersARB			= glGenBuffersD3D;
	gglIsBufferARB				= 0;
	gglBufferDataARB			= glBufferDataD3D;
	gglBufferSubDataARB			= 0;
	gglGetBufferSubDataARB		= 0;
	gglMapBufferARB				= 0;
	gglUnmapBufferARB			= 0;
	gglGetBufferParameterivARB	= 0;
	gglGetBufferPointervARB		= 0;

	gglChoosePixelFormatARB = 0;
}

int IsOGLExtensionSupported(const char *extension)
{
	return 0;
}
