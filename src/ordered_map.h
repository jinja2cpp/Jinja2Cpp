#ifndef JINJA2CPP_SRC_ORDERED_MAP_H
#define JINJA2CPP_SRC_ORDERED_MAP_H

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <list>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace jinja2
{

// A hash map that iterates in insertion order, like a Python dict (docs/tasks/0031).
//
// The entries live in a std::list and, once there are more than a few of them, an unordered
// index maps each key to its list node; small maps (most dict literals and kwargs) are
// searched linearly and allocate one node per entry. The index refers to the key stored in
// the node, so a lookup allocates nothing, and references and iterators to entries stay
// valid until the entry itself is erased. Assigning
// to an existing key keeps its position; erasing an entry keeps the order of the others.
// Equality ignores the order, as Python dict equality does.
template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEqual = std::equal_to<K>>
class OrderedMap
{
public:
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<const K, V>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using reference = value_type&;
    using const_reference = const value_type&;

private:
    using List = std::list<value_type>;

public:
    using iterator = typename List::iterator;
    using const_iterator = typename List::const_iterator;
    using reverse_iterator = typename List::reverse_iterator;
    using const_reverse_iterator = typename List::const_reverse_iterator;

    OrderedMap() = default;
    OrderedMap(std::initializer_list<value_type> init) { insert(init.begin(), init.end()); }
    template<typename InputIt>
    OrderedMap(InputIt first, InputIt last)
    {
        insert(first, last);
    }
    OrderedMap(const OrderedMap& other)
        : m_items(other.m_items)
    {
        Reindex();
    }
    // swap() keeps iterators valid on every standard library, a move constructor only since LWG 2321
    OrderedMap(OrderedMap&& other) { swap(other); }
    ~OrderedMap() = default;

    OrderedMap& operator=(const OrderedMap& other)
    {
        if (this != &other)
        {
            OrderedMap tmp(other);
            swap(tmp);
        }
        return *this;
    }
    OrderedMap& operator=(OrderedMap&& other)
    {
        if (this != &other)
        {
            OrderedMap tmp(std::move(other));
            swap(tmp);
        }
        return *this;
    }
    OrderedMap& operator=(std::initializer_list<value_type> init)
    {
        OrderedMap tmp(init);
        swap(tmp);
        return *this;
    }

    iterator begin() noexcept { return m_items.begin(); }
    const_iterator begin() const noexcept { return m_items.begin(); }
    const_iterator cbegin() const noexcept { return m_items.cbegin(); }
    iterator end() noexcept { return m_items.end(); }
    const_iterator end() const noexcept { return m_items.end(); }
    const_iterator cend() const noexcept { return m_items.cend(); }
    reverse_iterator rbegin() noexcept { return m_items.rbegin(); }
    const_reverse_iterator rbegin() const noexcept { return m_items.rbegin(); }
    reverse_iterator rend() noexcept { return m_items.rend(); }
    const_reverse_iterator rend() const noexcept { return m_items.rend(); }

    bool empty() const noexcept { return m_items.empty(); }
    size_type size() const noexcept { return m_items.size(); }
    size_type max_size() const noexcept { return m_index.max_size(); }

    void clear() noexcept
    {
        m_index.clear();
        m_items.clear();
    }
    void reserve(size_type count) { m_index.reserve(count); }
    void swap(OrderedMap& other) noexcept
    {
        m_items.swap(other.m_items);
        m_index.swap(other.m_index);
    }

    iterator find(const K& key)
    {
        if (!IsIndexed())
            return FindLinear(m_items, key);
        auto p = m_index.find(KeyRef{ &key });
        return p == m_index.end() ? m_items.end() : p->second;
    }
    const_iterator find(const K& key) const
    {
        if (!IsIndexed())
            return FindLinear(m_items, key);
        auto p = m_index.find(KeyRef{ &key });
        return p == m_index.end() ? m_items.end() : const_iterator(p->second);
    }
    size_type count(const K& key) const { return find(key) == end() ? 0 : 1; }
    bool contains(const K& key) const { return count(key) != 0; }
    std::pair<iterator, iterator> equal_range(const K& key)
    {
        auto p = find(key);
        return { p, p == end() ? p : std::next(p) };
    }
    std::pair<const_iterator, const_iterator> equal_range(const K& key) const
    {
        auto p = find(key);
        return { p, p == end() ? p : std::next(p) };
    }

    V& at(const K& key)
    {
        auto p = find(key);
        if (p == end())
            throw std::out_of_range("OrderedMap::at");
        return p->second;
    }
    const V& at(const K& key) const
    {
        auto p = find(key);
        if (p == end())
            throw std::out_of_range("OrderedMap::at");
        return p->second;
    }
    V& operator[](const K& key) { return try_emplace(key).first->second; }
    V& operator[](K&& key) { return try_emplace(std::move(key)).first->second; }

    template<typename... Args>
    std::pair<iterator, bool> try_emplace(const K& key, Args&&... args)
    {
        auto p = find(key);
        if (p != end())
            return { p, false };
        m_items.emplace_back(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(std::forward<Args>(args)...));
        return { IndexLast(), true };
    }
    template<typename... Args>
    std::pair<iterator, bool> try_emplace(K&& key, Args&&... args)
    {
        auto p = find(key);
        if (p != end())
            return { p, false };
        m_items.emplace_back(std::piecewise_construct, std::forward_as_tuple(std::move(key)), std::forward_as_tuple(std::forward<Args>(args)...));
        return { IndexLast(), true };
    }
    template<typename... Args>
    iterator try_emplace(const_iterator /*hint*/, const K& key, Args&&... args)
    {
        return try_emplace(key, std::forward<Args>(args)...).first;
    }
    template<typename... Args>
    iterator try_emplace(const_iterator /*hint*/, K&& key, Args&&... args)
    {
        return try_emplace(std::move(key), std::forward<Args>(args)...).first;
    }

    // Like std::unordered_map, an existing key keeps its value.
    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args)
    {
        List node;
        node.emplace_back(std::forward<Args>(args)...);
        auto p = find(node.front().first);
        if (p != end())
            return { p, false };
        m_items.splice(m_items.end(), node);
        return { IndexLast(), true };
    }
    template<typename... Args>
    iterator emplace_hint(const_iterator /*hint*/, Args&&... args)
    {
        return emplace(std::forward<Args>(args)...).first;
    }

    std::pair<iterator, bool> insert(const value_type& value) { return try_emplace(value.first, value.second); }
    template<typename P, typename = typename std::enable_if<std::is_constructible<value_type, P&&>::value>::type>
    std::pair<iterator, bool> insert(P&& value)
    {
        return emplace(std::forward<P>(value));
    }
    iterator insert(const_iterator /*hint*/, const value_type& value) { return insert(value).first; }
    template<typename P, typename = typename std::enable_if<std::is_constructible<value_type, P&&>::value>::type>
    iterator insert(const_iterator /*hint*/, P&& value)
    {
        return emplace(std::forward<P>(value)).first;
    }
    template<typename InputIt>
    void insert(InputIt first, InputIt last)
    {
        for (; first != last; ++first)
            emplace(*first);
    }
    void insert(std::initializer_list<value_type> init) { insert(init.begin(), init.end()); }

    template<typename M>
    std::pair<iterator, bool> insert_or_assign(const K& key, M&& obj)
    {
        auto p = find(key);
        if (p != end())
        {
            p->second = std::forward<M>(obj);
            return { p, false };
        }
        return try_emplace(key, std::forward<M>(obj));
    }
    template<typename M>
    std::pair<iterator, bool> insert_or_assign(K&& key, M&& obj)
    {
        auto p = find(key);
        if (p != end())
        {
            p->second = std::forward<M>(obj);
            return { p, false };
        }
        return try_emplace(std::move(key), std::forward<M>(obj));
    }

    iterator erase(const_iterator pos)
    {
        if (IsIndexed())
            m_index.erase(KeyRef{ &pos->first });
        return m_items.erase(pos);
    }
    iterator erase(iterator pos) { return erase(const_iterator(pos)); }
    iterator erase(const_iterator first, const_iterator last)
    {
        while (first != last)
            first = erase(first);
        return m_items.erase(last, last);
    }
    size_type erase(const K& key)
    {
        auto p = find(key);
        if (p == end())
            return 0;
        erase(p);
        return 1;
    }

    friend bool operator==(const OrderedMap& lhs, const OrderedMap& rhs)
    {
        if (lhs.size() != rhs.size())
            return false;
        for (auto& item : lhs)
        {
            auto p = rhs.find(item.first);
            if (p == rhs.end() || !(p->second == item.second))
                return false;
        }
        return true;
    }
    friend bool operator!=(const OrderedMap& lhs, const OrderedMap& rhs) { return !(lhs == rhs); }
    friend void swap(OrderedMap& lhs, OrderedMap& rhs) noexcept { lhs.swap(rhs); }

private:
    struct KeyRef
    {
        const K* key;
    };
    struct KeyRefHash
    {
        std::size_t operator()(const KeyRef& ref) const { return Hash()(*ref.key); }
    };
    struct KeyRefEqual
    {
        bool operator()(const KeyRef& lhs, const KeyRef& rhs) const { return KeyEqual()(*lhs.key, *rhs.key); }
    };

    // Up to this size a linear search beats hashing and saves an index node per entry
    static constexpr size_type IndexThreshold = 8;

    // The index is either empty or holds every entry; an empty index means a linear search
    bool IsIndexed() const noexcept { return !m_index.empty(); }

    template<typename Items>
    static auto FindLinear(Items& items, const K& key) -> decltype(items.begin())
    {
        KeyEqual equal;
        auto p = items.begin();
        for (; p != items.end(); ++p)
        {
            if (equal(p->first, key))
                break;
        }
        return p;
    }

    // Indexes the entry just appended to m_items, dropping it again if indexing throws
    iterator IndexLast()
    {
        auto item = std::prev(m_items.end());
        try
        {
            if (IsIndexed())
                m_index.emplace(KeyRef{ &item->first }, item);
            else if (m_items.size() > IndexThreshold)
                Reindex();
        }
        catch (...)
        {
            // Without the index lookups fall back to the linear search, which is always correct
            m_index.clear();
            m_items.pop_back();
            throw;
        }
        return item;
    }

    void Reindex()
    {
        m_index.clear();
        if (m_items.size() <= IndexThreshold)
            return;
        m_index.reserve(m_items.size());
        for (auto p = m_items.begin(); p != m_items.end(); ++p)
            m_index.emplace(KeyRef{ &p->first }, p);
    }

    List m_items;
    std::unordered_map<KeyRef, iterator, KeyRefHash, KeyRefEqual> m_index;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_ORDERED_MAP_H
