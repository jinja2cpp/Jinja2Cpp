#ifndef JINJA2_GENERIC_LIST_H
#define JINJA2_GENERIC_LIST_H

#include <nonstd/optional.hpp>

#include <iterator>
#include <memory>

namespace jinja2
{
class Value;
    
struct IndexBasedAccessor
{
     virtual Value GetItemByIndex(int64_t idx) const = 0;
     virtual size_t GetItemsCount() const = 0;
};

struct ListEnumerator
{
    virtual ~ListEnumerator() {}
    
    virtual void Reset() = 0;
    
    virtual bool MoveNext() = 0;
    virtual Value GetCurrent() const = 0;
};

using ListEnumeratorPtr = std::unique_ptr<ListEnumerator, void (*)(ListEnumerator*)>;

struct ListItemAccessor
{
    virtual ~ListItemAccessor() {}

    virtual const IndexBasedAccessor* GetIndexer() const = 0;
    virtual ListEnumeratorPtr CreateEnumerator() const = 0;
    virtual nonstd::optional<size_t> GetSize() const = 0;
    
    template<typename T, typename ... Args>
    static ListEnumeratorPtr MakeEnumerator(Args&& ... args)
    {
        return ListEnumeratorPtr(new T(std::forward<Args>(args)...), [](ListEnumerator* e) {delete e;});
    }
};

class GenericListIterator;

class GenericList
{
public:
    GenericList() = default;
    GenericList(std::function<const ListItemAccessor* ()> accessor)
        : m_accessor(std::move(accessor))
    {
    }

    nonstd::optional<size_t> GetSize() const
    {        
        return m_accessor ? m_accessor()->GetSize() : 0ULL;
    }

    auto GetAccessor() const
    {
        return m_accessor();
    }

    bool IsValid() const
    {
        return !(!m_accessor);
    }
    
    GenericListIterator begin() const;
    GenericListIterator end() const;
    auto cbegin() const;
    auto cend() const;

    std::function<const ListItemAccessor* ()> m_accessor;
};

}



#endif // JINJA2_GENERIC_LIST_H