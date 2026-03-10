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

#ifndef XYZ_POLYMORPHIC_H_
#define XYZ_POLYMORPHIC_H_

#include <cassert>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

#ifndef XYZ_POLYMORPHIC_HAS_EXTENDED_CONSTRUCTORS
#define XYZ_POLYMORPHIC_HAS_EXTENDED_CONSTRUCTORS 1
#endif  // XYZ_POLYMORPHIC_HAS_EXTENDED_CONSTRUCTORS

#ifndef XYZ_IN_PLACE_TYPE_DEFINED
#define XYZ_IN_PLACE_TYPE_DEFINED

namespace xyz {
template <class T>
struct in_place_type_t {};
}  // namespace xyz
#endif  // XYZ_IN_PLACE_TYPE_DEFINED

#ifndef XYZ_UNREACHABLE_DEFINED
#define XYZ_UNREACHABLE_DEFINED

namespace xyz {
[[noreturn]] inline void unreachable() {  // LCOV_EXCL_LINE
#if (__cpp_lib_unreachable >= 202202L)
  std::unreachable();  // LCOV_EXCL_LINE
#elif defined(_MSC_VER)
  __assume(false);  // LCOV_EXCL_LINE
#else
  __builtin_unreachable();  // LCOV_EXCL_LINE
#endif
}
}  // namespace xyz
#endif  // XYZ_UNREACHABLE_DEFINED

#ifndef XYZ_EMPTY_BASE_DEFINED
#define XYZ_EMPTY_BASE_DEFINED

// This is a helper class to allow empty base class optimization.
// This implementation is duplicated in compatibility/in_place_type_cxx14.h.
// These implementations must be kept in sync.
// We duplicate implementations to allow this header to work as a single
// include. https://godbolt.org needs single-file includes.
namespace xyz {
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
}  // namespace xyz
#endif  // XYZ_EMPTY_BASE_DEFINED

namespace xyz {

template <class T, class A = std::allocator<T>>
class polymorphic : private detail::empty_base_optimization<A> {
  struct control_block {
    using allocator_traits = std::allocator_traits<A>;
    typename allocator_traits::pointer p_;

    virtual ~control_block() = default;
    virtual void destroy(A& alloc) = 0;
    virtual control_block* clone(const A& alloc) = 0;
    virtual control_block* move(const A& alloc) = 0;
  };

  template <class U>
  class direct_control_block final : public control_block {
    union uninitialized_storage {
      U u_;

      uninitialized_storage() {}

      ~uninitialized_storage() {}
    } storage_;

    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<U>>;
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;

   public:
    template <class... Ts>
    direct_control_block(const A& alloc, Ts&&... ts) {
      cb_allocator cb_alloc(alloc);
      cb_alloc_traits::construct(cb_alloc, std::addressof(storage_.u_),
                                 std::forward<Ts>(ts)...);
      control_block::p_ = std::addressof(storage_.u_);
    }

    control_block* clone(const A& alloc) override {
      cb_allocator cb_alloc(alloc);
      auto mem = cb_alloc_traits::allocate(cb_alloc, 1);
      try {
        cb_alloc_traits::construct(cb_alloc, mem, alloc, storage_.u_);
        return mem;
      } catch (...) {
        cb_alloc_traits::deallocate(cb_alloc, mem, 1);
        throw;
      }
    }

    control_block* move(const A& alloc) override {
      cb_allocator cb_alloc(alloc);
      auto mem = cb_alloc_traits::allocate(cb_alloc, 1);
      try {
        cb_alloc_traits::construct(cb_alloc, mem, alloc,
                                   std::move(storage_.u_));
        return mem;
      } catch (...) {
        cb_alloc_traits::deallocate(cb_alloc, mem, 1);
        throw;
      }
    }

    void destroy(A& alloc) override {
      cb_allocator cb_alloc(alloc);
      cb_alloc_traits::destroy(cb_alloc, std::addressof(storage_.u_));
      cb_alloc_traits::deallocate(cb_alloc, this, 1);
    }
  };

  control_block* cb_;
  using allocator_traits = std::allocator_traits<A>;
  using alloc_base = detail::empty_base_optimization<A>;

