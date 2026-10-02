#ifndef JINJA2CPP_MAKE_GENERIC_LIST_H
#define JINJA2CPP_MAKE_GENERIC_LIST_H

#include "generic_list.h"
#include "reflected_value.h"
#include "value.h"

#include <functional>
#include <iterator>
#include <optional>
#include <type_traits>

namespace jinja2
{
/*!
 * \brief Generator of a generated list, see \ref MakeGenericList(ListGenerator)
 *
 * Returns the next item, or an empty optional at the end of the sequence.
 */
using ListGenerator = std::function<std::optional<Value>()>;

namespace detail
{
template<typename It1, typename It2>
struct InputIteratorListAccessor : IListItemAccessor
{
    mutable It1 m_begin;
    mutable It2 m_end;

    struct Enumerator : public IListEnumerator
    {
        It1* m_cur = nullptr;
        It2* m_end = nullptr;
        bool m_justInited = true;

        Enumerator(It1* begin, It2* end)
            : m_cur(begin)
            , m_end(end)
        {}

        void Reset() override
        {
        }

        bool MoveNext() override
        {
            if (m_justInited)
                m_justInited = false;
            else
                ++*m_cur;

            return (*m_cur) != (*m_end);
        }

        Value GetCurrent() const override
        {
            return Reflect(**m_cur);
        }

        ListEnumeratorPtr Clone() const override
        {
            return MakeEnumerator<Enumerator>(*this);
        }

        ListEnumeratorPtr Move() override
        {
            return MakeEnumerator<Enumerator>(std::move(*this));
        }
        bool IsEqual(const IComparable& other) const override
        {
            auto* val = dynamic_cast<const Enumerator*>(&other);
            if (!val)
                return false;
            if (m_cur != val->m_cur)
                return false;
            if (m_end != val->m_end)
                return false;
            if (m_justInited != val->m_justInited)
                return false;
            return true;
        }
    };

    explicit InputIteratorListAccessor(It1 b, It2 e) noexcept
        : m_begin(std::move(b))
        , m_end(std::move(e))
    {
    }

    std::optional<size_t> GetSize() const override
    {
        return std::optional<size_t>();
    }

    const IIndexBasedAccessor* GetIndexer() const override
    {
        return nullptr;
    }

    std::optional<ListEnumeratorPtr> CreateEnumerator() const override
    {
        return MakeEnumerator<Enumerator>(&m_begin, &m_end);
    }

    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const InputIteratorListAccessor*>(&other);
        if (!val)
            return false;
        return m_begin == val->m_begin && m_end == val->m_end;
    }
};

template<typename It1, typename It2>
struct ForwardIteratorListAccessor : IListItemAccessor
{
    It1 m_begin;
    It2 m_end;

    struct Enumerator : public IListEnumerator
    {
        It1 m_begin;
        It1 m_cur;
        It2 m_end;
        bool m_justInited = true;

        Enumerator(It1 begin, It2 end)
            : m_begin(begin)
            , m_cur(end)
            , m_end(end)
        {}

        void Reset() override
        {
            m_justInited = true;
        }

        bool MoveNext() override
        {
            if (m_justInited)
            {
                m_cur = m_begin;
                m_justInited = false;
            }
            else
            {
                ++m_cur;
            }

            return m_cur != m_end;
        }

        Value GetCurrent() const override
        {
            return Reflect(*m_cur);
        }

        ListEnumeratorPtr Clone() const override
        {
            return MakeEnumerator<Enumerator>(*this);
        }

        ListEnumeratorPtr Move() override
        {
            return MakeEnumerator<Enumerator>(std::move(*this));
        }
        bool IsEqual(const IComparable& other) const override
        {
            auto* val = dynamic_cast<const Enumerator*>(&other);
            if (!val)
                return false;
            if (m_begin != val->m_begin)
                return false;
            if (m_cur != val->m_cur)
                return false;
            if (m_end != val->m_end)
                return false;
            if (m_justInited != val->m_justInited)
                return false;
            return true;
        }
    };

    explicit ForwardIteratorListAccessor(It1 b, It2 e) noexcept
        : m_begin(std::move(b))
        , m_end(std::move(e))
    {
    }

    std::optional<size_t> GetSize() const override
    {
        return std::optional<size_t>();
    }

    const IIndexBasedAccessor* GetIndexer() const override
    {
        return nullptr;
    }

    std::optional<ListEnumeratorPtr> CreateEnumerator() const override
    {
        return MakeEnumerator<Enumerator>(m_begin, m_end);
    }
    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const ForwardIteratorListAccessor*>(&other);
        if (!val)
            return false;
        return m_begin == val->m_begin && m_end == val->m_end;
    }
};

