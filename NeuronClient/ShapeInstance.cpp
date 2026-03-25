#include "pch.h"
#include "ShapeInstance.h"
#include "ShapeMeshCache.h"

ShapeInstance::ShapeInstance()
  : m_master(nullptr),
    m_fragmentStates(nullptr) {}

ShapeInstance::ShapeInstance(ShapeStatic* _master)
  : m_master(_master),
    m_fragmentStates(nullptr)
{
  DEBUG_ASSERT(_master);
  int count = _master->m_numFragments;
  m_fragmentStates = new FragmentState[count];
  // Initialize from master's rest-pose defaults
  memcpy(m_fragmentStates, _master->m_defaultStates, sizeof(FragmentState) * count);
}

ShapeInstance::~ShapeInstance()
{
  delete [] m_fragmentStates;
}

ShapeInstance::ShapeInstance(ShapeInstance&& _other) noexcept
  : m_master(_other.m_master),
    m_fragmentStates(_other.m_fragmentStates)
{
  _other.m_master = nullptr;
  _other.m_fragmentStates = nullptr;
}

ShapeInstance& ShapeInstance::operator=(ShapeInstance&& _other) noexcept
{
  if (this != &_other)
  {
    delete [] m_fragmentStates;
    m_master = _other.m_master;
    m_fragmentStates = _other.m_fragmentStates;
    _other.m_master = nullptr;
    _other.m_fragmentStates = nullptr;
  }
  return *this;
}

FragmentState& ShapeInstance::GetState(int _fragmentIndex)
{
  DEBUG_ASSERT(_fragmentIndex >= 0 && _fragmentIndex < m_master->m_numFragments);
  return m_fragmentStates[_fragmentIndex];
}

const FragmentState& ShapeInstance::GetState(int _fragmentIndex) const
{
  DEBUG_ASSERT(_fragmentIndex >= 0 && _fragmentIndex < m_master->m_numFragments);
  return m_fragmentStates[_fragmentIndex];
}

FragmentState& ShapeInstance::GetState(const char* _fragmentName)
{
  int idx = m_master->GetFragmentIndex(_fragmentName);
  DEBUG_ASSERT(idx >= 0);
  return m_fragmentStates[idx];
}

void ShapeInstance::Render(float _predictionTime, const Matrix34& _transform) const
{
  Render(_predictionTime, static_cast<Transform3D>(_transform));
}

void XM_CALLCONV ShapeInstance::Render(float _predictionTime, XMMATRIX _transform) const
{
  // Ensure GPU vertex buffer exists (no-op after first call)
  ShapeMeshCache::Get().EnsureUploaded(m_master);

  glEnable(GL_COLOR_MATERIAL);
  auto& mv = OpenGLD3D::GetModelViewStack();
  mv.Push();
  mv.Multiply(_transform);

  m_master->m_rootFragment->RenderNative(m_fragmentStates, _predictionTime);

  glDisable(GL_COLOR_MATERIAL);
  mv.Pop();
}

bool ShapeInstance::RayHit(RayPackage* _package, const Matrix34& _transform, bool _accurate) const
{
  return m_master->m_rootFragment->RayHit(m_fragmentStates, _package, _transform, _accurate);
}

bool ShapeInstance::SphereHit(SpherePackage* _package, const Matrix34& _transform, bool _accurate) const
{
  return m_master->m_rootFragment->SphereHit(m_fragmentStates, _package, _transform, _accurate);
}

bool ShapeInstance::ShapeHit(ShapeStatic* _shape, const Matrix34& _theTransform, const Matrix34& _ourTransform, bool _accurate) const
{
  return m_master->m_rootFragment->ShapeHit(m_fragmentStates, _shape, _theTransform, _ourTransform, _accurate);
}

Matrix34 ShapeInstance::GetMarkerWorldMatrix(const ShapeMarkerData* _marker, const Matrix34& _rootTransform) const
{
  return _marker->GetWorldMatrix(m_fragmentStates, _rootTransform);
}

Transform3D ShapeInstance::GetMarkerWorldTransform(const ShapeMarkerData* _marker, const Transform3D& _rootTransform) const
{
  return _marker->GetWorldTransform(m_fragmentStates, _rootTransform);
}

LegacyVector3 ShapeInstance::CalculateCenter(const Matrix34& _transform) const
{
  LegacyVector3 center;
  int numFragments = 0;
  m_master->m_rootFragment->CalculateCenter(m_fragmentStates, _transform, center, numFragments);
  center /= static_cast<float>(numFragments);
  return center;
}

float ShapeInstance::CalculateRadius(const Matrix34& _transform, const LegacyVector3& _center) const
{
  float radius = 0.0f;
  m_master->m_rootFragment->CalculateRadius(m_fragmentStates, _transform, _center, radius);
  return radius;
}
