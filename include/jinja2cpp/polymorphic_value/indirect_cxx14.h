/* Copyright (c) 2016 The Value Types Authors. All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
==============================================================================*/

#ifndef XYZ_INDIRECT_H
#define XYZ_INDIRECT_H

#include <cassert>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

// Local change: upstream includes "feature_check.h" here for
// XYZ_HAS_TEMPLATE_ARGUMENT_DEDUCTION; the C++17 floor always has it, and this
// copy stays a single header. Vendored from jbcoe/value_types 57f2e7a (task 0072).

namespace xyz {
#ifndef XYZ_IN_PLACE_DEFINED
#define XYZ_IN_PLACE_DEFINED

struct in_place_t {};
#endif  // XYZ_IN_PLACE_DEFINED

#ifndef XYZ_UNREACHABLE_DEFINED
#define XYZ_UNREACHABLE_DEFINED

[[noreturn]] inline void unreachable() {  // LCOV_EXCL_LINE
#if (__cpp_lib_unreachable >= 202202L)
  std::unreachable();  // LCOV_EXCL_LINE
#elif defined(_MSC_VER)
  __assume(false);  // LCOV_EXCL_LINE
#else
  __builtin_unreachable();  // LCOV_EXCL_LINE
#endif
}
#endif  // XYZ_UNREACHABLE_DEFINED

#ifndef XYZ_EMPTY_BASE_DEFINED
#define XYZ_EMPTY_BASE_DEFINED

// This is a helper class to allow empty base class optimization.
// This implementation is duplicated in compatibility/polymorphic_cxx14.h.
// These implementations must be kept in sync.
// We duplicate implementations to allow this header to work as a single
// include. https://godbolt.org needs single-file includes.
// TODO: Add tests to keep implementations in sync.
namespace detail {
template <class T, bool CanBeEmptyBaseClass =
                       std::is_empty<T>::value && !std::is_final<T>::value>
class empty_base_optimization {
 protected:
  empty_base_optimization() = default;

  empty_base_optimization(const T& t) : t_(t) {}

  empty_base_optimization(T&& t) : t_(std::move(t)) {}

  T& get() noexcept { return t_; }

  const T& get() const noexcept { return t_; }

  T t_;
};

template <class T>
class empty_base_optimization<T, true> : private T {
 protected:
  empty_base_optimization() = default;

  empty_base_optimization(const T& t) : T(t) {}

  empty_base_optimization(T&& t) : T(std::move(t)) {}

  T& get() noexcept { return *this; }

  const T& get() const noexcept { return *this; }
};
}  // namespace detail
#endif  // XYZ_EMPTY_BASE_DEFINED

template <class T, class A>
class indirect;

template <class>
struct is_indirect : std::false_type {};

template <class T, class A>
struct is_indirect<indirect<T, A>> : std::true_type {};

template <class T, class A = std::allocator<T>>
class indirect : private detail::empty_base_optimization<A> {
  using allocator_traits = std::allocator_traits<A>;
  using alloc_base = detail::empty_base_optimization<A>;

 public:
  using value_type = T;
  using allocator_type = A;
  using pointer = typename allocator_traits::pointer;
  using const_pointer = typename allocator_traits::const_pointer;

  indirect(std::allocator_arg_t, const A& alloc) : alloc_base(alloc) {
    p_ = construct_from(alloc_base::get());
  }

  template <typename AA = A,
            typename std::enable_if<std::is_default_constructible<AA>::value,
                                    int>::type = 0>
  indirect() : indirect(std::allocator_arg, A()) {}

  template <
      class U,
      typename std::enable_if<
          std::is_same<
              T, typename std::remove_cv<
                     typename std::remove_reference<U>::type>::type>::value,
          int>::type = 0,
      typename std::enable_if<
          !std::is_same<typename std::remove_cv<
                            typename std::remove_reference<U>::type>::type,
                        xyz::in_place_t>::value,
          int>::type = 0>
  explicit constexpr indirect(std::allocator_arg_t, const A& alloc, U&& u)
      : alloc_base(alloc) {
    p_ = construct_from(alloc_base::get(), std::forward<U>(u));
  }

  template <
      class U,
      typename std::enable_if<
          std::is_same<
              T, typename std::remove_cv<
                     typename std::remove_reference<U>::type>::type>::value,
          int>::type = 0,
      typename std::enable_if<
          !std::is_same<typename std::remove_cv<
                            typename std::remove_reference<U>::type>::type,
                        xyz::in_place_t>::value,
          int>::type = 0>
  explicit constexpr indirect(U&& u)
      : indirect(std::allocator_arg, A{}, std::forward<U>(u)) {}

