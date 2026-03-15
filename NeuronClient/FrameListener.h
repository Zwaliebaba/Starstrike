#pragma once

struct ID3D12GraphicsCommandList;

namespace Neuron::Graphics
{
  class IFrameListener
  {
  public:
    virtual void OnFrameBegin(ID3D12GraphicsCommandList* cmdList) = 0;
    virtual ~IFrameListener() = default;
  };
}
