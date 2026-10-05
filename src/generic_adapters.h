#ifndef JINJA2CPP_SRC_GENERIC_ADAPTERS_H
#define JINJA2CPP_SRC_GENERIC_ADAPTERS_H

#include "internal_value.h"

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace jinja2
{

template<typename ImplType, typename List, typename ValType, typename Base>
class IndexedEnumeratorImpl : public Base
{
public:
    using ValueType = ValType;
    // Not ThisType: MSVC's C++17 mode finds a base member before the enclosing class's
    // ThisType in a derived Enumerator, which then names another instantiation
    using EnumeratorImplType = IndexedEnumeratorImpl<ImplType, List, ValType, Base>;

    explicit IndexedEnumeratorImpl(const List* list)
        : m_list(list)
        , m_maxItems(list->GetSize().value_or(0))
    {}

    void Reset() override
    {
        m_curItem = InvalidIndex;
    }

    bool MoveNext() override
    {
        if (m_curItem == InvalidIndex)
        {
            m_curItem = 0;
        }
        else
        {
            ++m_curItem;
        }

        return m_list != nullptr && m_curItem < static_cast<const ImplType*>(this)->CurrentSize();
    }

    // The number of items now; the list the enumerator was made for, by default. Not
    // virtual: MoveNext calls the one ImplType declares
    [[nodiscard]] size_t CurrentSize() const { return m_maxItems; }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const EnumeratorImplType*>(&other);
        if (!val)
        {
            return false;
        }
        if (m_list && val->m_list && !m_list->IsEqual(*val->m_list))
        {
            return false;
        }
        if ((m_list && !val->m_list) || (!m_list && val->m_list))
        {
            return false;
        }
        if (m_curItem != val->m_curItem)
        {
            return false;
        }
        if (m_maxItems != val->m_maxItems)
        {
            return false;
        }
        return true;
    }

protected:
    constexpr static auto InvalidIndex = std::numeric_limits<size_t>::max();
    const List* m_list{};
    size_t m_curItem = InvalidIndex;
    size_t m_maxItems{};
};


template<typename T>
class IndexedListItemAccessorImpl : public IListItemAccessor
    , public IIndexBasedAccessor
{
public:
    using ThisType = IndexedListItemAccessorImpl<T>;
    class Enumerator : public IndexedEnumeratorImpl<Enumerator, ThisType, Value, IListEnumerator>
    {
    public:
        using BaseClass = IndexedEnumeratorImpl<Enumerator, ThisType, Value, IListEnumerator>;
#ifdef _MSC_VER

#if __cplusplus < 202002L
        using IndexedEnumeratorImpl::IndexedEnumeratorImpl;
#else
        using IndexedEnumeratorImpl<Enumerator, ThisType, Value, IListEnumerator>::IndexedEnumeratorImpl;
#endif

#else
        using BaseClass::BaseClass;
#endif

        [[nodiscard]] typename BaseClass::ValueType GetCurrent() const override
        {
            auto indexer = this->m_list->GetIndexer();
            if (!indexer)
            {
                return Value();
            }

            return indexer->GetItemByIndex(this->m_curItem);
        }
        [[nodiscard]] ListEnumeratorPtr Clone() const override
        {
            auto result = MakeEnumerator<Enumerator>(this->m_list);
            auto base = static_cast<Enumerator*>(&(*result));
            base->m_curItem = this->m_curItem;
            return result;
        }

        ListEnumeratorPtr Move() override
        {
            auto result = MakeEnumerator<Enumerator>(this->m_list);
            auto base = static_cast<Enumerator*>(&(*result));
            base->m_curItem = this->m_curItem;
            this->m_list = nullptr;
            this->m_curItem = this->InvalidIndex;
            this->m_maxItems = 0;
            return result;
        }
    };

    [[nodiscard]] Value GetItemByIndex(int64_t idx) const override
    {
        auto item = static_cast<const T*>(this)->GetItem(idx);
        return item ? IntValue2Value(std::move(*item)) : Value();
    }

    [[nodiscard]] std::optional<size_t> GetSize() const override
    {
        return static_cast<const T*>(this)->GetItemsCountImpl();
    }

    [[nodiscard]] const IIndexBasedAccessor* GetIndexer() const override
    {
        return this;
    }

    [[nodiscard]] std::optional<ListEnumeratorPtr> CreateEnumerator() const override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const ThisType*>(&other);
        if (!val)
        {
            return false;
        }
        auto enumerator = CreateEnumerator();
        auto otherEnum = val->CreateEnumerator();
        if (!enumerator || !otherEnum)
        {
            return !enumerator && !otherEnum;
        }
        return (*enumerator)->IsEqual(**otherEnum);
    }
};

