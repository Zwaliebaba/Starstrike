#pragma once

// ---------------------------------------------------------------------------
// Overloaded — aggregate for composing lambda visitors with std::visit.
//
// Usage:
//   std::visit(Overloaded{
//       [](const TypeA& a) { ... },
//       [](const TypeB& b) { ... },
//   }, someVariant);
//
// C++20 aggregate CTAD: no deduction guide needed.
// ---------------------------------------------------------------------------

template<typename... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
