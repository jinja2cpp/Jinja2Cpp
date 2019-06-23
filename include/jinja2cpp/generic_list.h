#ifndef JINJA2_GENERIC_LIST_H
#define JINJA2_GENERIC_LIST_H

#include <nonstd/optional.hpp>

#include <iterator>
#include <memory>
#include <functional>

namespace jinja2
{
class Value;

struct IndexBasedAccessor
{
    virtual Value GetItemByIndex(int64_t idx) const = 0;

    virtual size_t GetItemsCount() const = 0;
};

struct ListEnumerator;
using ListEnumeratorPtr = std::unique_ptr<ListEnumerator, void (*)(ListEnumerator*)>;

inline auto MakeEmptyListEnumeratorPtr()
{
    return ListEnumeratorPtr(nullptr, [](ListEnumerator*) {});
}

struct ListEnumerator
{
    virtual ~ListEnumerator() {}

    virtual void Reset() = 0;

    virtual bool MoveNext() = 0;

    virtual Value GetCurrent() const = 0;

    virtual ListEnumeratorPtr Clone() const = 0;
};

struct ListItemAccessor
{
    virtual ~ListItemAccessor() {}

    virtual const IndexBasedAccessor* GetIndexer() const = 0;

    virtual ListEnumeratorPtr CreateEnumerator() const = 0;

    virtual nonstd::optional<size_t> GetSize() const = 0;

    template<typename T, typename ... Args>
    static ListEnumeratorPtr MakeEnumerator(Args&& ... args);
};

class GenericListIterator;

class GenericList
{
public:
    GenericList() = default;

    GenericList(std::function<const ListItemAccessor*()> accessor)
        : m_accessor(std::move(accessor))
    {
    }

    nonstd::optional<size_t> GetSize() const
    {
        return m_accessor ? m_accessor()->GetSize() : nonstd::optional<size_t>();
    }

    auto GetAccessor() const
    {
        return m_accessor ? m_accessor() : nullptr;
    }

    bool IsValid() const
    {
        return !(!m_accessor);
    }

    GenericListIterator begin() const;

    GenericListIterator end() const;

    auto cbegin() const;

    auto cend() const;

    std::function<const ListItemAccessor*()> m_accessor;
};

template<typename T, typename ...Args>
inline ListEnumeratorPtr ListItemAccessor::MakeEnumerator(Args&& ...args)
{
    return ListEnumeratorPtr(new T(std::forward<Args>(args)...), [](ListEnumerator* e) { delete e; });
}
}


#endif // JINJA2_GENERIC_LIST_H