#include "pch.h"
#include "UploadRingBuffer.h"

using namespace Neuron::Graphics;

// ---- UploadRingBuffer ----

void UploadRingBuffer::Init(ID3D12Device* device, SIZE_T size)
{
  m_size = size;
  m_offset = 0;

  D3D12_HEAP_PROPERTIES heapProps = {};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Width = size;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  check_hresult(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                IID_GRAPHICS_PPV_ARGS(m_resource)));

  D3D12_RANGE readRange = {0, 0};
  check_hresult(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&m_cpuBase)));

  m_gpuBase = m_resource->GetGPUVirtualAddress();
}

void UploadRingBuffer::Shutdown()
{
  if (m_resource)
  {
    m_resource->Unmap(0, nullptr);
    m_resource = nullptr;
  }
  m_cpuBase = nullptr;
  m_gpuBase = 0;
  m_size = 0;
  m_offset = 0;
}

void UploadRingBuffer::Reset() { m_offset = 0; }

UploadRingBuffer::Allocation UploadRingBuffer::Allocate(SIZE_T sizeBytes, SIZE_T alignment)
{
  // Align offset
  m_offset = (m_offset + alignment - 1) & ~(alignment - 1);

  // Wrap if we've exceeded the buffer.
  // This overwrites data from earlier draw calls in the same frame that the
  // GPU has not yet consumed (the command list is deferred).  The GPU will
  // read garbage, causing spiky/flickering geometry.  Trigger a debug assert
  // so the developer knows the buffer is too small, then wrap anyway to
  // avoid a hard crash in release builds.
  if (m_offset + sizeBytes > m_size)
  {
    DEBUG_ASSERT(false && "UploadRingBuffer wrapped mid-frame — increase UPLOAD_BUFFER_SIZE");
    m_offset = 0;
  }

  Allocation alloc;
  alloc.cpuPtr = m_cpuBase + m_offset;
  alloc.gpuAddr = m_gpuBase + m_offset;
  alloc.offset = m_offset;

  m_offset += sizeBytes;
  if (m_offset > m_highWaterMark)
    m_highWaterMark = m_offset;
  return alloc;
}

// ---- Static storage for per-frame upload buffers ----

static std::vector<UploadRingBuffer> s_uploadBuffers;

void Neuron::Graphics::CreateUploadBuffers()
{
  auto* device = Core::Get().GetD3DDevice();
  const UINT bufferCount = Core::Get().GetBackBufferCount();
  s_uploadBuffers.resize(bufferCount);
  for (UINT i = 0; i < bufferCount; i++)
    s_uploadBuffers[i].Init(device, UPLOAD_BUFFER_SIZE);
}

void Neuron::Graphics::ShutdownUploadBuffers()
{
  for (auto& buffer : s_uploadBuffers)
    buffer.Shutdown();
  s_uploadBuffers.clear();
}

void Neuron::Graphics::ResetCurrentUploadBuffer()
{
  s_uploadBuffers[Core::Get().GetCurrentFrameIndex()].Reset();
}

UploadRingBuffer& Neuron::Graphics::GetCurrentUploadBuffer()
{
  return s_uploadBuffers[Core::Get().GetCurrentFrameIndex()];
}
