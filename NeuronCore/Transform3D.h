#pragma once

#include <DirectXMath.h>

namespace Neuron
{
  /// Row-major affine transform wrapping XMFLOAT4X4.
  /// Row 0 = right, row 1 = up, row 2 = forward, row 3 = [pos, 1].
  /// Convention: row-vector multiply — v * M transforms a point from local to parent space.
  struct Transform3D
  {
    DirectX::XMFLOAT4X4 m;

    // --- Construction -----------------------------------------------------------

    [[nodiscard]] static Transform3D Identity() noexcept
    {
      Transform3D t;
      DirectX::XMStoreFloat4x4(&t.m, DirectX::XMMatrixIdentity());
      return t;
    }

    [[nodiscard]] static Transform3D XM_CALLCONV FromAxes(
      DirectX::FXMVECTOR _right,
      DirectX::FXMVECTOR _up,
      DirectX::FXMVECTOR _forward,
      DirectX::GXMVECTOR _pos) noexcept
    {
      Transform3D t;
      // Build row-major: rows = right, up, forward, pos.
      // XMMatrixSet constructs row-by-row.
      DirectX::XMMATRIX mat;
      mat.r[0] = DirectX::XMVectorSetW(_right, 0.0f);
      mat.r[1] = DirectX::XMVectorSetW(_up, 0.0f);
      mat.r[2] = DirectX::XMVectorSetW(_forward, 0.0f);
      mat.r[3] = DirectX::XMVectorSetW(_pos, 1.0f);
      DirectX::XMStoreFloat4x4(&t.m, mat);
      return t;
    }

    /// Construct from a raw 4×4 float array (must already be row-major).
    [[nodiscard]] static Transform3D FromXMFLOAT4X4(const DirectX::XMFLOAT4X4& _m) noexcept
    {
      return Transform3D{_m};
    }

    // --- Access -----------------------------------------------------------------

    [[nodiscard]] DirectX::XMVECTOR Right() const noexcept
    {
      return DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(&m._11));
    }

    [[nodiscard]] DirectX::XMVECTOR Up() const noexcept
    {
      return DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(&m._21));
    }

    [[nodiscard]] DirectX::XMVECTOR Forward() const noexcept
    {
      return DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(&m._31));
    }

    [[nodiscard]] DirectX::XMVECTOR Position() const noexcept
    {
      return DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(&m._41));
    }

    // --- Operations -------------------------------------------------------------

    /// Forward transform: v * M (row-vector convention, includes translation).
    [[nodiscard]] DirectX::XMVECTOR XM_CALLCONV TransformPoint(DirectX::FXMVECTOR _point) const noexcept
    {
      return DirectX::XMVector3Transform(_point, DirectX::XMLoadFloat4x4(&m));
    }

    /// Forward transform for direction/normal: v * M (row-vector, ignores translation).
    [[nodiscard]] DirectX::XMVECTOR XM_CALLCONV TransformNormal(DirectX::FXMVECTOR _normal) const noexcept
    {
      return DirectX::XMVector3TransformNormal(_normal, DirectX::XMLoadFloat4x4(&m));
    }

    /// Composition: this * rhs.  For row-vector convention, applying `this` first
    /// then `rhs` is equivalent to v * this * rhs.
    [[nodiscard]] Transform3D operator*(const Transform3D& _rhs) const noexcept
    {
      Transform3D result;
      DirectX::XMStoreFloat4x4(&result.m,
        DirectX::XMMatrixMultiply(
          DirectX::XMLoadFloat4x4(&m),
          DirectX::XMLoadFloat4x4(&_rhs.m)));
      return result;
    }

    Transform3D& operator*=(const Transform3D& _rhs) noexcept
    {
      DirectX::XMStoreFloat4x4(&m,
        DirectX::XMMatrixMultiply(
          DirectX::XMLoadFloat4x4(&m),
          DirectX::XMLoadFloat4x4(&_rhs.m)));
      return *this;
    }

    // --- Conversion -------------------------------------------------------------

    [[nodiscard]] const DirectX::XMFLOAT4X4& AsXMFLOAT4X4() const noexcept { return m; }
    [[nodiscard]] DirectX::XMMATRIX AsXMMATRIX() const noexcept { return DirectX::XMLoadFloat4x4(&m); }
  };

} // namespace Neuron
