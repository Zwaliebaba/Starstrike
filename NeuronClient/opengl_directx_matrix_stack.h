#pragma once

namespace Neuron
{
  struct Transform3D;
}

namespace OpenGLD3D
{
  class MatrixStack
  {
    public:
      MatrixStack();

      void LoadIdentity();
      void Load(const XMFLOAT4X4& m);
      void XM_CALLCONV Multiply(XMMATRIX m);
      void Push();
      void Pop();

      // Convenience methods — combine XMMatrix* construction + Multiply
      void Translate(float x, float y, float z);
      void Scale(float x, float y, float z);
      void RotateAxis(float angleDegrees, float x, float y, float z);

      // Setup methods — replace the current matrix entirely (Load, not Multiply)
      void LookAtRH(float eyeX, float eyeY, float eyeZ, float atX, float atY, float atZ, float upX, float upY, float upZ);
      void PerspectiveFovRH(float fovAngleYDegrees, float aspect, float nearZ, float farZ);
      void OrthoOffCenterRH(float left, float right, float bottom, float top, float nearZ = -1.0f, float farZ = 1.0f);

      const XMFLOAT4X4& GetTop() const { return m_current; }
      XMMATRIX GetTopXM() const { return XMLoadFloat4x4(&m_current); }

      bool IsDirty() const { return m_dirty; }
      void ClearDirty() { m_dirty = false; }
      void MarkDirty() { m_dirty = true; }

    private:
      XMFLOAT4X4 m_current;
      std::stack<XMFLOAT4X4> m_stack;
      bool m_dirty = true;
  };
};
