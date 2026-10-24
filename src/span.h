#ifndef JINJA2CPP_SRC_SPAN_H
#define JINJA2CPP_SRC_SPAN_H

#include <array>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace jinja2
{
inline namespace v2
{

template<typename T>
class Span;

namespace detail
{
template<typename T>
struct IsSpan : std::false_type
{
};

template<typename T>
struct IsSpan<Span<T>> : std::true_type
{
};

template<typename T>
struct IsStdArray : std::false_type
{
};

template<typename T, std::size_t n>
struct IsStdArray<std::array<T, n>> : std::true_type
{
};

template<typename R, typename T, typename = void>
struct IsSpanRange : std::false_type
{
};

// A contiguous range Span<T> may view, as std::span's range constructor accepts: its
// elements convert to T by a qualification conversion only, and a span of mutable
// elements does not bind to a temporary. Spans, arrays and C arrays have constructors
// of their own.
template<typename R, typename T>
struct IsSpanRange<R, T, std::void_t<decltype(std::data(std::declval<R&>())), decltype(std::size(std::declval<R&>()))>>
    : std::bool_constant<!IsSpan<std::remove_cv_t<std::remove_reference_t<R>>>::value && !IsStdArray<std::remove_cv_t<std::remove_reference_t<R>>>::value && !std::is_array_v<std::remove_reference_t<R>> && std::is_convertible_v<std::remove_pointer_t<decltype(std::data(std::declval<R&>()))> (*)[], T (*)[]> && (std::is_lvalue_reference_v<R> || std::is_const_v<T>)>
{
};
} // namespace detail

// A view of contiguous elements: std::span with dynamic extent, which C++17 lacks
// (docs/tasks/0230, step D4). Two words, trivially copyable. Element access is checked by
// assertions in builds without NDEBUG.
template<typename T>
class Span
{
public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using reverse_iterator = std::reverse_iterator<iterator>;

    constexpr Span() noexcept = default;
    constexpr Span(T* data, size_type size) noexcept
        : m_data(data)
        , m_size(size)
    {
    }
    constexpr Span(T* first, T* last) noexcept
        : m_data(first)
        , m_size(static_cast<size_type>(last - first))
    {
        assert(first <= last);
    }
    template<std::size_t n>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays): std::span's C array constructor
    constexpr Span(T (&array)[n]) noexcept // NOLINT(google-explicit-constructor): implicit, as std::span's
        : m_data(array)
        , m_size(n)
    {
    }
    template<typename U, std::size_t n, typename = std::enable_if_t<std::is_convertible_v<U (*)[], T (*)[]>>>
    constexpr Span(std::array<U, n>& array) noexcept // NOLINT(google-explicit-constructor): implicit, as std::span's
        : m_data(array.data())
        , m_size(n)
    {
    }
    template<typename U, std::size_t n, typename = std::enable_if_t<std::is_convertible_v<const U (*)[], T (*)[]>>>
    constexpr Span(const std::array<U, n>& array) noexcept // NOLINT(google-explicit-constructor): implicit, as std::span's
        : m_data(array.data())
        , m_size(n)
    {
    }
    template<typename R, typename = std::enable_if_t<detail::IsSpanRange<R, T>::value>>
    constexpr Span(R&& range) noexcept // NOLINT(google-explicit-constructor, bugprone-forwarding-reference-overload, cppcoreguidelines-missing-std-forward): views the range, as std::span's
        : m_data(std::data(range))
        , m_size(std::size(range))
    {
    }
    template<typename U, typename = std::enable_if_t<!std::is_same_v<U, T> && std::is_convertible_v<U (*)[], T (*)[]>>>
    constexpr Span(const Span<U>& other) noexcept // NOLINT(google-explicit-constructor): Span<T> to Span<const T>
        : m_data(other.data())
        , m_size(other.size())
    {
    }

    [[nodiscard]] constexpr T* data() const noexcept { return m_data; }
    [[nodiscard]] constexpr size_type size() const noexcept { return m_size; }
    // NOLINTNEXTLINE(readability-identifier-naming): std::span's name
    [[nodiscard]] constexpr size_type size_bytes() const noexcept { return m_size * sizeof(T); }
    [[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }

    [[nodiscard]] constexpr T& operator[](size_type index) const noexcept
    {
        assert(index < m_size);
        return m_data[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    [[nodiscard]] constexpr T& front() const noexcept
    {
        assert(m_size != 0);
        return *m_data;
    }
    [[nodiscard]] constexpr T& back() const noexcept
    {
        assert(m_size != 0);
        return m_data[m_size - 1]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    [[nodiscard]] constexpr iterator begin() const noexcept { return m_data; }
    [[nodiscard]] constexpr iterator end() const noexcept { return m_data + m_size; } // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    [[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    [[nodiscard]] constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

    // NOLINTNEXTLINE(readability-identifier-naming): std::span's name
    [[nodiscard]] constexpr Span first(size_type count) const noexcept
    {
        assert(count <= m_size);
        return { m_data, count };
    }
    // NOLINTNEXTLINE(readability-identifier-naming): std::span's name
    [[nodiscard]] constexpr Span last(size_type count) const noexcept
    {
        assert(count <= m_size);
        return { m_data + (m_size - count), count }; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    // A count past the end means the rest, as std::dynamic_extent does
    // NOLINTNEXTLINE(readability-identifier-naming): std::span's name
    [[nodiscard]] constexpr Span subspan(size_type offset, size_type count = static_cast<size_type>(-1)) const noexcept
    {
        assert(offset <= m_size);
        assert(count == static_cast<size_type>(-1) || count <= m_size - offset);
        return { m_data + offset, count == static_cast<size_type>(-1) ? m_size - offset : count }; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

private:
    T* m_data = nullptr;
    size_type m_size = 0;
};

template<typename T, std::size_t n>
Span(T (&)[n]) -> Span<T>; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
template<typename T, std::size_t n>
Span(std::array<T, n>&) -> Span<T>;
template<typename T, std::size_t n>
Span(const std::array<T, n>&) -> Span<const T>;
template<typename R>
Span(R&&) -> Span<std::remove_pointer_t<decltype(std::data(std::declval<R&>()))>>;

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_SPAN_H
