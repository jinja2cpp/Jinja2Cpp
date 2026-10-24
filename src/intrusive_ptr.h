#ifndef JINJA2CPP_SRC_INTRUSIVE_PTR_H
#define JINJA2CPP_SRC_INTRUSIVE_PTR_H

#include <cstddef>
#include <type_traits>

namespace jinja2
{
inline namespace v2
{

// A pointer that shares an object counting its own references, as boost::intrusive_ptr
// does (docs/tasks/0230, step D4): one word, and the count lives in the pointee. The
// pointee provides intrusive_ptr_add_ref(const T*) and intrusive_ptr_release(const T*),
// found by argument-dependent lookup; release destroys the object with its last reference.
template<typename T>
class IntrusivePtr
{
public:
    using element_type = T;

    constexpr IntrusivePtr() noexcept = default;
    constexpr IntrusivePtr(std::nullptr_t) noexcept {} // NOLINT(google-explicit-constructor): a null pointer converts, as for std::shared_ptr
    // Takes a reference to the object; with addRef false, adopts one its caller holds
    explicit IntrusivePtr(T* ptr, bool addRef = true) noexcept
        : m_ptr(ptr)
    {
        if (m_ptr && addRef)
        {
            intrusive_ptr_add_ref(m_ptr);
        }
    }
    IntrusivePtr(const IntrusivePtr& other) noexcept
        : m_ptr(other.m_ptr)
    {
        if (m_ptr)
        {
            intrusive_ptr_add_ref(m_ptr);
        }
    }
    IntrusivePtr(IntrusivePtr&& other) noexcept
        : m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr;
    }
    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    IntrusivePtr(const IntrusivePtr<U>& other) noexcept // NOLINT(google-explicit-constructor): derived to base, as for raw pointers
        : m_ptr(other.get())
    {
        if (m_ptr)
        {
            intrusive_ptr_add_ref(m_ptr);
        }
    }
    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    IntrusivePtr(IntrusivePtr<U>&& other) noexcept // NOLINT(google-explicit-constructor, cppcoreguidelines-rvalue-reference-param-not-moved): derived to base; takes over the reference
        : m_ptr(other.detach())
    {
    }
    ~IntrusivePtr()
    {
        if (m_ptr)
        {
            intrusive_ptr_release(m_ptr);
        }
    }

    IntrusivePtr& operator=(const IntrusivePtr& other) noexcept
    {
        IntrusivePtr(other).swap(*this);
        return *this;
    }
    IntrusivePtr& operator=(IntrusivePtr&& other) noexcept
    {
        IntrusivePtr(static_cast<IntrusivePtr&&>(other)).swap(*this);
        return *this;
    }
    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    IntrusivePtr& operator=(const IntrusivePtr<U>& other) noexcept
    {
        IntrusivePtr(other).swap(*this);
        return *this;
    }
    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    IntrusivePtr& operator=(IntrusivePtr<U>&& other) noexcept // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved): moved by the cast, as boost::intrusive_ptr does
    {
        IntrusivePtr(static_cast<IntrusivePtr<U>&&>(other)).swap(*this);
        return *this;
    }
    IntrusivePtr& operator=(std::nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    void reset() noexcept { IntrusivePtr().swap(*this); }
    void reset(T* ptr, bool addRef = true) noexcept { IntrusivePtr(ptr, addRef).swap(*this); }
    // Gives up the reference without releasing it: the caller owns it from here on
    // NOLINTNEXTLINE(readability-identifier-naming): boost::intrusive_ptr's name
    [[nodiscard]] T* detach() noexcept
    {
        T* ptr = m_ptr;
        m_ptr = nullptr;
        return ptr;
    }
    // Plain assignments rather than std::swap: they keep the inliner's estimate of every
    // function that moves a value as small as boost::intrusive_ptr's did
    void swap(IntrusivePtr& other) noexcept
    {
        T* ptr = m_ptr;
        m_ptr = other.m_ptr;
        other.m_ptr = ptr;
    }

    [[nodiscard]] T* get() const noexcept { return m_ptr; }
    T& operator*() const noexcept { return *m_ptr; }
    T* operator->() const noexcept { return m_ptr; }
    explicit operator bool() const noexcept { return m_ptr != nullptr; }

private:
    T* m_ptr = nullptr;
};

template<typename T, typename U>
bool operator==(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) noexcept
{
    return lhs.get() == rhs.get();
}
template<typename T, typename U>
bool operator!=(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) noexcept
{
    return lhs.get() != rhs.get();
}
template<typename T>
bool operator==(const IntrusivePtr<T>& lhs, std::nullptr_t) noexcept
{
    return !lhs;
}
template<typename T>
bool operator==(std::nullptr_t, const IntrusivePtr<T>& rhs) noexcept
{
    return !rhs;
}
template<typename T>
bool operator!=(const IntrusivePtr<T>& lhs, std::nullptr_t) noexcept
{
    return static_cast<bool>(lhs);
}
template<typename T>
bool operator!=(std::nullptr_t, const IntrusivePtr<T>& rhs) noexcept
{
    return static_cast<bool>(rhs);
}

template<typename T>
void swap(IntrusivePtr<T>& lhs, IntrusivePtr<T>& rhs) noexcept
{
    lhs.swap(rhs);
}

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_INTRUSIVE_PTR_H