  template <class... Us,
            typename std::enable_if<std::is_constructible<T, Us&&...>::value,
                                    int>::type = 0>
  indirect(std::allocator_arg_t, const A& alloc, xyz::in_place_t, Us&&... us)
      : alloc_base(alloc) {
    p_ = construct_from(alloc_base::get(), std::forward<Us>(us)...);
  }

  template <class... Us,
            typename std::enable_if<std::is_constructible<T, Us&&...>::value,
                                    int>::type = 0,
            class AA = A,
            typename std::enable_if<std::is_default_constructible<AA>::value,
                                    int>::type = 0>
  explicit indirect(xyz::in_place_t, Us&&... us)
      : indirect(std::allocator_arg, A(), xyz::in_place_t{},
                 std::forward<Us>(us)...) {}

  template <
      class U, class... Us,
      typename std::enable_if<
          std::is_constructible<T, std::initializer_list<U>, Us&&...>::value,
          int>::type = 0>
  indirect(std::allocator_arg_t, const A& alloc, xyz::in_place_t,
           std::initializer_list<U> ilist, Us&&... us)
      : alloc_base(alloc) {
    p_ = construct_from(alloc_base::get(), ilist, std::forward<Us>(us)...);
  }

  template <
      class AA = A,
      typename std::enable_if<std::is_default_constructible<AA>::value,
                              int>::type = 0,
      class U, class... Us,
      typename std::enable_if<
          std::is_constructible<T, std::initializer_list<U>, Us&&...>::value,
          int>::type = 0>
  indirect(xyz::in_place_t, std::initializer_list<U> ilist, Us&&... us)
      : indirect(std::allocator_arg, A(), xyz::in_place_t{}, ilist,
                 std::forward<Us>(us)...) {}

  indirect(std::allocator_arg_t, const A& alloc, const indirect& other)
      : alloc_base(alloc) {
    static_assert(std::is_copy_constructible<T>::value,
                  "T must be copy constructible");
    if (other.valueless_after_move()) {
      p_ = nullptr;
      return;
    }
    p_ = construct_from(alloc_base::get(), *other);
  }

  indirect(const indirect& other)
      : indirect(std::allocator_arg,
                 allocator_traits::select_on_container_copy_construction(
                     other.alloc_base::get()),
                 other) {}

  indirect(std::allocator_arg_t, const A& alloc,
           indirect&& other) noexcept(allocator_traits::is_always_equal::value)
      : alloc_base(alloc), p_(nullptr) {
    if (allocator_traits::is_always_equal::value) {
      std::swap(p_, other.p_);
    } else {
      if (alloc_base::get() == other.alloc_base::get()) {
        std::swap(p_, other.p_);
      } else {
        if (!other.valueless_after_move()) {
          p_ = construct_from(alloc_base::get(), std::move(*other));
          other.reset();
        } else {
          p_ = nullptr;
        }
      }
    }
  }

  indirect(indirect&& other) noexcept(allocator_traits::is_always_equal::value)
      : indirect(std::allocator_arg, other.alloc_base::get(),
                 std::move(other)) {}

  ~indirect() { reset(); }

  constexpr indirect& operator=(const indirect& other) {
    if (this == &other) return *this;

    // Check to see if the allocators need to be updated.
    // We defer actually updating the allocator until later because it may be
    // needed to delete the current control block.
    bool update_alloc =
        allocator_traits::propagate_on_container_copy_assignment::value;

    if (other.valueless_after_move()) {
      reset();
    } else {
      if (std::is_copy_assignable<T>::value && !valueless_after_move() &&
          alloc_base::get() == other.alloc_base::get()) {
        *p_ = *other;
      } else {
        // Constructing a new object could throw so we need to defer resetting
        // or updating allocators until this is done.
        auto tmp = construct_from(
            update_alloc ? other.alloc_base::get() : alloc_base::get(),
            *other.p_);
        reset();
        p_ = tmp;
      }
    }
    if (update_alloc) {
      alloc_base::get() = other.alloc_base::get();
    }
    return *this;
  }

