#ifndef VALUE_HELPERS_H
#define VALUE_HELPERS_H

#include <jinja2cpp/value.h>

#include <boost/iterator/iterator_facade.hpp>

namespace jinja2
{
class GenericListIterator
        : public boost::iterator_facade<
            GenericListIterator,
            const Value,
            boost::random_access_traversal_tag>
{
public:
    GenericListIterator()
        : m_current(0)
        , m_list(nullptr)
    {}

    explicit GenericListIterator(GenericList& list)
        : m_current(0)
        , m_list(&list)
    {}

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        ++ m_current;
        m_valueIdx = m_current;
        m_currentVal = m_current == m_list->GetSize() ? Value() : m_list->GetValueByIndex(static_cast<int64_t>(m_current));
    }

    int distance_to(const GenericListIterator& other) const
    {
        if (m_list == nullptr)
            return other.m_list == nullptr ? 0 : -other.distance_to(*this);

        if (other.m_list == nullptr)
            return m_list->GetSize() - m_current;

        return other.m_current - m_current;
    }

    void advance(int distance)
    {
        m_current += distance;
        if (distance != 0)
        {
            m_valueIdx = m_current;
            m_currentVal = m_current == m_list->GetSize() ? Value() : m_list->GetValueByIndex(static_cast<int64_t>(m_current));

        }
    }

    bool equal(const GenericListIterator& other) const
    {
        if (m_list == nullptr)
            return other.m_list == nullptr ? true : other.equal(*this);

        if (other.m_list == nullptr)
            return m_current == m_list->GetSize();

        return this->m_list == other.m_list && this->m_current == other.m_current;
    }

    const Value& dereference() const
    {
        if (m_current != m_valueIdx)
            m_currentVal = m_current == m_list->GetSize() ? Value() : m_list->GetValueByIndex(static_cast<int64_t>(m_current));
        return m_currentVal;
    }

    size_t m_current;
    mutable size_t m_valueIdx;
    mutable Value m_currentVal;
    GenericList* m_list;
};

class ConstGenericListIterator
        : public boost::iterator_facade<
            GenericListIterator,
            const Value,
            boost::random_access_traversal_tag>
{
public:
    ConstGenericListIterator()
        : m_current(0)
        , m_list(nullptr)
    {}

    explicit ConstGenericListIterator(const GenericList& list)
        : m_current(0)
        , m_list(&list)
    {}

private:
    friend class boost::iterator_core_access;

    void increment()
    {
        ++ m_current;
    }

    int distance_to(const ConstGenericListIterator& other) const
    {
        if (m_list == nullptr)
            return other.m_list == nullptr ? 0 : -other.distance_to(*this);

        if (other.m_list == nullptr)
            return m_list->GetSize() - m_current;

        return other.m_current - m_current;
    }

    void advance(int distance)
    {
        m_current += distance;
    }

    bool equal(const ConstGenericListIterator& other) const
    {
        if (m_list == nullptr)
            return other.m_list == nullptr ? true : other.equal(*this);

        if (other.m_list == nullptr)
            return m_current == m_list->GetSize();

        return this->m_list == other.m_list && this->m_current == other.m_current;
    }

    const Value& dereference() const
    {
        return m_list->GetValueByIndex(static_cast<int64_t>(m_current));
    }

    size_t m_current;
    const GenericList* m_list;
};

inline auto begin(GenericList& list)
{
    return GenericListIterator(list);
}

inline auto end(GenericList& list)
{
    return GenericListIterator();
}

inline auto begin(const GenericList& list)
{
    return ConstGenericListIterator(list);
}

inline auto end(const GenericList& list)
{
    return ConstGenericListIterator();
}
} // jinja2

#endif // VALUE_HELPERS_H