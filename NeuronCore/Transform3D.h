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

    [[nodiscard]] static Transform3D XM_CALLCONV FromAxes(DirectX::FXMVECTOR _right, DirectX::FXMVECTOR _up, DirectX::FXMVECTOR _forward,
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
    [[nodiscard]] static Transform3D FromXMFLOAT4X4(const DirectX::XMFLOAT4X4& _m) noexcept { return Transform3D{_m}; }

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

    // Float3 accessors — convenient for bridging to legacy types without SIMD.
    [[nodiscard]] DirectX::XMFLOAT3 RightF3() const noexcept { return {m._11, m._12, m._13}; }
    [[nodiscard]] DirectX::XMFLOAT3 UpF3() const noexcept { return {m._21, m._22, m._23}; }
    [[nodiscard]] DirectX::XMFLOAT3 ForwardF3() const noexcept { return {m._31, m._32, m._33}; }
    [[nodiscard]] DirectX::XMFLOAT3 PositionF3() const noexcept { return {m._41, m._42, m._43}; }

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
      DirectX::XMStoreFloat4x4(&result.m, DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&m), DirectX::XMLoadFloat4x4(&_rhs.m)));
      return result;
    }

    Transform3D& operator*=(const Transform3D& _rhs) noexcept
    {
      DirectX::XMStoreFloat4x4(&m, DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&m), DirectX::XMLoadFloat4x4(&_rhs.m)));
      return *this;
    }

    /// Orthonormalize the 3×3 rotation part in-place.
    /// Matches the legacy Matrix34::Normalise() convention: forward is
    /// normalised first, then right = cross(up, forward), up = cross(forward, right).
    void Orthonormalize() noexcept
    {
      using namespace DirectX;
      XMVECTOR f = XMVector3Normalize(Forward());
      XMVECTOR r = XMVector3Normalize(XMVector3Cross(Up(), f));
      XMVECTOR u = XMVector3Cross(f, r);
      XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&m._11), XMVectorSetW(r, 0.0f));
      XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&m._21), XMVectorSetW(u, 0.0f));
      XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&m._31), XMVectorSetW(f, 0.0f));
    }

    /// Rotate the orientation (rows 0-2) around an axis whose direction is the
    /// normalised _axis and whose magnitude is the rotation angle in radians.
    /// Position (row 3) is unchanged.  Matches Matrix34::RotateAround(LegacyVector3).
    Transform3D& XM_CALLCONV RotateAround(DirectX::FXMVECTOR _axis) noexcept
    {
      using namespace DirectX;
      float magSq = XMVectorGetX(XMVector3LengthSq(_axis));
      if (magSq < 1e-10f)
        return *this;
      float angle = sqrtf(magSq);
      XMVECTOR norm = XMVectorScale(_axis, 1.0f / angle);
      XMMATRIX rot = XMMatrixRotationAxis(norm, angle);
      XMMATRIX mat = XMLoadFloat4x4(&m);
      XMVECTOR savedPos = mat.r[3];
      mat = XMMatrixMultiply(mat, rot); // Post-multiply: each row rotated by R
      mat.r[3] = savedPos; // Restore position (rotation is orientation-only)
      XMStoreFloat4x4(&m, mat);
      return *this;
    }

    /// Add an offset to the position row (row 3).
    void XM_CALLCONV Translate(DirectX::FXMVECTOR _offset) noexcept
    {
      using namespace DirectX;
      XMVECTOR pos = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(&m._41));
      pos = XMVectorAdd(pos, _offset);
      XMStoreFloat4(reinterpret_cast<XMFLOAT4*>(&m._41), pos);
    }

    /// Approximate identity comparison (matches DirectXMath tolerance).
    [[nodiscard]] bool IsIdentity() const noexcept { return DirectX::XMMatrixIsIdentity(DirectX::XMLoadFloat4x4(&m)); }

    // --- Conversion -------------------------------------------------------------

    [[nodiscard]] const DirectX::XMFLOAT4X4& AsXMFLOAT4X4() const noexcept { return m; }
    [[nodiscard]] DirectX::XMMATRIX AsXMMATRIX() const noexcept { return DirectX::XMLoadFloat4x4(&m); }
    operator DirectX::XMMATRIX() const noexcept { return DirectX::XMLoadFloat4x4(&m); }
  };
} // namespace Neuron
