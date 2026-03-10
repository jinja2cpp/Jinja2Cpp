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
#include <concepts>
#include <initializer_list>
#include <memory>
#include <utility>

#ifndef XYZ_POLYMORPHIC_HAS_EXTENDED_CONSTRUCTORS
#define XYZ_POLYMORPHIC_HAS_EXTENDED_CONSTRUCTORS 1
#endif  // XYZ_POLYMORPHIC_HAS_EXTENDED_CONSTRUCTORS

namespace xyz {

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

template <class T, class A = std::allocator<T>>
class polymorphic {
  struct control_block {
    using allocator_traits = std::allocator_traits<A>;
    typename allocator_traits::pointer p_;

    virtual constexpr ~control_block() = default;
    virtual constexpr void destroy(A& alloc) = 0;
    virtual constexpr control_block* clone(const A& alloc) = 0;
    virtual constexpr control_block* move(const A& alloc) = 0;
  };

  template <class U>
  class direct_control_block final : public control_block {
    union uninitialized_storage {
      U u_;

      constexpr uninitialized_storage() {}

      constexpr ~uninitialized_storage() {}
    } storage_;

    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<U>>;
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;

   public:
    template <class... Ts>
    constexpr direct_control_block(const A& alloc, Ts&&... ts) {
      cb_allocator cb_alloc(alloc);
      cb_alloc_traits::construct(cb_alloc, std::addressof(storage_.u_),
                                 std::forward<Ts>(ts)...);
      control_block::p_ = std::addressof(storage_.u_);
    }

    constexpr control_block* clone(const A& alloc) override {
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

    constexpr control_block* move(const A& alloc) override {
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

    constexpr void destroy(A& alloc) override {
      cb_allocator cb_alloc(alloc);
      cb_alloc_traits::destroy(cb_alloc, std::addressof(storage_.u_));
      cb_alloc_traits::deallocate(cb_alloc, this, 1);
    }
  };

  control_block* cb_;

#if defined(_MSC_VER)
  // https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#msvc-extensions-and-abi
  [[msvc::no_unique_address]] A alloc_;
#else
  [[no_unique_address]] A alloc_;
#endif

  using allocator_traits = std::allocator_traits<A>;

  template <class U, class... Ts>
  [[nodiscard]] constexpr control_block* create_control_block(
      Ts&&... ts) const {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<U>>;
    cb_allocator cb_alloc(alloc_);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    auto mem = cb_alloc_traits::allocate(cb_alloc, 1);
    try {
      cb_alloc_traits::construct(cb_alloc, mem, alloc_,
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

  //
  // Constructors.
  //

  explicit constexpr polymorphic()
    requires std::default_initializable<A>
      : polymorphic(std::allocator_arg_t{}, A{}) {
    static_assert(std::default_initializable<T> && std::copy_constructible<T>);
  }

  template <class U>
  constexpr explicit polymorphic(U&& u)
    requires(!std::same_as<polymorphic, std::remove_cvref_t<U>>) &&
            std::copy_constructible<std::remove_cvref_t<U>> &&
            std::derived_from<std::remove_cvref_t<U>, T> &&
            std::default_initializable<A>
      : polymorphic(std::allocator_arg_t{}, A{}, std::forward<U>(u)) {}

  template <class U, class... Ts>
  explicit constexpr polymorphic(std::in_place_type_t<U>, Ts&&... ts)
    requires std::same_as<std::remove_cvref_t<U>, U> &&
             std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T> &&
             std::default_initializable<A>
      : polymorphic(std::allocator_arg_t{}, A{}, std::in_place_type<U>,
                    std::forward<Ts>(ts)...) {}

  template <class U, class I, class... Ts>
  explicit constexpr polymorphic(std::in_place_type_t<U>,
                                 std::initializer_list<I> ilist, Ts&&... ts)
    requires std::same_as<std::remove_cvref_t<U>, U> &&
             std::constructible_from<U, std::initializer_list<I>, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T> &&
             std::default_initializable<A>
      : polymorphic(std::allocator_arg_t{}, A{}, std::in_place_type<U>, ilist,
                    std::forward<Ts>(ts)...) {}

  constexpr polymorphic(const polymorphic& other)
      : polymorphic(std::allocator_arg_t{},
                    allocator_traits::select_on_container_copy_construction(
                        other.alloc_),
                    other) {}

  constexpr polymorphic(polymorphic&& other) noexcept(
      allocator_traits::is_always_equal::value)
      : polymorphic(std::allocator_arg_t{}, other.alloc_, std::move(other)) {}

  //
  // Allocator-extended constructors.
  //

  explicit constexpr polymorphic(std::allocator_arg_t, const A& alloc)
      : alloc_(alloc) {
    static_assert(std::default_initializable<T> && std::copy_constructible<T>);

    cb_ = create_control_block<T>();
  }

  template <class U>
  constexpr explicit polymorphic(std::allocator_arg_t, const A& alloc, U&& u)
    requires(not std::same_as<polymorphic, std::remove_cvref_t<U>>) &&
            std::copy_constructible<std::remove_cvref_t<U>> &&
            std::derived_from<std::remove_cvref_t<U>, T>
      : alloc_(alloc) {
    cb_ = create_control_block<std::remove_cvref_t<U>>(std::forward<U>(u));
  }

  template <class U, class... Ts>
  explicit constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                                 std::in_place_type_t<U>, Ts&&... ts)
    requires std::same_as<std::remove_cvref_t<U>, U> &&
             std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T>
      : alloc_(alloc) {
    cb_ = create_control_block<U>(std::forward<Ts>(ts)...);
  }

  template <class U, class I, class... Ts>
  explicit constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                                 std::in_place_type_t<U>,
                                 std::initializer_list<I> ilist, Ts&&... ts)
    requires std::same_as<std::remove_cvref_t<U>, U> &&
             std::constructible_from<U, std::initializer_list<I>, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T>
      : alloc_(alloc) {
    cb_ = create_control_block<U>(ilist, std::forward<Ts>(ts)...);
  }

  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        const polymorphic& other)
      : alloc_(alloc) {
    if (!other.valueless_after_move()) {
      cb_ = other.cb_->clone(alloc_);
    } else {
      cb_ = nullptr;
    }
  }

  constexpr polymorphic(
      std::allocator_arg_t, const A& alloc,
      polymorphic&& other) noexcept(allocator_traits::is_always_equal::value)
      : alloc_(alloc) {
    if constexpr (allocator_traits::is_always_equal::value) {
      cb_ = std::exchange(other.cb_, nullptr);
    } else {
      if (alloc_ == other.alloc_) {
        cb_ = std::exchange(other.cb_, nullptr);
      } else {
        if (!other.valueless_after_move()) {
          cb_ = other.cb_->move(alloc_);
        } else {
          cb_ = nullptr;
        }
      }
    }
  }

  //
  // Destructor.
  //

  constexpr ~polymorphic() { reset(); }

  //
  // Assignment operators.
  //

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

      // Inlining the allocator into the construct_from call confuses LCOV.
      auto tmp = other.cb_->clone(update_alloc ? other.alloc_ : alloc_);
      reset();
      cb_ = tmp;
    }
    if (update_alloc) {
      alloc_ = other.alloc_;
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
      if (alloc_ == other.alloc_) {
        std::swap(cb_, other.cb_);
        other.reset();
      } else {
        // Constructing a new control block could throw so we need to defer
        // resetting or updating allocators until this is done.

        // Inlining the allocator into the construct_from call confuses LCOV.
        auto tmp = other.cb_->move(update_alloc ? other.alloc_ : alloc_);
        reset();
        cb_ = tmp;
      }
    }

