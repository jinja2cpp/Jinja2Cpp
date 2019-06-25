#ifndef JINJA2_GENERIC_LIST_ITERATOR_H
#define JINJA2_GENERIC_LIST_ITERATOR_H

#include "generic_list.h"
#include "value.h"
#include "value_ptr.hpp"

namespace jinja2
{

class GenericListIterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = const Value;
    using difference_type = std::ptrdiff_t;
    using reference = const Value&;
    using pointer = const Value*;

    struct Cloner
    {
        ListEnumerator* operator()(const ListEnumerator &x) const
        {
            return x.Clone().release();
        }

        ListEnumerator* operator()(ListEnumerator &&x) const
        {
            return x.Move().release();
        }
    };

    using EnumeratorPtr = nonstd::value_ptr<ListEnumerator, Cloner>;
    
    GenericListIterator(ListEnumerator* e = nullptr)
        : m_enumerator(e)
    {
        if (m_enumerator)
            m_hasValue = m_enumerator->MoveNext();

        if (m_hasValue)
            m_current = std::move(m_enumerator->GetCurrent());
    }

    bool operator == (const GenericListIterator& other) const
    {
        if (!this->m_enumerator)
            return !other.m_enumerator ? true : other == *this;

        if (!other.m_enumerator)
            return !m_hasValue;

        return this->m_enumerator.get() == other.m_enumerator.get();
    }

    bool operator != (const GenericListIterator& other) const
    {
        return !(*this == other);
    }

    reference operator *() const
    {
        return m_current;
    }

    GenericListIterator& operator ++()
    {
        m_hasValue = m_enumerator->MoveNext();
        if (m_hasValue)
            m_current = std::move(m_enumerator->GetCurrent());

        return *this;
    }

    GenericListIterator operator++(int)
    {
        GenericListIterator result(std::move(m_current));

        this->operator++();
        return result;
    }
private:
    explicit GenericListIterator(Value&& val)
        : m_hasValue(true)
        , m_current(std::move(val))
    {

    }
    
private:
    const EnumeratorPtr m_enumerator;
    bool m_hasValue = false;
    Value m_current;
};

inline GenericListIterator GenericList::begin() const
{
    return m_accessor && m_accessor() ? GenericListIterator(m_accessor()->CreateEnumerator().release()) : GenericListIterator();
}
    
inline GenericListIterator GenericList::end() const
{
    return GenericListIterator();
}

inline auto GenericList::cbegin() const {return begin();}
inline auto GenericList::cend() const {return end();}
}

#endif // JINJA2_GENERIC_LIST_ITERATOR_H