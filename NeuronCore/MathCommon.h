#pragma once

namespace Neuron::Math
{
  constexpr uint8_t Log2(uint64_t value)
  {
    if (value == 0) return 0;
    return static_cast<uint8_t>(std::bit_width(value - 1));
  }

  template <typename T>
  constexpr T AlignUpWithMask(T value, size_t mask)
  {
    if constexpr (std::is_convertible_v<T, size_t>)
      return static_cast<T>(static_cast<size_t>(value) + mask & ~mask);
    else
      return reinterpret_cast<T>(reinterpret_cast<size_t>(value) + mask & ~mask);
  }

  template <typename T>
  constexpr T AlignDownWithMask(T value, size_t mask)
  {
    if constexpr (std::is_convertible_v<T, size_t>)
      return static_cast<T>(static_cast<size_t>(value) & ~mask);
    else
      return reinterpret_cast<T>(reinterpret_cast<size_t>(value) & ~mask);
  }

  template <typename T>
  constexpr T AlignUp(T value, size_t alignment) { return AlignUpWithMask(value, alignment - 1); }

  template <typename T>
  constexpr T AlignDown(T value, size_t alignment) { return AlignDownWithMask(value, alignment - 1); }

  template <typename T>
  constexpr bool IsAligned(T value, size_t alignment) { return 0 == (reinterpret_cast<size_t>(value) & (alignment - 1)); }

  template <typename T>
  constexpr T DivideByMultiple(T value, size_t alignment) { return static_cast<T>((value + alignment - 1) / alignment); }

  template <typename T>
  constexpr bool IsPowerOfTwo(T value) { return 0 == (value & (value - 1)); }

  template <typename T>
  constexpr bool IsDivisible(T value, T divisor) { return (value / divisor) * divisor == value; }
}
