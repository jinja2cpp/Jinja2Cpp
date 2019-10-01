#ifndef JINJA2CPP_GENERIC_LIST_IMPL_H
#define JINJA2CPP_GENERIC_LIST_IMPL_H

#include "generic_list.h"
#include "value.h"

#include <iterator>

namespace jinja2
{
namespace lists_impl
{
template<typename It1, typename It2>
struct InputIteratorListAccessor : ListItemAccessor
{
    mutable It1 m_begin;
    mutable It2 m_end;

    struct Enumerator : public ListEnumerator
    {
        It1* m_cur;
        It2* m_end;
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
                ++ *m_cur;

            return (*m_cur) != (*m_end);
        }

        Value GetCurrent() const override
        {
            return Reflect(**m_cur);
        }

        ListEnumeratorPtr Clone() const override
        {
            auto result = MakeEnumerator<Enumerator>(m_cur, m_end);
            auto ptr = static_cast<Enumerator*>(result.get());
            ptr->m_cur = m_cur;
            ptr->m_justInited = m_justInited;
            return result;
        }

        ListEnumeratorPtr Move() override
        {
            return MakeEnumerator<Enumerator>(std::move(*this));
        }
    };

    explicit InputIteratorListAccessor(It1&& b, It2&& e) noexcept
        : m_begin(std::move(b))
        , m_end(std::move(e))
    {
    }

    nonstd::optional<size_t> GetSize() const override
    {
        return nonstd::optional<size_t>();
    }

    const IndexBasedAccessor* GetIndexer() const override
    {
        return nullptr;
    }

    ListEnumeratorPtr CreateEnumerator() const override
    {
        return MakeEnumerator<Enumerator>(&m_begin, &m_end  );
    }

};

template<typename It1, typename It2>
struct ForwardIteratorListAccessor : ListItemAccessor
{
    It1 m_begin;
    It2 m_end;

    struct Enumerator : public ListEnumerator
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
                ++ m_cur;

            return m_cur != m_end;
        }

        Value GetCurrent() const override
        {
            return Reflect(*m_cur);
        }

        ListEnumeratorPtr Clone() const override
        {
            auto result = MakeEnumerator<Enumerator>(m_cur, m_end);
            auto ptr = static_cast<Enumerator*>(result.get());
            ptr->m_begin = m_cur;
            ptr->m_cur = m_cur;
            ptr->m_justInited = m_justInited;
            return result;
        }

        ListEnumeratorPtr Move() override
        {
            return MakeEnumerator<Enumerator>(std::move(*this));
        }
    };

    explicit ForwardIteratorListAccessor(It1&& b, It2&& e) noexcept
        : m_begin(std::move(b))
        , m_end(std::move(e))
    {
    }

    nonstd::optional<size_t> GetSize() const override
    {
        return nonstd::optional<size_t>();
    }

    const IndexBasedAccessor* GetIndexer() const override
    {
        return nullptr;
    }

    ListEnumeratorPtr CreateEnumerator() const override
    {
        return MakeEnumerator<Enumerator>(m_begin, m_end);
    }

};

template<typename It1, typename It2>
struct RandomIteratorListAccessor : ListItemAccessor, IndexBasedAccessor
{
    It1 m_begin;
    It2 m_end;

    struct Enumerator : public ListEnumerator
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
                ++ m_cur;

            return m_cur != m_end;
        }

        Value GetCurrent() const override
        {
            return Reflect(*m_cur);
        }

        ListEnumeratorPtr Clone() const override
        {
            auto result = MakeEnumerator<Enumerator>(m_cur, m_end);
            auto ptr = static_cast<Enumerator*>(result.get());
            ptr->m_begin = m_cur;
            ptr->m_cur = m_cur;
            ptr->m_justInited = m_justInited;
            return result;
        }

        ListEnumeratorPtr Move() override
        {
            return MakeEnumerator<Enumerator>(std::move(*this));
        }
    };

    explicit RandomIteratorListAccessor(It1 b, It2 e) noexcept
        : m_begin(std::move(b))
        , m_end(std::move(e))
    {
    }

    nonstd::optional<size_t> GetSize() const override
    {
        return std::distance(m_begin, m_end);
    }

    const IndexBasedAccessor* GetIndexer() const override
    {
        return this;
    }

    ListEnumeratorPtr CreateEnumerator() const override
    {
        return MakeEnumerator<Enumerator>(m_begin, m_end);
    }


    Value GetItemByIndex(int64_t idx) const override
    {
        auto p = m_begin;
        std::advance(p, static_cast<size_t>(idx));
        return Reflect(*p);
    }

};

using ListGenerator = std::function<nonstd::optional<Value>()>;

class GeneratedListAccessor : public ListItemAccessor
{
public:
    class Enumerator : public ListEnumerator
    {
    public:
        Enumerator(const ListGenerator* fn)
            : m_fn(fn)
        { }

        void Reset() override
        {
        }

        bool MoveNext() override
        {
            if (m_isFinished)
                return false;

            auto res = (*m_fn)();
            if (!res)
                return false;

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

    protected:
        const ListGenerator* m_fn;
        Value m_current;
        bool m_isFinished = false;
    };

    explicit GeneratedListAccessor(ListGenerator&& fn) : m_fn(std::move(fn)) {}

    nonstd::optional<size_t> GetSize() const override
    {
        return nonstd::optional<size_t>();
    }
    const IndexBasedAccessor* GetIndexer() const override
    {
        return nullptr;
    }

    ListEnumeratorPtr CreateEnumerator() const override
    {
        return MakeEnumerator<Enumerator>(&m_fn);
    }
private:
    ListGenerator m_fn;
};

template<typename It1, typename It2>
auto MakeGenericList(It1&& it1, It2&& it2, std::input_iterator_tag)
{
    return GenericList([accessor = InputIteratorListAccessor<It1, It2>(std::forward<It1>(it1), std::forward<It2>(it2))]() {return &accessor;});
}

template<typename It1, typename It2>
auto MakeGenericList(It1&& it1, It2&& it2, std::random_access_iterator_tag)
{
    return GenericList([accessor = RandomIteratorListAccessor<It1, It2>(std::forward<It1>(it1), std::forward<It2>(it2))]() {return &accessor;});
}

template<typename It1, typename It2, typename Category>
auto MakeGenericList(It1&& it1, It2&& it2, Category)
{
    return GenericList([accessor = ForwardIteratorListAccessor<It1, It2>(std::forward<It1>(it1), std::forward<It2>(it2))]() {return &accessor;});
}

auto MakeGeneratedList(ListGenerator&& fn)
{
    return GenericList([accessor = GeneratedListAccessor(std::move(fn))]() {return &accessor;});
}
}

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
auto MakeGenericList(It1&& it1, It2&& it2)
{
    return lists_impl::MakeGenericList(std::forward<It1>(it1), std::forward<It2>(it2), typename std::iterator_traits<It1>::iterator_category());
}

/*!
 * \brief Create instance of the GenericList from the generator method (generator-based generic list)
 *
 * List generator method should follow the function signature: nonstd::optional<Value>() . Non-empty optional returned from the generator means that generated
 * list isn't empty yet. The first returned empty optional object means the end of the generated sequence. For instance:
 * ```
 * jinja2::MakeGenericList([cur = 10]() mutable -> nonstd::optional<Value> {
 *          if (cur > 90)
 *              return nonstd::optional<Value>();
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
auto MakeGenericList(lists_impl::ListGenerator fn)
{
    return lists_impl::MakeGeneratedList(std::move(fn));
}

}

#endif //JINJA2CPP_GENERIC_LIST_IMPL_H