    if (update_alloc) {
      alloc_ = other.alloc_;
    }
    return *this;
  }

  //
  // Accessors.
  //

  [[nodiscard]] constexpr pointer operator->() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return cb_->p_;
  }

  [[nodiscard]] constexpr const_pointer operator->() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return cb_->p_;
  }

  [[nodiscard]] constexpr T& operator*() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *cb_->p_;
  }

  [[nodiscard]] constexpr const T& operator*() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *cb_->p_;
  }

  [[nodiscard]] constexpr bool valueless_after_move() const noexcept {
    return cb_ == nullptr;
  }

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  //
  // Modifiers.
  //

  constexpr void swap(polymorphic& other) noexcept(
      std::allocator_traits<A>::propagate_on_container_swap::value ||
      std::allocator_traits<A>::is_always_equal::value) {
    if constexpr (allocator_traits::propagate_on_container_swap::value) {
      // If allocators move with their allocated objects, we can swap both.
      std::swap(alloc_, other.alloc_);
      std::swap(cb_, other.cb_);
      return;
    } else /* constexpr */ {
      if (alloc_ == other.alloc_) {
        std::swap(cb_, other.cb_);
      } else {
        unreachable();  // LCOV_EXCL_LINE
      }
    }
  }

  friend constexpr void swap(polymorphic& lhs, polymorphic& rhs) noexcept(
      noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

 private:
  constexpr void reset() noexcept {
    if (cb_ != nullptr) {
      cb_->destroy(alloc_);
      cb_ = nullptr;
    }
  }
};

}  // namespace xyz

#endif  // XYZ_POLYMORPHIC_H_
