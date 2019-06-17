#ifndef JINJA2_GENERIC_LIST_ITERATOR_H
#define JINJA2_GENERIC_LIST_ITERATOR_H

#include "generic_list.h"
#include "value.h"

namespace jinja2
{

class GenericListIterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = const Value;
    using difference_type = std::ptrdiff_t;
    using reference = const Value;
    using pointer = const Value*;
    
    GenericListIterator(ListEnumeratorPtr enumerator = ListEnumeratorPtr(nullptr, nullptr))
        : m_enumerator(std::move(enumerator))
    {}
    
private:
    const ListEnumeratorPtr m_enumerator;
};

inline auto GenericList::begin() const
{
    return m_accessor && m_accessor() ? GenericListIterator(std::move(m_accessor()->CreateEnumerator())) : GenericListIterator();
}
    
inline auto GenericList::end() const
{
    return GenericListIterator();
}

inline auto GenericList::cbegin() const {return begin();}
inline auto GenericList::cend() const {return end();}
}

#endif // JINJA2_GENERIC_LIST_ITERATOR_H