  constexpr indirect& operator=(indirect&& other) noexcept(
      allocator_traits::propagate_on_container_move_assignment::value ||
      allocator_traits::is_always_equal::value) {
    if (this == &other) return *this;

    // Check to see if the allocators need to be updated.
    // We defer actually updating the allocator until later because it may be
    // needed to delete the current control block.
    bool update_alloc =
        allocator_traits::propagate_on_container_move_assignment::value;

    if (other.valueless_after_move()) {
      reset();
    } else {
      if (alloc_base::get() == other.alloc_base::get()) {
        std::swap(p_, other.p_);
        other.reset();
      } else {
        // Constructing a new object could throw so we need to defer resetting
        // or updating allocators until this is done.
        auto tmp = construct_from(
            update_alloc ? other.alloc_base::get() : alloc_base::get(),
            std::move(*other.p_));
        reset();
        other.reset();
        p_ = tmp;
      }
    }
    if (update_alloc) {
      alloc_base::get() = other.alloc_base::get();
    }
    return *this;
  }

  template <class U, typename std::enable_if<
                         (std::is_constructible<T, U>::value &&
                          std::is_assignable<T&, U>::value &&
                          !std::is_same<std::remove_cv_t<U>, indirect>::value),
                         int>::type = 0>
  constexpr indirect& operator=(U&& u) {
    if (valueless_after_move()) {
      p_ = construct_from(get_allocator(), std::forward<U>(u));
    } else {
      *p_ = std::forward<U>(u);
    }
    return *this;
  }

  [[nodiscard]] const T& operator*() const& noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *p_;
  }