template<typename T>
class IndexedListAccessorImpl : public IListAccessor
    , public IndexedListItemAccessorImpl<T>
{
public:
    using ThisType = IndexedListAccessorImpl<T>;
    class Enumerator : public IndexedEnumeratorImpl<Enumerator, ThisType, InternalValue, IListAccessorEnumerator>
    {
    public:
        using BaseClass = IndexedEnumeratorImpl<Enumerator, ThisType, InternalValue, IListAccessorEnumerator>;
#ifdef _MSC_VER

#if __cplusplus < 202002L
        using IndexedEnumeratorImpl::IndexedEnumeratorImpl;
#else
        using IndexedEnumeratorImpl<Enumerator, ThisType, InternalValue, IListAccessorEnumerator>::IndexedEnumeratorImpl;
#endif

#else
        using BaseClass::BaseClass;
#endif

        // The live size: a list the template owns can grow or shrink while it is
        // iterated, and Python's iteration follows it
        [[nodiscard]] size_t CurrentSize() const { return static_cast<const T*>(this->m_list)->GetItemsCountImpl(); }

        [[nodiscard]] typename BaseClass::ValueType GetCurrent() const override
        {
            const auto* list = static_cast<const T*>(this->m_list);
            const auto idx = static_cast<int64_t>(this->m_curItem);
            // An adapter with GetCurrentItem gives the item without the std::optional of GetItem
            if constexpr (HasCurrentItem<T>::value)
            {
                return list->GetCurrentItem(idx);
            }
            else
            {
                auto result = list->GetItem(idx);
                if (!result)
                {
                    return InternalValue();
                }

                return std::move(result.value());
            }
        }

        [[nodiscard]] std::optional<ListAccessorEnumeratorPtr> Clone() const override
        {
            auto result = std::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, this->m_list);
            auto base = *result;
            auto& typedBase = static_cast<Enumerator&>(*base);
            typedBase.m_curItem = this->m_curItem;
            return result;
        }

        std::optional<ListAccessorEnumeratorPtr> Transfer() override
        {
            auto result = std::make_optional<ListAccessorEnumeratorPtr>(types::in_place_type_t<Enumerator>{}, std::move(*this));
            auto base = *result;
            auto& typedBase = static_cast<Enumerator&>(*base);
            typedBase.m_curItem = this->m_curItem;
            this->m_list = nullptr;
            this->m_curItem = this->InvalidIndex;
            this->m_maxItems = 0;
            return result;
        }
    };

    [[nodiscard]] std::optional<size_t> GetSize() const override
    {
        return static_cast<const T*>(this)->GetItemsCountImpl();
    }
    [[nodiscard]] std::optional<ListAccessorEnumeratorPtr> CreateListAccessorEnumerator() const override;

    void ForEach(IListAccessor::ItemVisitor fn) const override
    {
        const auto* list = static_cast<const T*>(this);
        // The size is read at each step: a list the template owns can change while fn runs
        for (size_t idx = 0; idx < list->GetItemsCountImpl(); ++idx)
        {
            if constexpr (HasCurrentItem<T>::value)
            {
                if (!fn(list->GetCurrentItem(static_cast<int64_t>(idx))))
                {
                    return;
                }
            }
            else
            {
                auto item = list->GetItem(static_cast<int64_t>(idx));
                if (!fn(item ? std::move(*item) : InternalValue()))
                {
                    return;
                }
            }
        }
    }

private:
    // Whether T has InternalValue GetCurrentItem(int64_t): the item the enumerator stands
    // on, Undefined past the end
    template<typename U, typename = void>
    struct HasCurrentItem : std::false_type
    {
    };
    template<typename U>
    struct HasCurrentItem<U, std::void_t<decltype(std::declval<const U&>().GetCurrentItem(int64_t{}))>> : std::true_type
    {
    };
};

template<typename T>
class MapItemAccessorImpl : public IMapItemAccessor
{
public:
    [[nodiscard]] Value GetValueByName(const std::string& name) const override
    {
        return IntValue2Value(static_cast<const T*>(this)->GetItem(name));
    }
};

template<typename T>
class MapAccessorImpl : public IMapAccessor
    , public MapItemAccessorImpl<T>
{
public:
};

template<typename T>
inline std::optional<ListAccessorEnumeratorPtr> IndexedListAccessorImpl<T>::CreateListAccessorEnumerator() const
{
    return ListAccessorEnumeratorPtr(types::in_place_type_t<Enumerator>{}, Enumerator(this));
}

template<typename T>
inline std::optional<ListEnumeratorPtr> IndexedListItemAccessorImpl<T>::CreateEnumerator() const
{
    return MakeEnumerator<Enumerator>(this);
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_GENERIC_ADAPTERS_H
