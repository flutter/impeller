// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <algorithm>
#include <cmath>
#include <ostream>
#include <string>
#include <type_traits>

#include "impeller/geometry/scalar.h"
#include "impeller/geometry/size.h"
#include "impeller/geometry/type_traits.h"

namespace impeller {

template <class T>
struct TPoint {
  using Type = T;

  Type x = {};
  Type y = {};

  constexpr TPoint() = default;

  template <class U>
  explicit constexpr TPoint(const TPoint<U>& other)
      : TPoint(static_cast<Type>(other.x), static_cast<Type>(other.y)) {}

  template <class U>
  explicit constexpr TPoint(const TSize<U>& other)
      : TPoint(static_cast<Type>(other.width),
               static_cast<Type>(other.height)) {}

  constexpr TPoint(Type x, Type y) : x(x), y(y) {}

  static constexpr TPoint<Type> MakeXY(Type x, Type y) { return {x, y}; }

  constexpr bool operator==(const TPoint& p) const {
    return p.x == x && p.y == y;
  }

  constexpr bool operator!=(const TPoint& p) const {
    return p.x != x || p.y != y;
  }

  template <class U>
  inline TPoint operator+=(const TPoint<U>& p) {
    x += static_cast<Type>(p.x);
    y += static_cast<Type>(p.y);
    return *this;
  }

  template <class U>
  inline TPoint operator+=(const TSize<U>& s) {
    x += static_cast<Type>(s.width);
    y += static_cast<Type>(s.height);
    return *this;
  }

  template <class U>
  inline TPoint operator-=(const TPoint<U>& p) {
    x -= static_cast<Type>(p.x);
    y -= static_cast<Type>(p.y);
    return *this;
  }

  template <class U>
  inline TPoint operator-=(const TSize<U>& s) {
    x -= static_cast<Type>(s.width);
    y -= static_cast<Type>(s.height);
    return *this;
  }

  template <class U>
  inline TPoint operator*=(const TPoint<U>& p) {
    x *= static_cast<Type>(p.x);
    y *= static_cast<Type>(p.y);
    return *this;
  }

  template <class U>
  inline TPoint operator*=(const TSize<U>& s) {
    x *= static_cast<Type>(s.width);
    y *= static_cast<Type>(s.height);
    return *this;
  }

  template <class U>
  inline TPoint operator/=(const TPoint<U>& p) {
    x /= static_cast<Type>(p.x);
    y /= static_cast<Type>(p.y);
    return *this;
  }

  template <class U>
  inline TPoint operator/=(const TSize<U>& s) {
    x /= static_cast<Type>(s.width);
    y /= static_cast<Type>(s.height);
    return *this;
  }

  constexpr TPoint operator-() const { return {-x, -y}; }

  constexpr TPoint operator+(const TPoint& p) const {
    return {x + p.x, y + p.y};
  }

  template <class U>
  constexpr TPoint operator+(const TSize<U>& s) const {
    return {x + static_cast<Type>(s.width), y + static_cast<Type>(s.height)};
  }

  constexpr TPoint operator-(const TPoint& p) const {
    return {x - p.x, y - p.y};
  }

  template <class U>
  constexpr TPoint operator-(const TSize<U>& s) const {
    return {x - static_cast<Type>(s.width), y - static_cast<Type>(s.height)};
  }

  template <class U, class = std::enable_if_t<std::is_arithmetic_v<U>>>
  constexpr TPoint operator*(U scale) const {
    return {static_cast<Type>(x * scale), static_cast<Type>(y * scale)};
  }

  constexpr TPoint operator*(const TPoint& p) const {
    return {x * p.x, y * p.y};
  }

  template <class U>
  constexpr TPoint operator*(const TSize<U>& s) const {
    return {x * static_cast<Type>(s.width), y * static_cast<Type>(s.height)};
  }

  template <class U, class = std::enable_if_t<std::is_arithmetic_v<U>>>
  constexpr TPoint operator/(U d) const {
    return {static_cast<Type>(x / d), static_cast<Type>(y / d)};
  }

  constexpr TPoint operator/(const TPoint& p) const {
    return {x / p.x, y / p.y};
  }

  template <class U>
  constexpr TPoint operator/(const TSize<U>& s) const {
    return {x / static_cast<Type>(s.width), y / static_cast<Type>(s.height)};
  }

  constexpr Type GetDistanceSquared(const TPoint& p) const {
    double dx = p.x - x;
    double dy = p.y - y;
    return dx * dx + dy * dy;
  }

  constexpr TPoint Min(const TPoint& p) const {
    return {std::min<Type>(x, p.x), std::min<Type>(y, p.y)};
  }