  [[nodiscard]] T& operator*() & noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *p_;
  }

  [[nodiscard]] T&& operator*() && noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return std::move(*p_);
  }

  [[nodiscard]] const T&& operator*() const&& noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return std::move(*p_);
  }

  [[nodiscard]] const_pointer operator->() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return p_;
  }

  [[nodiscard]] pointer operator->() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return p_;
  }

  [[nodiscard]] bool valueless_after_move() const noexcept {
    return p_ == nullptr;
  }

  allocator_type get_allocator() const noexcept { return alloc_base::get(); }

  void swap(indirect& other) noexcept(
      std::allocator_traits<A>::propagate_on_container_swap::value ||
      std::allocator_traits<A>::is_always_equal::value) {
    if (allocator_traits::propagate_on_container_swap::value) {
      // If allocators move with their allocated objects we can swap both.
      std::swap(alloc_base::get(), other.alloc_base::get());
      std::swap(p_, other.p_);
      return;
    } else /*  */ {
      if (alloc_base::get() == other.alloc_base::get()) {
        std::swap(p_, other.p_);
      } else {
        unreachable();  // LCOV_EXCL_LINE
      }
    }
  }

  friend void swap(indirect& lhs,
                   indirect& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

  template <class U, class AA>
  [[nodiscard]] friend bool operator==(
      const indirect<T, A>& lhs,
      const indirect<U, AA>& rhs) noexcept(noexcept(*lhs == *rhs)) {
    if (lhs.valueless_after_move() || rhs.valueless_after_move()) {
      return lhs.valueless_after_move() && rhs.valueless_after_move();
    }
    return *lhs == *rhs;
  }

  template <class U, class AA>
  [[nodiscard]] friend bool operator!=(
      const indirect<T, A>& lhs,
      const indirect<U, AA>& rhs) noexcept(noexcept(*lhs != *rhs)) {
    if (lhs.valueless_after_move() || rhs.valueless_after_move()) {
      return !(lhs.valueless_after_move() && rhs.valueless_after_move());
    }
    return *lhs != *rhs;
  }

  template <class U, class AA>
  [[nodiscard]] friend bool operator<(
      const indirect<T, A>& lhs,
      const indirect<U, AA>& rhs) noexcept(noexcept(*lhs < *rhs)) {
    if (lhs.valueless_after_move()) {
      return !rhs.valueless_after_move();
    }
    return !rhs.valueless_after_move() && *lhs < *rhs;
  }

  template <class U, class AA>
  [[nodiscard]] friend bool operator>(
      const indirect<T, A>& lhs,
      const indirect<U, AA>& rhs) noexcept(noexcept(*lhs > *rhs)) {
    if (rhs.valueless_after_move()) {
      return !lhs.valueless_after_move();
    }
    return !lhs.valueless_after_move() && *lhs > *rhs;
  }

  template <class U, class AA>
  [[nodiscard]] friend bool operator<=(
      const indirect<T, A>& lhs,
      const indirect<U, AA>& rhs) noexcept(noexcept(*lhs <= *rhs)) {
    if (lhs.valueless_after_move()) {
      return true;
    }
    return !rhs.valueless_after_move() && *lhs <= *rhs;
  }

  template <class U, class AA>
  [[nodiscard]] friend bool operator>=(
      const indirect<T, A>& lhs,
      const indirect<U, AA>& rhs) noexcept(noexcept(*lhs >= *rhs)) {
    if (rhs.valueless_after_move()) {
      return true;
    }

    return !lhs.valueless_after_move() && *lhs >= *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator==(const indirect<T, A>& lhs,
                                       const U& rhs) noexcept(noexcept(*lhs ==
                                                                       rhs)) {
    if (lhs.valueless_after_move()) {
      return false;
    }
    return *lhs == rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator==(
      const U& lhs, const indirect<T, A>& rhs) noexcept(noexcept(lhs == *rhs)) {
    if (rhs.valueless_after_move()) {
      return false;
    }
    return lhs == *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator!=(const indirect<T, A>& lhs,
                                       const U& rhs) noexcept(noexcept(*lhs !=
                                                                       rhs)) {
    if (lhs.valueless_after_move()) {
      return true;
    }
    return *lhs != rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator!=(
      const U& lhs, const indirect<T, A>& rhs) noexcept(noexcept(lhs != *rhs)) {
    if (rhs.valueless_after_move()) {
      return true;
    }
    return lhs != *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator<(const indirect<T, A>& lhs,
                                      const U& rhs) noexcept(noexcept(*lhs <
                                                                      rhs)) {
    if (lhs.valueless_after_move()) {
      return true;
    }
    return *lhs < rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator<(
      const U& lhs, const indirect<T, A>& rhs) noexcept(noexcept(lhs < *rhs)) {
    if (rhs.valueless_after_move()) {
      return false;
    }
    return lhs < *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator>(const indirect<T, A>& lhs,
                                      const U& rhs) noexcept(noexcept(*lhs >
                                                                      rhs)) {
    if (lhs.valueless_after_move()) {
      return false;
    }
    return *lhs > rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator>(
      const U& lhs, const indirect<T, A>& rhs) noexcept(noexcept(lhs > *rhs)) {
    if (rhs.valueless_after_move()) {
      return true;
    }
    return lhs > *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator<=(const indirect<T, A>& lhs,
                                       const U& rhs) noexcept(noexcept(*lhs <=
                                                                       rhs)) {
    if (lhs.valueless_after_move()) {
      return true;
    }
    return *lhs <= rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator<=(
      const U& lhs, const indirect<T, A>& rhs) noexcept(noexcept(lhs <= *rhs)) {
    if (rhs.valueless_after_move()) {
      return false;
    }
    return lhs <= *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator>=(const indirect<T, A>& lhs,
                                       const U& rhs) noexcept(noexcept(*lhs >=
                                                                       rhs)) {
    if (lhs.valueless_after_move()) {
      return false;
    }
    return *lhs >= rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  [[nodiscard]] friend bool operator>=(
      const U& lhs, const indirect<T, A>& rhs) noexcept(noexcept(lhs >= *rhs)) {
    if (rhs.valueless_after_move()) {
      return true;
    }
    return lhs >= *rhs;
  }

 private:
  pointer p_;

  void reset() noexcept {
    if (p_ == nullptr) return;
    destroy_with(alloc_base::get(), p_);
    p_ = nullptr;
  }

  template <typename... Ts>
  [[nodiscard]] static T* construct_from(A alloc, Ts&&... ts) {
    T* mem = allocator_traits::allocate(alloc, 1);
    try {
      allocator_traits::construct(alloc, mem, std::forward<Ts>(ts)...);
      return mem;
    } catch (...) {
      allocator_traits::deallocate(alloc, mem, 1);
      throw;
    }
  }

  static void destroy_with(A alloc, T* p) {
    allocator_traits::destroy(alloc, p);
    allocator_traits::deallocate(alloc, p, 1);
  }
};

template <typename Value>
indirect(Value) -> indirect<Value>;

template <typename Alloc, typename Value>
indirect(std::allocator_arg_t, Alloc, Value) -> indirect<
    Value, typename std::allocator_traits<Alloc>::template rebind_alloc<Value>>;

}  // namespace xyz

template <class T, class Alloc>
struct std::hash<xyz::indirect<T, Alloc>> {
  std::size_t operator()(const xyz::indirect<T, Alloc>& key) const {
    if (key.valueless_after_move()) {
      return static_cast<std::size_t>(-1);  // Implementation defined value.
    }
    return std::hash<typename xyz::indirect<T, Alloc>::value_type>{}(*key);
  }
};

#endif  // XYZ_INDIRECT_H
