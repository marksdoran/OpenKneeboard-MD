/*
 * OpenKneeboard
 *
 * Copyright (C) 2022 Fred Emmott <fred@fredemmott.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */
#pragma once

#include <shims/winrt/base.h>

#include <concepts>
#include <memory>

namespace OpenKneeboard {

namespace WeakWrap {

namespace detail {

///// std::shared_ptr /////

template <class T>
auto convert_to_weak(const std::shared_ptr<T>& p) {
  return std::weak_ptr(p);
}

template <class T>
auto convert_to_weak(std::enable_shared_from_this<T>* p) {
  return p->weak_from_this();
}

template <class T>
auto convert_to_strong(const std::weak_ptr<T>& p) {
  return p.lock();
}

///// C++/WinRT /////

template <class T>
concept with_get_weak = requires(T t) { t->get_weak(); };

template <with_get_weak T>
auto convert_to_weak(const T& t) {
  return t->get_weak();
}

template <std::derived_from<winrt::Windows::Foundation::IInspectable> T>
auto convert_to_weak(const T& t) {
  return winrt::make_weak(t);
}

template <class T>
auto convert_to_strong(const winrt::weak_ref<T>& p) {
  return p.get();
}

// "customization point objects" - ADL without the two-step

struct __convert_to_weak {
  template <class T>
  auto operator()(T&& x) const {
    return convert_to_weak(std::forward<T>(x));
  }
};

struct __convert_to_strong {
  template <class T>
  auto operator()(T&& x) const {
    return convert_to_strong(std::forward<T>(x));
  }
};

}// namespace detail

inline constexpr detail::__convert_to_weak convert_to_weak {};
inline constexpr detail::__convert_to_strong convert_to_strong {};

template <class T>
using weak_t = decltype(convert_to_weak(std::declval<T>()));
template <class T>
using strong_t = decltype(convert_to_strong(std::declval<weak_t<T>>()));

}// namespace WeakWrap

/** Wrap a function with one that captures weak pointers, but passes strong
 * (shared) pointers to the original function.
 *
 * This is useful for event handlers, to stop the event handlers from
 * introducing a refcount cycle to 'this'.
 *
 * Usage:
 *
 *    auto handler = weak_wrap(this)([](auto strongThis) { ... });
 *
 *    auto handler = weak_wrap(this)(
 *        [](WeakWrap::strong_t<decltype(this)> strongThis) {
 *            ...
 *        });
 */
template <class... TPtrs>
class weak_wrap {
 public:
  weak_wrap(TPtrs... ptrs)
    : mWeakPtrs(std::make_tuple(WeakWrap::convert_to_weak(ptrs)...)) {
  }

  auto operator()(auto f) const noexcept {
    // This is essentially an explicit deduction guide, however as of C++20
    // those are only supported for classes, not functions/methods
    return bindWithReturnType<decltype(f(
      std::declval<WeakWrap::strong_t<TPtrs>>()...))>(f);
  }

  template <class TRet>
  auto bindWithReturnType(
    typename std::function<TRet(WeakWrap::strong_t<TPtrs>...)> func)
    const noexcept {
    return [func, weak_ptrs = mWeakPtrs]() {
      const auto strong_ptrs = std::apply(
        [](auto&&... weak_ptrs) {
          return std::make_tuple(WeakWrap::convert_to_strong(weak_ptrs)...);
        },
        weak_ptrs);

      if (!std::apply(
            []<class... TArgs>(const TArgs&... args) { return (... && args); },
            strong_ptrs)) {
        return;
      }
      std::apply(func, strong_ptrs);
    };
  }

 private:
  typename std::tuple<WeakWrap::weak_t<TPtrs>...> mWeakPtrs;
};

}// namespace OpenKneeboard