  template <class U, class... Ts>
  control_block* create_control_block(Ts&&... ts) const {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<U>>;
    cb_allocator cb_alloc(alloc_base::get());
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    auto mem = cb_alloc_traits::allocate(cb_alloc, 1);
    try {
      cb_alloc_traits::construct(cb_alloc, mem, alloc_base::get(),
                                 std::forward<Ts>(ts)...);
      return mem;
    } catch (...) {
      cb_alloc_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

 public:
  using value_type = T;
  using allocator_type = A;
  using pointer = typename allocator_traits::pointer;
  using const_pointer = typename allocator_traits::const_pointer;

  template <typename TT = T,
            typename std::enable_if<std::is_default_constructible<TT>::value,
                                    int>::type = 0>
  polymorphic(std::allocator_arg_t, const A& alloc) : alloc_base(alloc) {
    cb_ = create_control_block<T>();
  }

  template <typename TT = T,
            typename std::enable_if<std::is_default_constructible<TT>::value,
                                    int>::type = 0,
            typename AA = A,
            typename std::enable_if<std::is_default_constructible<AA>::value,
                                    int>::type = 0>
  polymorphic() : alloc_base() {
    cb_ = create_control_block<T>();
  }

  template <
      class U, class... Ts,
      typename std::enable_if<std::is_constructible<U, Ts&&...>::value,
                              int>::type = 0,
      typename std::enable_if<std::is_copy_constructible<U>::value, int>::type =
          0,
      typename std::enable_if<std::is_base_of<T, U>::value, int>::type = 0>
  polymorphic(std::allocator_arg_t, const A& alloc, in_place_type_t<U>,
              Ts&&... ts)
      : alloc_base(alloc) {
    cb_ = create_control_block<U>(std::forward<Ts>(ts)...);
  }

  template <
      class U, class I, class... Ts,
      typename std::enable_if<
          std::is_constructible<U, std::initializer_list<I>, Ts&&...>::value,
          int>::type = 0,
      typename std::enable_if<std::is_copy_constructible<U>::value, int>::type =
          0,
      typename std::enable_if<std::is_base_of<T, U>::value, int>::type = 0>
  polymorphic(std::allocator_arg_t, const A& alloc, in_place_type_t<U>,
              std::initializer_list<I> ilist, Ts&&... ts)
      : alloc_base(alloc) {
    cb_ = create_control_block<T>(ilist, std::forward<Ts>(ts)...);
  }

  template <
      class U, class I, class... Ts,
      typename std::enable_if<
          std::is_constructible<U, std::initializer_list<I>, Ts&&...>::value,
          int>::type = 0,
      typename std::enable_if<std::is_copy_constructible<U>::value, int>::type =
          0,
      typename std::enable_if<std::is_base_of<T, U>::value, int>::type = 0,
      typename AA = A,
      typename std::enable_if<std::is_default_constructible<AA>::value,
                              int>::type = 0>
  explicit polymorphic(in_place_type_t<U>, std::initializer_list<I> ilist,
                       Ts&&... ts)
      : polymorphic(std::allocator_arg, A(), in_place_type_t<U>{}, ilist,
                    std::forward<Ts>(ts)...) {}

  template <
      class U, class... Ts,
      typename std::enable_if<std::is_constructible<U, Ts&&...>::value,
                              int>::type = 0,
      typename std::enable_if<std::is_copy_constructible<U>::value, int>::type =
          0,
      typename std::enable_if<std::is_base_of<T, U>::value, int>::type = 0,
      typename AA = A,
      typename std::enable_if<std::is_default_constructible<AA>::value,
                              int>::type = 0>
  explicit polymorphic(in_place_type_t<U>, Ts&&... ts)
      : polymorphic(std::allocator_arg, A(), in_place_type_t<U>{},
                    std::forward<Ts>(ts)...) {}

  template <
      class U,
      typename std::enable_if<
          !std::is_same<polymorphic,
                        typename std::remove_cv<typename std::remove_reference<
                            U>::type>::type>::value,
          int>::type = 0,
      typename std::enable_if<
          std::is_copy_constructible<typename std::remove_cv<
              typename std::remove_reference<U>::type>::type>::value,
          int>::type = 0,
      typename std::enable_if<
          std::is_base_of<
              T, typename std::remove_cv<
                     typename std::remove_reference<U>::type>::type>::value,
          int>::type = 0>
  explicit polymorphic(std::allocator_arg_t, const A& alloc, U&& u)
      : polymorphic(std::allocator_arg_t{}, alloc,
                    in_place_type_t<typename std::remove_cv<
                        typename std::remove_reference<U>::type>::type>{},
                    std::forward<U>(u)) {}

  template <
      class U,
      typename std::enable_if<
          !std::is_same<polymorphic,
                        typename std::remove_cv<typename std::remove_reference<
                            U>::type>::type>::value,
          int>::type = 0,
      typename std::enable_if<
          std::is_copy_constructible<typename std::remove_cv<
              typename std::remove_reference<U>::type>::type>::value,
          int>::type = 0,
      typename std::enable_if<
          std::is_base_of<
              T, typename std::remove_cv<
                     typename std::remove_reference<U>::type>::type>::value,
          int>::type = 0>
  explicit polymorphic(U&& u)
      : polymorphic(std::allocator_arg_t{}, A{},
                    in_place_type_t<typename std::remove_cv<
                        typename std::remove_reference<U>::type>::type>{},
                    std::forward<U>(u)) {}

  polymorphic(std::allocator_arg_t, const A& alloc, const polymorphic& other)
      : alloc_base(alloc) {
    if (!other.valueless_after_move()) {
      cb_ = other.cb_->clone(alloc_base::get());
    } else {
      cb_ = nullptr;
    }
  }

  polymorphic(const polymorphic& other)
      : polymorphic(std::allocator_arg,
                    allocator_traits::select_on_container_copy_construction(
                        other.get_allocator()),
                    other) {}

  polymorphic(
      std::allocator_arg_t, const A& alloc,
      polymorphic&& other) noexcept(allocator_traits::is_always_equal::value)
      : alloc_base(alloc) {
    if (allocator_traits::propagate_on_container_copy_assignment::value) {
      cb_ = other.cb_;
      other.cb_ = nullptr;
    } else {
      if (get_allocator() == other.get_allocator()) {
        cb_ = other.cb_;
        other.cb_ = nullptr;
      } else {
        if (!other.valueless_after_move()) {
          cb_ = other.cb_->move(alloc_base::get());
        } else {
          cb_ = nullptr;
        }
      }
    }
  }

  polymorphic(polymorphic&& other) noexcept
      : polymorphic(std::allocator_arg, other.get_allocator(),
                    std::move(other)) {}

  ~polymorphic() { reset(); }

  constexpr polymorphic& operator=(const polymorphic& other) {
    if (this == &other) return *this;

    // Check to see if the allocators need to be updated.
    // We defer actually updating the allocator until later because it may be
    // needed to delete the current control block.
    bool update_alloc =
        allocator_traits::propagate_on_container_copy_assignment::value;

    if (other.valueless_after_move()) {
      reset();
    } else {
      // Constructing a new control block could throw so we need to defer
      // resetting or updating allocators until this is done.
      auto tmp = other.cb_->clone(update_alloc ? other.alloc_base::get()
                                               : alloc_base::get());
      reset();
      cb_ = tmp;
    }
    if (update_alloc) {
      alloc_base::get() = other.alloc_base::get();
    }
    return *this;
  }

  constexpr polymorphic& operator=(polymorphic&& other) noexcept(
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
        std::swap(cb_, other.cb_);
        other.reset();
      } else {
        // Constructing a new control block could throw so we need to defer
        // resetting or updating allocators until this is done.
        auto tmp = other.cb_->move(update_alloc ? other.alloc_base::get()
                                                : alloc_base::get());
        reset();
        cb_ = tmp;
      }
    }

    if (update_alloc) {
      alloc_base::get() = other.alloc_base::get();
    }
    return *this;
  }

