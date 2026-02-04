#pragma once

#undef LoadString

class PersistentState
{
  public:
    static void Initialize(_In_ const Windows::Foundation::Collections::IPropertySet& _settingsValues,
                                       _In_ const hstring& key);

    static void SaveBool(const hstring& _key, bool _value);
    static void SaveInt32(const hstring& _key, int value);
    static void SaveFloat(const hstring& key, float value);
    static void XM_CALLCONV SaveVector3(const hstring& _key, XMVECTOR value);
    static void SaveString(const hstring& _key, const hstring& string);

    [[nodiscard]] static bool LoadBool(const hstring& _key, bool defaultValue);
    [[nodiscard]] static int LoadInt32(const hstring& key, int defaultValue);
    [[nodiscard]] static float LoadFloat(const hstring& key, float defaultValue);
    [[nodiscard]] static XMVECTOR XM_CALLCONV LoadVector3(const hstring& key, XMVECTOR defaultValue);
    [[nodiscard]] static hstring LoadString(const hstring& key, const hstring& defaultValue);

  private:
    inline static hstring m_keyName;
    inline static Windows::Foundation::Collections::IPropertySet m_settingsValues;
};
