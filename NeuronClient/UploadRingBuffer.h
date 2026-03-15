#pragma once

namespace Neuron::Graphics
{
  constexpr SIZE_T UPLOAD_BUFFER_SIZE = 64 * 1024 * 1024; // 64 MB ring buffer

  // General-purpose upload ring buffer for per-frame CPU→GPU transfers.
  // One buffer exists per back-buffer frame to avoid GPU data races.
  class UploadRingBuffer
  {
  public:
    void Init(ID3D12Device* device, SIZE_T size);
    void Shutdown();
    void Reset();

    // Allocate space and return CPU pointer + GPU virtual address
    struct Allocation
    {
      void* cpuPtr;
      D3D12_GPU_VIRTUAL_ADDRESS gpuAddr;
      SIZE_T offset;
    };

    Allocation Allocate(SIZE_T sizeBytes, SIZE_T alignment = 4);

    ID3D12Resource* GetResource() const { return m_resource.get(); }
    SIZE_T GetOffset() const { return m_offset; }
    SIZE_T GetHighWaterMark() const { return m_highWaterMark; }
    void ResetHighWaterMark() { m_highWaterMark = 0; }

  private:
    com_ptr<ID3D12Resource> m_resource;
    UINT8* m_cpuBase = nullptr;
    D3D12_GPU_VIRTUAL_ADDRESS m_gpuBase = 0;
    SIZE_T m_size = 0;
    SIZE_T m_offset = 0;
    SIZE_T m_highWaterMark = 0;
  };

  // Returns the upload buffer for the current frame. Hides frame-index bookkeeping.
  UploadRingBuffer& GetCurrentUploadBuffer();

  // Internal — called by Core for lifecycle management.
  void CreateUploadBuffers();
  void ShutdownUploadBuffers();
  void ResetCurrentUploadBuffer();
}