template<typename It1, typename It2>
struct RandomIteratorListAccessor : IListItemAccessor
    , IIndexBasedAccessor
{
    It1 m_begin;
    It2 m_end;

    struct Enumerator : public IListEnumerator
    {
        It1 m_begin;
        It1 m_cur;
        It2 m_end;
        bool m_justInited = true;

        Enumerator(It1 begin, It2 end)
            : m_begin(begin)
            , m_cur(end)
            , m_end(end)
        {}

        void Reset() override
        {
            m_justInited = true;
        }

        bool MoveNext() override
        {
            if (m_justInited)
            {
                m_cur = m_begin;
                m_justInited = false;
            }
            else
            {
                ++m_cur;
            }

            return m_cur != m_end;
        }

        Value GetCurrent() const override
        {
            return Reflect(*m_cur);
        }

        ListEnumeratorPtr Clone() const override
        {
            return MakeEnumerator<Enumerator>(*this);
        }

        ListEnumeratorPtr Move() override
        {
            return MakeEnumerator<Enumerator>(std::move(*this));
        }
        bool IsEqual(const IComparable& other) const override
        {
            auto* val = dynamic_cast<const Enumerator*>(&other);
            if (!val)
                return false;
            if (m_begin != val->m_begin)
                return false;
            if (m_cur != val->m_cur)
                return false;
            if (m_end != val->m_end)
                return false;
            if (m_justInited != val->m_justInited)
                return false;
            return true;
        }
    };

    explicit RandomIteratorListAccessor(It1 b, It2 e) noexcept
        : m_begin(std::move(b))
        , m_end(std::move(e))
    {
    }

    std::optional<size_t> GetSize() const override
    {
        return std::distance(m_begin, m_end);
    }

    const IIndexBasedAccessor* GetIndexer() const override
    {
        return this;
    }

    std::optional<ListEnumeratorPtr> CreateEnumerator() const override
    {
        return MakeEnumerator<Enumerator>(m_begin, m_end);
    }


    Value GetItemByIndex(int64_t idx) const override
    {
        auto p = m_begin;
        std::advance(p, static_cast<size_t>(idx));
        return Reflect(*p);
    }

    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const RandomIteratorListAccessor*>(&other);
        if (!val)
            return false;
        return m_begin == val->m_begin && m_end == val->m_end;
    }
};

class GeneratedListAccessor : public IListItemAccessor
{
public:
    class Enumerator : public IListEnumerator
    {
    public:
        Enumerator(const ListGenerator* fn)
            : m_fn(fn)
        {}

        void Reset() override
        {
        }

        bool MoveNext() override
        {
            if (m_isFinished)
                return false;

            auto res = (*m_fn)();
            if (!res)
            {
                m_isFinished = true;
                return false;
            }

            m_current = std::move(*res);

            return true;
        }

        Value GetCurrent() const override { return m_current; }

        ListEnumeratorPtr Clone() const override
        {
            return MakeEnumerator<Enumerator>(*this);
        }

        ListEnumeratorPtr Move() override
        {
            return MakeEnumerator<Enumerator>(std::move(*this));
        }

        bool IsEqual(const IComparable& other) const override
        {
            const auto* val = dynamic_cast<const Enumerator*>(&other);
            if (!val)
                return false;
            return m_fn == val->m_fn && m_current == val->m_current && m_isFinished == val->m_isFinished;
        }
    protected:
        const ListGenerator* m_fn;
        Value m_current;
        bool m_isFinished = false;
    };

    explicit GeneratedListAccessor(ListGenerator&& fn)
        : m_fn(std::move(fn)) {}

    std::optional<size_t> GetSize() const override
    {
        return std::optional<size_t>();
    }
    const IIndexBasedAccessor* GetIndexer() const override
    {
        return nullptr;
    }

    std::optional<ListEnumeratorPtr> CreateEnumerator() const override
    {
        return MakeEnumerator<Enumerator>(&m_fn);
    }

    // IsEqual stays the identity from IComparable: calling the generators to compare them would advance them.

private:
    ListGenerator m_fn;
};

template<typename It1, typename It2>
GenericList MakeGenericList(It1 it1, It2 it2, std::input_iterator_tag)
{
    return GenericList([accessor = InputIteratorListAccessor<It1, It2>(std::move(it1), std::move(it2))]() { return &accessor; });
}

template<typename It1, typename It2>
GenericList MakeGenericList(It1 it1, It2 it2, std::random_access_iterator_tag)
{
    return GenericList([accessor = RandomIteratorListAccessor<It1, It2>(std::move(it1), std::move(it2))]() { return &accessor; });
}

template<typename It1, typename It2, typename Category>
GenericList MakeGenericList(It1 it1, It2 it2, Category)
{
    return GenericList([accessor = ForwardIteratorListAccessor<It1, It2>(std::move(it1), std::move(it2))]() { return &accessor; });
}

inline GenericList MakeGeneratedList(ListGenerator&& fn)
{
    return GenericList([accessor = GeneratedListAccessor(std::move(fn))]() { return &accessor; });
}
} // namespace detail

/*!
 * \brief Create instance of the GenericList from the pair of iterators
 *
 * @tparam It1 Type of the first (begin) iterator
 * @tparam It2 Type of the second (end) iterator
 * @param it1 First (begin) iterator
 * @param it2 Second (end) iterator
 * @return Instance of GenericList object for the provided pair of iterators
 */
template<typename It1, typename It2>
GenericList MakeGenericList(It1&& it1, It2&& it2)
{
    using Begin = std::decay_t<It1>;
    using End = std::decay_t<It2>;
    return detail::MakeGenericList<Begin, End>(
        std::forward<It1>(it1), std::forward<It2>(it2), typename std::iterator_traits<Begin>::iterator_category());
}

/*!
 * \brief Create instance of the GenericList from the generator method (generator-based generic list)
 *
 * List generator method should follow the function signature: std::optional<Value>() . Non-empty optional returned from the generator means that generated
 * list isn't empty yet. The first returned empty optional object means the end of the generated sequence. For instance:
 * ```
 * jinja2::MakeGenericList([cur = 10]() mutable -> std::optional<Value> {
 *          if (cur > 90)
 *              return std::optional<Value>();
 *
 *          auto tmp = cur;
 *          cur += 10;
 *          return Value(tmp);
   });
 * ```
 * This generator produces the following list: `[10, 20, 30, 40, 50, 60, 70, 80, 90]`
 *
 * @param fn Generator of the items sequences
 * @return Instance of GenericList object for the provided generation method
 */
inline GenericList MakeGenericList(ListGenerator fn)
{
    return detail::MakeGeneratedList(std::move(fn));
}

} // namespace jinja2

#endif // JINJA2CPP_MAKE_GENERIC_LIST_H
