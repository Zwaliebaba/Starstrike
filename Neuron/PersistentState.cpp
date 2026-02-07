#include "pch.h"
#include "PersistentState.h"

using namespace Windows::Foundation;
using namespace Collections;

void PersistentState::Initialize(_In_ const IPropertySet& _settingsValues, _In_ const hstring& key)
{
  m_settingsValues = _settingsValues;
  m_keyName = key;
}

void PersistentState::SaveBool(const hstring& _key, bool _value) { m_settingsValues.Insert(m_keyName + _key, box_value(_value)); }

void PersistentState::SaveInt32(const hstring& _key, int value) { m_settingsValues.Insert(m_keyName + _key, box_value(value)); }

void PersistentState::SaveFloat(const hstring& key, float value) { m_settingsValues.Insert(m_keyName + key, box_value(value)); }

void XM_CALLCONV PersistentState::SaveVector3(const hstring& _key, const XMVECTOR value)
{
  m_settingsValues.Insert(m_keyName + _key, PropertyValue::CreateSingleArray({
    XMVectorGetX(value), XMVectorGetY(value), XMVectorGetZ(value)}));
}

void PersistentState::SaveString(const hstring& _key, const hstring& string)
{
  m_settingsValues.Insert(m_keyName + _key, box_value(string));
}

bool PersistentState::LoadBool(const hstring& _key, bool defaultValue)
{
  return unbox_value_or(m_settingsValues.TryLookup(m_keyName + _key), defaultValue);
}

int PersistentState::LoadInt32(const hstring& key, int defaultValue)
{
  return unbox_value_or(m_settingsValues.TryLookup(m_keyName + key), defaultValue);
}

float PersistentState::LoadFloat(const hstring& key, float defaultValue)
{
  return unbox_value_or(m_settingsValues.TryLookup(m_keyName + key), defaultValue);
}

XMVECTOR XM_CALLCONV PersistentState::LoadVector3(const hstring& key, XMVECTOR defaultValue)
{
  auto propertyValue = unbox_value_or(m_settingsValues.TryLookup(m_keyName + key), IPropertyValue {});
  if (propertyValue != nullptr)
  {
    com_array<float> array3;
    propertyValue.GetSingleArray(array3);
    return XMVectorSet(array3[0], array3[1], array3[2], 0.0f);
  }
  return defaultValue;
}

hstring PersistentState::LoadString(const hstring& key, const hstring& defaultValue)
{
  return winrt::unbox_value_or<hstring>(m_settingsValues.TryLookup(m_keyName + key), defaultValue);
}
