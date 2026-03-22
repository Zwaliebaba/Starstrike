#pragma once

#include "GameVector3.h"
#include <DirectXMath.h>
#include <cmath>

// ---------------------------------------------------------------------------
// GameMatrix — pure storage type backed by XMFLOAT4X4.
//
// Row-major layout matching DirectXMath and the legacy Matrix34 convention:
//   Row 0 = right,  Row 1 = up,  Row 2 = forward,  Row 3 = [pos, 1].
//
// Replaces both Matrix33 and Matrix34.  A rotation-only matrix is simply a
// GameMatrix with translation zeroed.
//
// For math operations, load into XMMATRIX via Load() and use Neuron::Math
// free functions or native DirectXMath.  No arithmetic operators.
// ---------------------------------------------------------------------------

namespace Neuron::Math
{
  struct GameMatrix
  {
    DirectX::XMFLOAT4X4 m;

    // --- Construction -------------------------------------------------------

    /// Default-constructs to zero matrix.  Use Identity() for an identity matrix.
    GameMatrix() noexcept : m() {}

    /// Construct from XMMATRIX (store).
    GameMatrix(DirectX::FXMMATRIX _m) noexcept
    {
      DirectX::XMStoreFloat4x4(&m, _m);
    }

    /// Construct from raw XMFLOAT4X4 (explicit to avoid accidental conversions).
    explicit GameMatrix(const DirectX::XMFLOAT4X4& _m) noexcept : m(_m) {}

    /// Construct from front, up, pos — matches legacy Matrix34(front, up, pos).
    /// Computes right = Cross(up, front) normalised, recomputes up = Cross(front, right) normalised.
    GameMatrix(const GameVector3& _front, const GameVector3& _up, const GameVector3& _pos) noexcept
    {
      using namespace DirectX;
      XMVECTOR f = XMLoadFloat3(&_front.v);
      XMVECTOR u = XMLoadFloat3(&_up.v);
      XMVECTOR r = XMVector3Normalize(XMVector3Cross(u, f));
      u = XMVector3Normalize(XMVector3Cross(f, r));

      m._11 = XMVectorGetX(r); m._12 = XMVectorGetY(r); m._13 = XMVectorGetZ(r); m._14 = 0.0f;
      m._21 = XMVectorGetX(u); m._22 = XMVectorGetY(u); m._23 = XMVectorGetZ(u); m._24 = 0.0f;
      m._31 = XMVectorGetX(f); m._32 = XMVectorGetY(f); m._33 = XMVectorGetZ(f); m._34 = 0.0f;
      m._41 = _pos.x;          m._42 = _pos.y;          m._43 = _pos.z;          m._44 = 1.0f;
    }

    // --- Factories ----------------------------------------------------------

    [[nodiscard]] static GameMatrix Identity() noexcept
    {
      GameMatrix gm;
      DirectX::XMStoreFloat4x4(&gm.m, DirectX::XMMatrixIdentity());
      return gm;
    }

    // --- Load / Store -------------------------------------------------------

    [[nodiscard]] DirectX::XMMATRIX Load() const noexcept
    {
      return DirectX::XMLoadFloat4x4(&m);
    }

    // --- Row accessors (return GameVector3 for storage-level use) -----------

    [[nodiscard]] GameVector3 GetRight()       const noexcept { return GameVector3(m._11, m._12, m._13); }
    [[nodiscard]] GameVector3 GetUp()          const noexcept { return GameVector3(m._21, m._22, m._23); }
    [[nodiscard]] GameVector3 GetForward()     const noexcept { return GameVector3(m._31, m._32, m._33); }
    [[nodiscard]] GameVector3 GetTranslation() const noexcept { return GameVector3(m._41, m._42, m._43); }

    // --- Row mutators -------------------------------------------------------

    void SetRight(const GameVector3& _v) noexcept       { m._11 = _v.x; m._12 = _v.y; m._13 = _v.z; }
    void SetUp(const GameVector3& _v) noexcept          { m._21 = _v.x; m._22 = _v.y; m._23 = _v.z; }
    void SetForward(const GameVector3& _v) noexcept     { m._31 = _v.x; m._32 = _v.y; m._33 = _v.z; }
    void SetTranslation(const GameVector3& _v) noexcept { m._41 = _v.x; m._42 = _v.y; m._43 = _v.z; }

    // --- Comparison (per-element approximate, matches legacy Matrix34) ------

    [[nodiscard]] bool operator==(const GameMatrix& _b) const noexcept
    {
      const float* a = reinterpret_cast<const float*>(&m);
      const float* b = reinterpret_cast<const float*>(&_b.m);
      for (int i = 0; i < 16; ++i)
      {
        if (std::fabsf(a[i] - b[i]) >= 1e-6f)
          return false;
      }
      return true;
    }

    [[nodiscard]] bool operator!=(const GameMatrix& _b) const noexcept
    {
      return !(*this == _b);
    }
  };
}