  [[nodiscard]] pointer operator->() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return cb_->p_;
  }

  [[nodiscard]] const_pointer operator->() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return cb_->p_;
  }

  [[nodiscard]] T& operator*() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *cb_->p_;
  }

  [[nodiscard]] const T& operator*() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *cb_->p_;
  }

  [[nodiscard]] bool valueless_after_move() const noexcept {
    return cb_ == nullptr;
  }

  allocator_type get_allocator() const noexcept { return alloc_base::get(); }

  void swap(polymorphic& other) noexcept(
      std::allocator_traits<A>::propagate_on_container_swap::value ||
      std::allocator_traits<A>::is_always_equal::value) {
    if (allocator_traits::propagate_on_container_swap::value) {
      // If allocators move with their allocated objects we can swap both.
      std::swap(alloc_base::get(), other.alloc_base::get());
      std::swap(cb_, other.cb_);
      return;
    } else /*  */ {
      if (alloc_base::get() == other.alloc_base::get()) {
        std::swap(cb_, other.cb_);
      } else {
        unreachable();  // LCOV_EXCL_LINE
      }
    }
  }

  friend void swap(polymorphic& lhs,
                   polymorphic& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

 private:
  void reset() noexcept {
    if (cb_ != nullptr) {
      cb_->destroy(alloc_base::get());
      cb_ = nullptr;
    }
  }
};

}  // namespace xyz

#endif  // XYZ_POLYMORPHIC_H_
