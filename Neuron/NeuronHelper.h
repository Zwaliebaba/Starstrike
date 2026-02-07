#pragma once

#include <type_traits>

namespace Neuron
{
#define ENUM_HELPER(T, S, E)                                                                                                                         \
   T inline operator ++(T& _value) noexcept { return _value = static_cast<T>(static_cast<std::underlying_type_t<T>>(_value) + 1); }           \
   T inline operator ++(T& _value, int) noexcept { return _value = static_cast<T>(static_cast<std::underlying_type_t<T>>(_value) + 1); }      \
   T inline operator --(T& _value) noexcept { return _value = static_cast<T>(static_cast<std::underlying_type_t<T>>(_value) - 1); }           \
   T inline operator --(T& _value, int) noexcept { return _value = static_cast<T>(static_cast<std::underlying_type_t<T>>(_value) - 1); }      \
   inline T operator*(T _type) noexcept { return _type; }                                                                                            \
   constexpr size_t SizeOf##T() noexcept { return static_cast<size_t>(T::E) + 1; }                                                                   \
  namespace std {                                                                                                                                    \
    constexpr T begin([[maybe_unused]] T _type) noexcept { return T::S; }                                                                            \
    constexpr T end([[maybe_unused]] T _type) noexcept { return static_cast<T>(SizeOf##T()); }                                                       \
  }                                                                                                                                                  \
   constexpr T operator | (T a, T b) noexcept { return T(((_ENUM_FLAG_SIZED_INTEGER<T>::type)a) | ((_ENUM_FLAG_SIZED_INTEGER<T>::type)b)); }         \
   inline T &operator |= (T &a, T b) noexcept { return (T &)(((_ENUM_FLAG_SIZED_INTEGER<T>::type &)a) |= ((_ENUM_FLAG_SIZED_INTEGER<T>::type)b)); }  \
   constexpr T operator & (T a, T b) noexcept { return T(((_ENUM_FLAG_SIZED_INTEGER<T>::type)a) & ((_ENUM_FLAG_SIZED_INTEGER<T>::type)b)); }         \
   inline T &operator &= (T &a, T b) noexcept { return (T &)(((_ENUM_FLAG_SIZED_INTEGER<T>::type &)a) &= ((_ENUM_FLAG_SIZED_INTEGER<T>::type)b)); }  \
   constexpr T operator ~ (T a) noexcept { return T(~((_ENUM_FLAG_SIZED_INTEGER<T>::type)a)); }                                                      \
   constexpr T operator ^ (T a, T b) noexcept { return T(((_ENUM_FLAG_SIZED_INTEGER<T>::type)a) ^ ((_ENUM_FLAG_SIZED_INTEGER<T>::type)b)); }         \
   inline T &operator ^= (T &a, T b) noexcept { return (T &)(((_ENUM_FLAG_SIZED_INTEGER<T>::type &)a) ^= ((_ENUM_FLAG_SIZED_INTEGER<T>::type)b)); }  \
   constexpr bool operator ! (T a) noexcept { return !((_ENUM_FLAG_SIZED_INTEGER<T>::type)a); }                                                      
  //template <> struct std::formatter<T> : std::formatter<int> {                                                                                     \
   //  auto format(const T& id, std::format_context& ctx) const {return std::formatter<int>::format(static_cast<int>(id), ctx); }                     \
   //}

  template <typename T> constexpr bool IsValidEnum(T _value) noexcept { return (_value >= begin(_value) && _value < end(_value)); }

  template <typename T> constexpr size_t I(T _value) noexcept { return static_cast<size_t>(_value); }

  struct NonCopyable
  {
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
  };

  // Helper smart-pointers
  struct virtual_deleter
  {
    void operator()(void* p) noexcept
    {
      if (p)
        VirtualFree(p, 0, MEM_RELEASE);
    }
  };

  struct aligned_deleter
  {
    void operator()(void* p) noexcept { _aligned_free(p); }
  };

  inline std::string Trim(const std::string& s)
  {
    auto view = std::string_view(s);
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };

    // Trim start
    view.remove_prefix(std::distance(view.begin(), std::ranges::find_if(view, not_space)));
    // Trim end
    view.remove_suffix(std::distance(view.rbegin(), std::find_if(view.rbegin(), view.rend(), not_space)));

    return std::string(view);
  }

}
