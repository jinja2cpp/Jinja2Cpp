#ifndef JINJA2CPP_SRC_SMALL_VECTOR_H
#define JINJA2CPP_SRC_SMALL_VECTOR_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

// The growth paths stay out of line, as in boost::container: inlined into every insertion
// they used up the compiler's inlining budget for the hot code around it
#if defined(_MSC_VER) && !defined(__clang__)
#define JINJA2CPP_SMALL_VECTOR_NOINLINE __declspec(noinline)
#else
#define JINJA2CPP_SMALL_VECTOR_NOINLINE __attribute__((noinline))
#endif

namespace jinja2
{
inline namespace v2
{

// Growth policy that doubles the capacity instead of the default 1.6x.
struct SmallVectorDoubling
{
};

template<typename T, std::size_t N, typename Options = void>
class SmallVector;

// A vector that keeps its first elements in a buffer inside the object (docs/tasks/0230,
// step D5): boost::container::small_vector's layout and growth, without Boost.
//
// SmallVectorBase<T> is the capacity-erased part of every SmallVector<T, N>, for functions
// that take any of them (with the same growth Options). It holds the element pointer, the size and the capacity; the inline
// buffer follows it in SmallVector<T, N>, at the first offset aligned for T, as in
// boost::container and llvm::SmallVector. That fixed offset is how the base tells its
// inline buffer from a heap one without storing a flag. The base cannot be created, copied
// or destroyed on its own.
template<typename T, typename Options = void>
class SmallVectorBase
{
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    SmallVectorBase(const SmallVectorBase&) = delete;
    SmallVectorBase(SmallVectorBase&&) = delete;

    SmallVectorBase& operator=(const SmallVectorBase& other)
    {
        if (this != &other)
        {
            assign(other.begin(), other.end());
        }
        return *this;
    }
    // Moving needs the source's inline capacity, which only SmallVector knows
    SmallVectorBase& operator=(SmallVectorBase&& other) = delete;
    SmallVectorBase& operator=(std::initializer_list<T> values)
    {
        assign(values.begin(), values.end());
        return *this;
    }

    [[nodiscard]] iterator begin() noexcept { return m_data; }
    [[nodiscard]] const_iterator begin() const noexcept { return m_data; }
    [[nodiscard]] const_iterator cbegin() const noexcept { return m_data; }
    [[nodiscard]] iterator end() noexcept { return m_data + m_size; }             // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    [[nodiscard]] const_iterator end() const noexcept { return m_data + m_size; } // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    [[nodiscard]] const_iterator cend() const noexcept { return end(); }
    [[nodiscard]] reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    [[nodiscard]] reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    [[nodiscard]] const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

    [[nodiscard]] size_type size() const noexcept { return m_size; }
    [[nodiscard]] bool empty() const noexcept { return m_size == 0; }
    // NOLINTNEXTLINE(readability-identifier-naming): std::vector's name
    [[nodiscard]] size_type capacity() const noexcept { return m_capacity; }
    [[nodiscard]] static constexpr size_type max_size() noexcept { return std::numeric_limits<difference_type>::max() / sizeof(T); }
    [[nodiscard]] T* data() noexcept { return m_data; }
    [[nodiscard]] const T* data() const noexcept { return m_data; }

    [[nodiscard]] T& operator[](size_type index) noexcept
    {
        assert(index < m_size);
        return m_data[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    [[nodiscard]] const T& operator[](size_type index) const noexcept
    {
        assert(index < m_size);
        return m_data[index]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    [[nodiscard]] T& at(size_type index)
    {
        if (index >= m_size)
        {
            throw std::out_of_range("SmallVector::at");
        }
        return (*this)[index];
    }
    [[nodiscard]] const T& at(size_type index) const
    {
        if (index >= m_size)
        {
            throw std::out_of_range("SmallVector::at");
        }
        return (*this)[index];
    }
    [[nodiscard]] T& front() noexcept { return (*this)[0]; }
    [[nodiscard]] const T& front() const noexcept { return (*this)[0]; }
    [[nodiscard]] T& back() noexcept { return (*this)[m_size - 1]; }
    [[nodiscard]] const T& back() const noexcept { return (*this)[m_size - 1]; }

    template<typename... Args>
    // NOLINTNEXTLINE(readability-identifier-naming): std::vector's name
    T& emplace_back(Args&&... args)
    {
        if (m_size == m_capacity)
        {
            return *GrowAndEmplaceBack(std::forward<Args>(args)...);
        }
        T* slot = end();
        ::new (static_cast<void*>(slot)) T(std::forward<Args>(args)...);
        ++m_size;
        return *slot;
    }
    void push_back(const T& value) { emplace_back(value); }
    void push_back(T&& value) { emplace_back(std::move(value)); }
    // NOLINTNEXTLINE(readability-identifier-naming): std::vector's name
    void pop_back() noexcept
    {
        assert(m_size != 0);
        --m_size;
        std::destroy_at(end());
    }

    template<typename... Args>
    iterator emplace(const_iterator pos, Args&&... args)
    {
        const auto index = static_cast<size_type>(pos - begin());
        assert(index <= m_size);
        // Appending first keeps an argument that names an element of this vector valid
        emplace_back(std::forward<Args>(args)...);
        std::rotate(begin() + index, end() - 1, end()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return begin() + index;                         // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    iterator insert(const_iterator pos, const T& value) { return emplace(pos, value); }
    iterator insert(const_iterator pos, T&& value) { return emplace(pos, std::move(value)); }
    iterator insert(const_iterator pos, size_type count, const T& value)
    {
        const auto index = static_cast<size_type>(pos - begin());
        assert(index <= m_size);
        const size_type oldSize = m_size;
        if (count != 0)
        {
            // A copy, for a value that names an element of this vector
            T copy(value);
            reserve(GrowthFor(count));
            std::uninitialized_fill_n(end(), count, copy);
            m_size += count;
            std::rotate(begin() + index, begin() + oldSize, end()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
        return begin() + index; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    template<typename It, typename = std::enable_if_t<!std::is_integral_v<It>>>
    iterator insert(const_iterator pos, It first, It last)
    {
        const auto index = static_cast<size_type>(pos - begin());
        assert(index <= m_size);
        const size_type oldSize = m_size;
        Append(first, last);
        std::rotate(begin() + index, begin() + oldSize, end()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return begin() + index;                                 // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    iterator insert(const_iterator pos, std::initializer_list<T> values) { return insert(pos, values.begin(), values.end()); }

    iterator erase(const_iterator pos) { return erase(pos, pos + 1); } // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    iterator erase(const_iterator first, const_iterator last)
    {
        assert(begin() <= first && first <= last && last <= end());
        auto* from = begin() + (first - begin()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto* to = begin() + (last - begin());    // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (from != to)
        {
            auto* newEnd = std::move(to, end(), from);
            std::destroy(newEnd, end());
            m_size = static_cast<size_type>(newEnd - begin());
        }
        return from;
    }
    void clear() noexcept
    {
        std::destroy(begin(), end());
        m_size = 0;
    }

    void reserve(size_type capacity)
    {
        if (capacity > max_size())
        {
            throw std::length_error("SmallVector: too many elements");
        }
        if (capacity > m_capacity)
        {
            Reallocate(capacity);
        }
    }
    // NOLINTNEXTLINE(readability-identifier-naming): std::vector's name
    void resize(size_type size)
    {
        ResizeImpl(size, [](T* first, size_type count) { std::uninitialized_value_construct_n(first, count); });
    }
    // NOLINTNEXTLINE(readability-identifier-naming): std::vector's name
    void resize(size_type size, const T& value)
    {
        if (size > m_size)
        {
            T copy(value);
            ResizeImpl(size, [&copy](T* first, size_type count) { std::uninitialized_fill_n(first, count, copy); });
        }
        else
        {
            ResizeImpl(size, nullptr);
        }
    }
    template<typename It, typename = std::enable_if_t<!std::is_integral_v<It>>>
    // NOLINTNEXTLINE(readability-identifier-naming): std::vector's name
    void assign(It first, It last)
    {
        clear();
        Append(first, last);
    }
    // NOLINTNEXTLINE(readability-identifier-naming): std::vector's name
    void assign(size_type count, const T& value)
    {
        T copy(value);
        clear();
        reserve(count);
        std::uninitialized_fill_n(begin(), count, copy);
        m_size = count;
    }
    // NOLINTNEXTLINE(readability-identifier-naming): std::vector's name
    void assign(std::initializer_list<T> values) { assign(values.begin(), values.end()); }

    friend bool operator==(const SmallVectorBase& lhs, const SmallVectorBase& rhs) { return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()); }
    friend bool operator!=(const SmallVectorBase& lhs, const SmallVectorBase& rhs) { return !(lhs == rhs); }

protected:
    SmallVectorBase(T* inlineData, size_type inlineCapacity) noexcept
        : m_data(inlineData)
        , m_capacity(inlineCapacity)
    {
        assert(inlineData == InlineData());
    }
    ~SmallVectorBase()
    {
        std::destroy(begin(), end());
        ReleaseHeap();
    }

    template<typename It>
    void Append(It first, It last)
    {
        using Category = typename std::iterator_traits<It>::iterator_category;
        if constexpr (std::is_base_of_v<std::forward_iterator_tag, Category>)
        {
            const auto count = static_cast<size_type>(std::distance(first, last));
            reserve(GrowthFor(count));
            std::uninitialized_copy(first, last, end());
            m_size += count;
        }
        else
        {
            for (; first != last; ++first)
            {
                emplace_back(*first);
            }
        }
    }

    // Move construction: this vector is new and its inline buffer holds as many elements as
    // `other`'s (the same SmallVector type)
    void MoveConstructFrom(SmallVectorBase& other, size_type inlineCapacity) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (!other.IsInline())
        {
            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            other.m_data = other.InlineData();
            other.m_size = 0;
            other.m_capacity = inlineCapacity;
            return;
        }
        std::uninitialized_move(other.begin(), other.end(), begin());
        m_size = other.m_size;
        other.clear();
    }

    // Leaves `other` empty, with `otherInlineCapacity` as the capacity of its inline buffer
    void MoveFrom(SmallVectorBase& other, size_type otherInlineCapacity)
    {
        if (!other.IsInline())
        {
            clear();
            ReleaseHeap();
            m_data = std::exchange(other.m_data, other.InlineData());
            m_size = std::exchange(other.m_size, 0);
            m_capacity = std::exchange(other.m_capacity, otherInlineCapacity);
            return;
        }
        const size_type common = std::min(m_size, other.m_size);
        std::move(other.begin(), other.begin() + common, begin()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (other.m_size > m_size)
        {
            Append(std::make_move_iterator(other.begin() + common), std::make_move_iterator(other.end())); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }
        else
        {
            ResizeImpl(common, nullptr);
        }
        other.clear();
    }

    // Moves the elements back into the inline buffer of `inlineCapacity` when they fit, or
    // into a heap buffer of their size
    void ShrinkToFit(size_type inlineCapacity)
    {
        if (IsInline() || m_size == m_capacity)
        {
            return;
        }
        T* heap = m_data;
        const size_type heapCapacity = m_capacity;
        T* fresh = m_size <= inlineCapacity ? InlineData() : Allocate(m_size);
        try
        {
            RelocateTo(fresh);
        }
        catch (...)
        {
            if (fresh != InlineData())
            {
                Deallocate(fresh, m_size);
            }
            throw;
        }
        std::destroy(begin(), end());
        Deallocate(heap, heapCapacity);
        m_data = fresh;
        m_capacity = fresh == InlineData() ? inlineCapacity : m_size;
    }

private:
    [[nodiscard]] T* InlineData() const noexcept
    {
        // The inline buffer starts at the first offset past this base that is aligned for T
        constexpr std::size_t offset = (sizeof(SmallVectorBase) + alignof(T) - 1) / alignof(T) * alignof(T);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, cppcoreguidelines-pro-type-const-cast, cppcoreguidelines-pro-bounds-pointer-arithmetic): the buffer the derived SmallVector lays out there
        return reinterpret_cast<T*>(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(this)) + offset);
    }
    [[nodiscard]] bool IsInline() const noexcept { return m_data == InlineData(); }

    // The capacity that holds `extra` more elements: the growth factor applied to the
    // current capacity, or more if that is not enough
    [[nodiscard]] size_type GrowthFor(size_type extra) const
    {
        if (max_size() - m_size < extra)
        {
            throw std::length_error("SmallVector: too many elements");
        }
        const size_type needed = m_size + extra;
        if (needed <= m_capacity)
        {
            return m_capacity;
        }
        const size_type limit = max_size();
        size_type grown = 0;
        if constexpr (std::is_same_v<Options, SmallVectorDoubling>)
        {
            grown = m_capacity > limit / 2 ? limit : m_capacity * 2;
        }
        else
        {
            grown = m_capacity > limit / 8 * 5 ? limit : (m_capacity / 5 * 8) + (m_capacity % 5 * 8 / 5);
        }
        return std::max(grown, needed);
    }

    static T* Allocate(size_type capacity) { return std::allocator<T>().allocate(capacity); }
    static void Deallocate(T* data, size_type capacity) noexcept { std::allocator<T>().deallocate(data, capacity); }
    // Frees a heap buffer. The capacity of the inline buffer is not known here: the caller
    // sets the capacity
    void ReleaseHeap() noexcept
    {
        if (!IsInline())
        {
            Deallocate(m_data, m_capacity);
            m_data = InlineData();
            m_capacity = 0;
        }
    }

    // Moves the elements to a new heap buffer; copies them when moving may throw, so a
    // failure leaves the vector as it was
    JINJA2CPP_SMALL_VECTOR_NOINLINE void Reallocate(size_type capacity)
    {
        T* fresh = Allocate(capacity);
        try
        {
            RelocateTo(fresh);
        }
        catch (...)
        {
            Deallocate(fresh, capacity);
            throw;
        }
        AdoptBuffer(fresh, capacity);
    }

    template<typename... Args>
    JINJA2CPP_SMALL_VECTOR_NOINLINE T* GrowAndEmplaceBack(Args&&... args)
    {
        const size_type capacity = GrowthFor(1);
        T* fresh = Allocate(capacity);
        T* slot = fresh + m_size; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        try
        {
            // The new element first: an argument may name an element of this vector
            ::new (static_cast<void*>(slot)) T(std::forward<Args>(args)...);
        }
        catch (...)
        {
            Deallocate(fresh, capacity);
            throw;
        }
        try
        {
            RelocateTo(fresh);
        }
        catch (...)
        {
            std::destroy_at(slot);
            Deallocate(fresh, capacity);
            throw;
        }
        AdoptBuffer(fresh, capacity);
        ++m_size;
        return slot;
    }

    void RelocateTo(T* fresh)
    {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
        {
            std::uninitialized_move(begin(), end(), fresh);
        }
        else
        {
            std::uninitialized_copy(begin(), end(), fresh);
        }
    }
    void AdoptBuffer(T* fresh, size_type capacity) noexcept
    {
        std::destroy(begin(), end());
        ReleaseHeap();
        m_data = fresh;
        m_capacity = capacity;
    }

    template<typename Construct>
    void ResizeImpl(size_type size, Construct construct)
    {
        if (size < m_size)
        {
            std::destroy(begin() + size, end()); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            m_size = size;
        }
        else if (size > m_size)
        {
            if constexpr (!std::is_same_v<Construct, std::nullptr_t>)
            {
                reserve(GrowthFor(size - m_size));
                construct(end(), size - m_size);
                m_size = size;
            }
        }
    }

    T* m_data;
    size_type m_size = 0;
    size_type m_capacity;
};

namespace detail
{
// The inline buffer of a SmallVector, laid out right after its SmallVectorBase
template<typename T, std::size_t N>
struct SmallVectorStorage
{
    alignas(T) unsigned char m_inline[N * sizeof(T)]; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays, misc-non-private-member-variables-in-classes)
};
} // namespace detail

template<typename T, std::size_t N, typename Options>
class SmallVector
    : public SmallVectorBase<T, Options>
    , private detail::SmallVectorStorage<T, N>
{
    static_assert(N > 0, "a SmallVector keeps at least one element inline");
    static_assert(std::is_void_v<Options> || std::is_same_v<Options, SmallVectorDoubling>, "Options is void or SmallVectorDoubling");
    using Base = SmallVectorBase<T, Options>;
    using Storage = detail::SmallVectorStorage<T, N>;

public:
    using typename Base::size_type;

    SmallVector() noexcept
        : Base(InlineStart(), N)
    {
    }
    explicit SmallVector(size_type count)
        : SmallVector()
    {
        this->resize(count);
    }
    SmallVector(size_type count, const T& value)
        : SmallVector()
    {
        this->assign(count, value);
    }
    template<typename It, typename = std::enable_if_t<!std::is_integral_v<It>>>
    SmallVector(It first, It last)
        : SmallVector()
    {
        this->Append(first, last);
    }
    SmallVector(std::initializer_list<T> values)
        : SmallVector()
    {
        this->Append(values.begin(), values.end());
    }
    SmallVector(const SmallVector& other)
        : SmallVector()
    {
        this->Append(other.begin(), other.end());
    }
    SmallVector(SmallVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : SmallVector()
    {
        this->MoveConstructFrom(other, N);
    }
    ~SmallVector() = default;

    SmallVector& operator=(const SmallVector& other)
    {
        Base::operator=(other);
        return *this;
    }
    SmallVector& operator=(SmallVector&& other) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
    {
        if (this != &other)
        {
            this->MoveFrom(other, N);
        }
        return *this;
    }
    SmallVector& operator=(std::initializer_list<T> values)
    {
        Base::operator=(values);
        return *this;
    }

    // Moves the elements back into the inline buffer when they fit, or into a heap buffer of
    // their size
    // NOLINTNEXTLINE(readability-identifier-naming): std::vector's name
    void shrink_to_fit() { this->ShrinkToFit(N); }

private:
    T* InlineStart() noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): storage for N elements of T
        return reinterpret_cast<T*>(static_cast<Storage*>(this)->m_inline);
    }
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_SMALL_VECTOR_H