  constexpr TPoint Max(const TPoint& p) const {
    return {std::max<Type>(x, p.x), std::max<Type>(y, p.y)};
  }

  constexpr Type GetDistance(const TPoint& p) const {
    return sqrt(GetDistanceSquared(p));
  }

  constexpr Type GetLengthSquared() const { return GetDistanceSquared({}); }

  constexpr Type GetLength() const { return GetDistance({}); }

  constexpr TPoint Normalize() const {
    const auto length = GetLength();
    if (length == 0) {
      return {};
    }
    return {x / length, y / length};
  }

  constexpr Type Cross(const TPoint& p) const { return (x * p.y) - (y * p.x); }

  constexpr Type Dot(const TPoint& p) const { return (x * p.x) + (y * p.y); }

  constexpr TPoint Reflect(const TPoint& axis) const {
    return *this - axis * this->Dot(axis) * 2;
  }

  constexpr bool IsZero() const { return x == 0 && y == 0; }
};

// Specializations for mixed (float & integer) algebraic operations.

template <class F, class I, class = MixedOp<F, I>>
constexpr TPoint<F> operator+(const TPoint<F>& p1, const TPoint<I>& p2) {
  return {p1.x + static_cast<F>(p2.x), p1.y + static_cast<F>(p2.y)};
}

template <class F, class I, class = MixedOp<F, I>>
constexpr TPoint<F> operator+(const TPoint<I>& p1, const TPoint<F>& p2) {
  return p2 + p1;
}

template <class F, class I, class = MixedOp<F, I>>
constexpr TPoint<F> operator-(const TPoint<F>& p1, const TPoint<I>& p2) {
  return {p1.x - static_cast<F>(p2.x), p1.y - static_cast<F>(p2.y)};
}

template <class F, class I, class = MixedOp<F, I>>
constexpr TPoint<F> operator-(const TPoint<I>& p1, const TPoint<F>& p2) {
  return {static_cast<F>(p1.x) - p2.x, static_cast<F>(p1.y) - p2.y};
}

template <class F, class I, class = MixedOp<F, I>>
constexpr TPoint<F> operator*(const TPoint<F>& p1, const TPoint<I>& p2) {
  return {p1.x * static_cast<F>(p2.x), p1.y * static_cast<F>(p2.y)};
}

template <class F, class I, class = MixedOp<F, I>>
constexpr TPoint<F> operator*(const TPoint<I>& p1, const TPoint<F>& p2) {
  return p2 * p1;
}

template <class F, class I, class = MixedOp<F, I>>
constexpr TPoint<F> operator/(const TPoint<F>& p1, const TPoint<I>& p2) {
  return {p1.x / static_cast<F>(p2.x), p1.y / static_cast<F>(p2.y)};
}

template <class F, class I, class = MixedOp<F, I>>
constexpr TPoint<F> operator/(const TPoint<I>& p1, const TPoint<F>& p2) {
  return {static_cast<F>(p1.x) / p2.x, static_cast<F>(p1.y) / p2.y};
}

// RHS algebraic operations with arithmetic types.

template <class T, class U, class = std::enable_if_t<std::is_arithmetic_v<U>>>
constexpr TPoint<T> operator*(U s, const TPoint<T>& p) {
  return p * s;
}

template <class T, class U, class = std::enable_if_t<std::is_arithmetic_v<U>>>
constexpr TPoint<T> operator/(U s, const TPoint<T>& p) {
  return {static_cast<T>(s) / p.x, static_cast<T>(s) / p.y};
}

// RHS algebraic operations with TSize.

template <class T, class U>
constexpr TPoint<T> operator+(const TSize<U>& s, const TPoint<T>& p) {
  return p + s;
}

template <class T, class U>
constexpr TPoint<T> operator-(const TSize<U>& s, const TPoint<T>& p) {
  return {static_cast<T>(s.width) - p.x, static_cast<T>(s.height) - p.y};
}

template <class T, class U>
constexpr TPoint<T> operator*(const TSize<U>& s, const TPoint<T>& p) {
  return p * s;
}

template <class T, class U>
constexpr TPoint<T> operator/(const TSize<U>& s, const TPoint<T>& p) {
  return {static_cast<T>(s.width) / p.x, static_cast<T>(s.height) / p.y};
}

using Point = TPoint<Scalar>;
using IPoint = TPoint<int64_t>;

}  // namespace impeller

namespace std {

template <class T>
inline std::ostream& operator<<(std::ostream& out,
                                const impeller::TPoint<T>& p) {
  out << "(" << p.x << ", " << p.y << ")";
  return out;
}

}  // namespace std
