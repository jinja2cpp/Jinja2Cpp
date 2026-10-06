#ifndef JINJA2CPP_SRC_LOOKUP_RESULT_H
#define JINJA2CPP_SRC_LOOKUP_RESULT_H

#include "internal_value.h"

#include <cassert>

namespace jinja2
{
// Where a lookup found a value, or nothing: a nullable reference to a value stored elsewhere
// (a scope, a constant node), read in place instead of copied (docs/tasks/0088, 0117). It is
// valid while what holds the value is unchanged: for a variable, until a scope of the
// context next gains, loses or clears a name. It is 8 bytes, trivially copyable and returned
// in a register, so it costs what the pointer it replaces did, but it offers no arithmetic,
// no delete and no way back to a raw pointer.
template<typename T>
class [[nodiscard]] BasicLookupResult
{
public:
    BasicLookupResult() noexcept = default; // not found
    explicit BasicLookupResult(T& value) noexcept
        : m_value(&value)
    {
    }
    // A temporary would be gone before the result is read
    explicit BasicLookupResult(const T&&) = delete;

    explicit operator bool() const noexcept { return m_value != nullptr; }
    T& operator*() const
    {
        assert(m_value);
        return *m_value;
    }
    T* operator->() const
    {
        assert(m_value);
        return m_value;
    }
    // Whether both refer to the same stored value (or both to none)
    [[nodiscard]] bool IsSame(const BasicLookupResult& other) const noexcept { return m_value == other.m_value; }

private:
    T* m_value = nullptr;
};

// A value found for reading
using LookupResult = BasicLookupResult<const InternalValue>;
// A variable found for changing in place (docs/tasks/0020)
using MutableLookupResult = BasicLookupResult<InternalValue>;
} // namespace jinja2

#endif // JINJA2CPP_SRC_LOOKUP_RESULT_H
