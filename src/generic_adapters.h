#ifndef GENERIC_ADAPTERS_H
#define GENERIC_ADAPTERS_H

#include <jinja2cpp/value.h>
#include "internal_value.h"

namespace jinja2
{
template<typename T>
class IndexedListItemAccessorImpl : public ListItemAccessor, public IndexBasedAccessor
{
public:
    using ThisType = IndexedListItemAccessorImpl<T>;

    class Enumerator : ListEnumerator
    {
    public:
        Enumerator(const ThisType* list)
            : m_list(list)
            , m_maxItems(list->GetSize().get())
        { }
        
        void Reset() override
        {
            m_curItem = m_invalidIndex;
        }
        
        bool MoveNext() override
        {
            if (m_curItem == m_invalidIndex)
                m_curItem = 0;
            else    
                ++ m_curItem;
                
            return m_curItem < m_maxItems;
        }
        
        Value GetCurrent() const override
        {
            return m_list->GetValueByIndex(m_curItem);
        }
        
    private:
        constexpr static auto m_invalidIndex = std::numeric_limits<size_t>::max();
        const ThisType* m_list;
        size_t m_curItem = m_invalidIndex;
        size_t m_maxItems;
    };

    Value GetItemByIndex(int64_t idx) const
    {
        return IntValue2Value(static_cast<const T*>(this)->GetItem(idx));
    }
    
    nonstd::optional<size_t> GetSize() const override
    {
        return static_cast<const T*>(this)->GetItemsCount();
    }
    
    const IndexBasedAccessor* GetIndexer() const override
    {
        return this;
    }
    
    ListEnumeratorPtr CreateEnumerator() const override
    {
        return MakeEnumerator<Enumerator>(this);
    }
    
};

template<typename T>
class IndexedListAccessorImpl : public IListAccessor, public IndexedListItemAccessorImpl<T>
{
public:
};

template<typename T>
class MapItemAccessorImpl : public MapItemAccessor
{
public:
    Value GetValueByName(const std::string& name) const
    {
        return IntValue2Value(static_cast<const T*>(this)->GetItem(name));
    }
};

template<typename T>
class MapAccessorImpl : public IMapAccessor, public MapItemAccessorImpl<T>
{
public:
};

} // jinja2

#endif // GENERIC_ADAPTERS_H
