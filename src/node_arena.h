#ifndef JINJA2CPP_SRC_NODE_ARENA_H
#define JINJA2CPP_SRC_NODE_ARENA_H

#include <boost/container/small_vector.hpp>
#include <boost/core/span.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

// The handles a parse tree is linked with (docs/design/0118-parse-tree-arena-plan.md).
//
// Every node of a template's tree belongs to the template's arena, and nodes refer to each
// other by NodeRef and ArenaSpan: 32-bit offsets, never pointers. A handle is resolved only
// through the arena that made it: NodeArena while the template is parsed, ArenaView
// (RenderContext::Nodes) during a render.
//
// The parse bump-allocates nodes and lists from a few blocks, and a handle is the number
// of bytes allocated before its object. Seal then moves everything into one buffer of
// exactly that size, where each object lands at its handle's offset: no handle changes,
// and the template keeps no slack (phase P4).

#ifndef JINJA2CPP_NODEREF_CHECKS_LEVEL
#error "JINJA2CPP_NODEREF_CHECKS_LEVEL comes with the jinja2cpp target (CMake option JINJA2CPP_NODEREF_CHECKS)"
#endif

namespace jinja2
{
// How much a handle is checked before it is followed (CMake JINJA2CPP_NODEREF_CHECKS):
// 0 (OFF): never. 1 (ON, the Release default): Seal checks every handle of the tree, and
// a render checks the ones that cross into another template. 2 (FULL, Debug, sanitizer and
// fuzz builds): every access too
constexpr int NodeRefChecks = JINJA2CPP_NODEREF_CHECKS_LEVEL;
static_assert(NodeRefChecks >= 0 && NodeRefChecks <= 2, "JINJA2CPP_NODEREF_CHECKS_LEVEL is 0, 1 or 2");

// A handle that does not point at a node of its type: a bug, never a template's fault
class InvalidNodeRef : public std::logic_error
{
public:
    InvalidNodeRef()
        : std::logic_error("invalid node reference")
    {
    }
};

// What a node is, stored in its header: checks a downcast without RTTI
enum class NodeKind : std::uint8_t
{
    None, // not made by a NodeArena
    // Expressions
    FullExpr,
    NameRef,
    SelfRef,
    SubscriptExpr,
    LoopAttrExpr,
    FilteredExpr,
    ConstantExpr,
    TupleExpr,
    DictExpr,
    UnaryExpr,
    IsExpr,
    BinaryExpr,
    CompareExpr,
    SliceExpr,
    CallExpr,
    FilterChain,
    IfExpr,
    // Renderers
    ComposedBody,
    RawText,
    ExprRenderer,
    FinalizedExprRenderer,
    TemplateRoot,
    ForStmt,
    IfStmt,
    ElseBranchStmt,
    SetLineStmt,
    SetRawBlockStmt,
    SetFilteredBlockStmt,
    BlockStmt,
    ExtendsStmt,
    IncludeStmt,
    ImportStmt,
    MacroStmt,
    MacroCallStmt,
    DoStmt,
    TransStmt,
    LoopControlStmt,
    WithStmt,
    FilterStmt,
    AutoescapeStmt,
    // Objects the arena moves through their own vptr (ArenaObjectBase)
    FilterObject,
    TesterObject,
};
// The number of kinds, None included
constexpr std::uint8_t NodeKindCount = static_cast<std::uint8_t>(NodeKind::TesterObject) + 1;

// The kinds of a class family: a root class that has subclasses of many kinds says which
// with a static Family, and names itself as its FamilyOwner, so that a class between it
// and its subclasses does not claim the whole family
struct KindRange
{
    NodeKind first;
    NodeKind last;

    [[nodiscard]] constexpr bool Contains(NodeKind kind) const { return kind >= first && kind <= last; }
};

// The header every arena node starts with, after its vptr
class ArenaNode
{
public:
    [[nodiscard]] NodeKind GetKind() const { return m_kind; }

private:
    friend class NodeArena;
    NodeKind m_kind = NodeKind::None;
};

// The base of a class family the arena does not list class by class, filters and tests: an
// object of it moves itself when the arena seals. One made on the heap never moves
class ArenaObjectBase : public ArenaNode
{
public:
    virtual ~ArenaObjectBase() = default;

    // Moves the object to where the arena keeps its header at `to`
    virtual void RelocateTo(std::byte* /*to*/) { std::terminate(); }

protected:
    ArenaObjectBase() = default;
    ArenaObjectBase(const ArenaObjectBase&) = default;
    ArenaObjectBase(ArenaObjectBase&&) = default;
    ArenaObjectBase& operator=(const ArenaObjectBase&) = default;
    ArenaObjectBase& operator=(ArenaObjectBase&&) = default;
};

class ArenaView;
class NodeArena;
class SealedArena;

namespace detail
{
template<typename Derived>
class ArenaAccess;
// Makes handles from raw offsets and reaches a sealed tree's bytes: defined by the tests only
struct ArenaTestAccess;
class RefChecker;
} // namespace detail

// A link to a node of T, or a null link: the node's offset in its arena
template<typename T>
class NodeRef
{
public:
    NodeRef() = default;
    // To a base of T
    template<typename U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
    NodeRef(NodeRef<U> other) // NOLINT(google-explicit-constructor)
        : m_offset(other.m_offset)
    {
        // The offset of a node is also the offset of its base only when both start the
        // same way: nodes derive singly, so a base starts at the node unless one of them
        // has a vptr and the other has not
        static_assert(std::is_polymorphic_v<T> == std::is_polymorphic_v<U>, "a base that starts inside the node");
    }

    explicit operator bool() const { return m_offset != 0; }

private:
    friend class ArenaView;
    friend class NodeArena;
    friend struct detail::ArenaTestAccess;
    friend class detail::RefChecker;
    template<typename U>
    friend class NodeRef;
    template<typename Derived>
    friend class detail::ArenaAccess;

    explicit NodeRef(std::uint32_t offset)
        : m_offset(offset)
    {
    }

    std::uint32_t m_offset = 0;
};

// A list the arena holds: a statement body, the items of a literal, the branches of an `if`
template<typename T>
class ArenaSpan
{
public:
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>, "an arena list holds handles");

    ArenaSpan() = default;

    [[nodiscard]] bool empty() const { return m_size == 0; }
    [[nodiscard]] std::size_t size() const { return m_size; }

private:
    friend class ArenaView;
    friend class NodeArena;
    friend struct detail::ArenaTestAccess;
    friend class detail::RefChecker;

    ArenaSpan(std::uint32_t offset, std::uint32_t size)
        : m_offset(offset)
        , m_size(size)
    {
    }

    std::uint32_t m_offset = 0;
    std::uint32_t m_size = 0;
};

namespace detail
{
template<typename T, typename = void>
struct HasKindMatch : std::false_type
{
};
template<typename T>
struct HasKindMatch<T, std::void_t<decltype(T::MatchesKind(NodeKind::None))>> : std::true_type
{
};
template<typename T, typename = void>
struct HasKind : HasKindMatch<T>
{
};
template<typename T>
struct HasKind<T, std::void_t<decltype(T::Kind)>> : std::true_type
{
};
template<typename T, typename = void>
struct OwnsFamily : std::false_type
{
};
template<typename T>
struct OwnsFamily<T, std::void_t<decltype(T::Family), typename T::FamilyOwner>> : std::is_same<typename T::FamilyOwner, T>
{
};
// Whether a node's kind tells if it is a T
template<typename T>
constexpr bool HasKindCheck = HasKind<T>::value || OwnsFamily<T>::value;

// True if a node of this kind is a T. A class with subclasses in the tree says which kinds
// it covers with a static MatchesKind(NodeKind); its subclasses inherit that, so each of
// them declares its own
template<typename T>
[[nodiscard]] constexpr bool KindIs(NodeKind kind)
{
    static_assert(HasKindCheck<T>, "a class whose kinds the arena does not know");
    if constexpr (HasKindMatch<T>::value)
    {
        return T::MatchesKind(kind);
    }
    else if constexpr (HasKind<T>::value)
    {
        return kind == T::Kind;
    }
    else
    {
        return T::Family.Contains(kind);
    }
}

// Kind checks and downcasts, for the arena while it is built and for a view of it
template<typename Derived>
class ArenaAccess
{
public:
    // True if the node is a T. A class with subclasses in the tree says which kinds it
    // covers with a static MatchesKind(NodeKind); its subclasses inherit that, so each
    // of them declares its own
    template<typename T, typename U>
    [[nodiscard]] bool Is(NodeRef<U> ref) const
    {
        if (!ref)
        {
            return false;
        }
        return KindIs<T>(Self()[ref].GetKind());
    }

    // True if a node of this kind is a T
    template<typename T>
    [[nodiscard]] static bool KindIs(NodeKind kind)
    {
        return detail::KindIs<T>(kind);
    }

    // The node as a T, or a null link if it is not one
    template<typename T, typename U>
    [[nodiscard]] NodeRef<T> As(NodeRef<U> ref) const
    {
        static_assert(std::is_base_of_v<U, T>, "a downcast");
        static_assert(std::is_polymorphic_v<T> == std::is_polymorphic_v<U>, "a node that starts inside its base");
        return Is<T>(ref) ? NodeRef<T>(ref.m_offset) : NodeRef<T>();
    }

    // The node, which is a T
    template<typename T, typename U>
    [[nodiscard]] T& Get(NodeRef<U> ref) const
    {
        const auto node = As<T>(ref);
        assert(node);
        return Self()[node];
    }

private:
    [[nodiscard]] const Derived& Self() const { return static_cast<const Derived&>(*this); }
};

class RefChecker;

// How the arena moves and destroys a node of one kind
struct NodeOps
{
    // Moves the node to where the arena keeps its header at `to`, leaving the original to
    // be destroyed; then checks the handles of the moved node, unless refs is null
    void (*relocate)(ArenaNode& from, std::byte* to, RefChecker* refs);
    // Lets a moved node fix what it keeps outside itself: the lists it points into
    void (*relocated)(ArenaNode& node, const ArenaView& nodes);
    void (*destroy)(ArenaNode& node) noexcept;
};

template<typename T, typename = void>
struct HasRelocated : std::false_type
{
};
template<typename T>
struct HasRelocated<T, std::void_t<decltype(std::declval<T&>().OnRelocated(std::declval<const ArenaView&>()))>> : std::true_type
{
};

// Where the ArenaNode header of a T lies in it, known without an object: after the vptr
// of a polymorphic class, whose bases list the node's own root first. Checked against
// HeaderOffset on every Make in FULL
template<typename T>
constexpr std::uint32_t HeaderOffsetOf = std::is_polymorphic_v<T> ? sizeof(void*) : 0;

// Where the ArenaNode header of a T lies in it
template<typename T>
std::ptrdiff_t HeaderOffset(T& node)
{
    return reinterpret_cast<std::byte*>(&static_cast<ArenaNode&>(node)) - reinterpret_cast<std::byte*>(&node);
}

// Bitmaps over a tree's bytes, a bit per granule: where the nodes' headers start, and in
// FULL which nodes and lists Seal reached. A header lies at a node's offset or one pointer
// past it, both multiples of the granule
constexpr std::uint32_t BitGranule = sizeof(void*) >= 8 ? 8 : 4;

inline std::size_t BitWords(std::uint32_t bytes)
{
    return ((std::size_t{ bytes } / BitGranule) + 63) / 64;
}

inline bool TestBit(const std::uint64_t* bits, std::uint32_t offset)
{
    const std::size_t idx = offset / BitGranule;
    return ((bits[idx / 64] >> (idx % 64)) & 1U) != 0;
}

inline void SetBit(std::uint64_t* bits, std::uint32_t offset)
{
    const std::size_t idx = offset / BitGranule;
    bits[idx / 64] |= std::uint64_t{ 1 } << (idx % 64);
}

template<typename T>
struct IsNodeRef : std::false_type
{
};
template<typename T>
struct IsNodeRef<NodeRef<T>> : std::true_type
{
};

// Checks the handles of a tree as Seal moves its nodes: each one points inside the tree, at
// the start of a node of its type, and each list lies inside it. Each node class lists its
// handles with `void VisitRefs(RefChecker&) const`, the handles in the heap containers it
// owns included. Never throws: Seal runs it where it cannot unwind, and throws after
class RefChecker final
{
public:
    // starts: the bit of every node's header; validated, in FULL: where to mark what the
    // checks reach
    RefChecker(const std::byte* base, std::uint32_t size, const std::uint64_t* starts, std::uint64_t* validated) noexcept
        : m_base(base)
        , m_size(size)
        , m_nodesSize(size - std::uint32_t{ sizeof(std::uint64_t) })
        , m_starts(starts)
        , m_validated(validated)
    {
    }

    template<typename T>
    void operator()(NodeRef<T> ref) noexcept
    {
        const std::uint32_t offset = ref.m_offset;
        if (offset == 0)
        {
            return;
        }
        // A header is at the node's offset or one pointer past it, both in the granule
        constexpr std::uint32_t alignMask = static_cast<std::uint32_t>(std::max<std::size_t>(alignof(T), BitGranule) - 1);
        const std::uint32_t header = offset + HeaderOffsetOf<T>;
        // In range first, so that the bit and the kind byte are inside the tree. An offset
        // inside the arena's header wraps below it and fails the same compare
        if ((offset & alignMask) != 0 || std::uint64_t{ offset - sizeof(std::uint64_t) } + sizeof(T) > m_nodesSize || !TestBit(m_starts, header))
        {
            m_ok = false;
            return;
        }
        if constexpr (HasKindCheck<T>)
        {
            // The kind byte starts the header; Seal copied every node's bytes first
            if (!KindIs<T>(static_cast<NodeKind>(m_base[header])))
            {
                m_ok = false;
            }
        }
        if constexpr (NodeRefChecks >= 2)
        {
            SetBit(m_validated, header);
        }
    }

    // A list: of handles, each checked too; of other items, only where the list lies
    template<typename T>
    void operator()(ArenaSpan<T> list) noexcept
    {
        if (!CheckSpan(list))
        {
            return;
        }
        if constexpr (IsNodeRef<T>::value)
        {
            All(Items(list));
        }
    }

    // A list of structs: visitItem(checker, item) checks the handles of each
    template<typename T, typename F>
    void operator()(ArenaSpan<T> list, const F& visitItem) noexcept
    {
        if (CheckSpan(list))
        {
            All(Items(list), visitItem);
        }
    }

    // Handles, or pairs with a handle second, in a heap container a node owns
    template<typename Range>
    void All(const Range& items) noexcept
    {
        for (const auto& item : items)
        {
            if constexpr (IsNodeRef<std::decay_t<decltype(item)>>::value)
            {
                (*this)(item);
            }
            else
            {
                (*this)(item.second);
            }
        }
    }

    template<typename Range, typename F>
    void All(const Range& items, const F& visitItem) noexcept
    {
        for (const auto& item : items)
        {
            visitItem(*this, item);
        }
    }

    [[nodiscard]] bool Ok() const noexcept { return m_ok; }

private:
    [[nodiscard]] bool InRange(std::uint32_t offset, std::size_t bytes) const noexcept
    {
        return offset >= sizeof(std::uint64_t) && offset <= m_size && bytes <= m_size - offset;
    }

    template<typename T>
    bool CheckSpan(ArenaSpan<T> list) noexcept
    {
        if (list.empty())
        {
            if (list.m_offset != 0)
            {
                m_ok = false;
            }
            return false;
        }
        if (!InRange(list.m_offset, std::size_t{ list.m_size } * sizeof(T)) || list.m_offset % BitGranule != 0)
        {
            m_ok = false;
            return false;
        }
        if constexpr (NodeRefChecks >= 2)
        {
            SetBit(m_validated, list.m_offset);
        }
        return true;
    }

    template<typename T>
    boost::span<const T> Items(ArenaSpan<T> list) const noexcept
    {
        return { std::launder(reinterpret_cast<const T*>(m_base + list.m_offset)), list.m_size };
    }

    const std::byte* m_base;
    std::uint32_t m_size;
    // The bytes after the arena's header
    std::uint32_t m_nodesSize;
    const std::uint64_t* m_starts;
    std::uint64_t* m_validated;
    bool m_ok = true;
};

template<typename T>
void Relocate(ArenaNode& from, std::byte* to, RefChecker* refs)
{
    auto& node = static_cast<T&>(from);
    if constexpr (std::is_base_of_v<ArenaObjectBase, T>)
    {
        // An interface: the object knows its class
        node.RelocateTo(to);
    }
    else
    {
        new (to - HeaderOffset(node)) T(std::move(node));
    }
    // Seal passes the checker unless the checks are OFF
    if constexpr (NodeRefChecks >= 1)
    {
        static_cast<const T&>(*std::launder(reinterpret_cast<ArenaNode*>(to))).VisitRefs(*refs);
    }
}

// An F the arena made: moves itself as an F
template<typename F>
class ArenaObject final : public F
{
public:
    using F::F;

    void RelocateTo(std::byte* to) override { new (to - HeaderOffset(*this)) ArenaObject(std::move(*this)); }
};

template<typename T>
void Relocated(ArenaNode& node, const ArenaView& nodes)
{
    static_cast<T&>(node).OnRelocated(nodes);
}

template<typename T>
constexpr auto RelocatedFor() -> decltype(NodeOps::relocated)
{
    if constexpr (HasRelocated<T>::value)
    {
        return &Relocated<T>;
    }
    else
    {
        return nullptr;
    }
}

template<typename T>
void Destroy(ArenaNode& node) noexcept
{
    static_cast<T&>(node).~T();
}

// The operations of a node kind: one class per kind (node_arena.cpp)
const NodeOps& OpsOf(NodeKind kind);

// The offsets of the nodes an arena made, in the order it made them. Small templates fit
// in the inline part; the list is not movable, as the arena that holds it
class OffsetList
{
public:
    OffsetList() = default;
    OffsetList(const OffsetList&) = delete;
    OffsetList(OffsetList&&) = delete;
    OffsetList& operator=(const OffsetList&) = delete;
    OffsetList& operator=(OffsetList&&) = delete;
    ~OffsetList() = default;

    // Makes room for one more offset, so that Push cannot fail
    void Reserve()
    {
        if (m_size == m_capacity)
        {
            Grow();
        }
    }
    void Push(std::uint32_t offset) noexcept
    {
        assert(m_size < m_capacity);
        m_data[m_size++] = offset;
    }
    void Clear() noexcept { m_size = 0; }

    [[nodiscard]] std::size_t size() const noexcept { return m_size; }
    [[nodiscard]] const std::uint32_t* data() const noexcept { return m_data; }
    std::uint32_t operator[](std::size_t idx) const noexcept { return m_data[idx]; }

private:
    void Grow();

    static constexpr std::size_t InlineCapacity = 32;
    std::array<std::uint32_t, InlineCapacity> m_inline{};
    std::unique_ptr<std::uint32_t[]> m_heap;
    std::uint32_t* m_data = m_inline.data();
    std::size_t m_size = 0;
    std::size_t m_capacity = InlineCapacity;
};

// The start of an arena's buffer, before its first object: an offset of 0 is a null link
struct ArenaHeader
{
    // Bytes of objects and lists, the header included
    std::uint32_t size;
    // Objects to destroy, whose header offsets follow the objects
    std::uint32_t objects;
};
static_assert(sizeof(ArenaHeader) == sizeof(std::uint64_t), "RefChecker starts the tree after the header");

// Where a sealed tree keeps the FULL bitmap of the nodes and lists Seal reached: after the
// cleanup table, aligned for its words
inline std::size_t ValidatedBitsAt(std::uint32_t size, std::uint32_t objects)
{
    return (std::size_t{ size } + (std::size_t{ objects } * sizeof(std::uint32_t)) + 7) & ~std::size_t{ 7 };
}

// A handle Seal checks although no node holds it: the template's root, or what a test reads
struct RootRef
{
    void (*check)(RefChecker& refs, std::uint32_t offset, std::uint32_t size);
    std::uint32_t offset;
    std::uint32_t size;
};
} // namespace detail

// Resolves the handles of one template's sealed tree. A render holds the view of the
// template whose code runs (RenderContext::Nodes) and switches it where it crosses into
// another template's tree
class ArenaView : public detail::ArenaAccess<ArenaView>
{
public:
    ArenaView() = default;

    template<typename T>
    T& operator[](NodeRef<T> ref) const
    {
        if constexpr (NodeRefChecks >= 2)
        {
            CheckRef(ref);
        }
        assert(ref && InRange(ref.m_offset, sizeof(T)));
        return *std::launder(reinterpret_cast<T*>(m_base + ref.m_offset));
    }

    template<typename T>
    boost::span<const T> operator[](ArenaSpan<T> list) const
    {
        if constexpr (NodeRefChecks >= 2)
        {
            CheckSpan(list);
        }
        // An empty list has offset 0, which is still inside the buffer: no branch
        assert(list.empty() ? list.m_offset == 0 : InRange(list.m_offset, list.m_size * sizeof(T)));
        return boost::span<const T>(std::launder(reinterpret_cast<const T*>(m_base + list.m_offset)), list.m_size);
    }

    // The node, after checking that the handle points at the start of a node of its type:
    // for a handle that crosses into this tree from another template's, at every level
    // but OFF. Throws InvalidNodeRef
    template<typename T>
    T& Checked(NodeRef<T> ref) const
    {
        CheckRef(ref);
        if (!IsObjectStart(ref.m_offset + detail::HeaderOffsetOf<T>))
        {
            throw InvalidNodeRef();
        }
        return *std::launder(reinterpret_cast<T*>(m_base + ref.m_offset));
    }

    // A list to rewrite while the arena is sealed, from a node's OnRelocated
    template<typename T>
    boost::span<T> Rewrite(ArenaSpan<T> list) const
    {
        if (list.empty())
        {
            return {};
        }
        assert(InRange(list.m_offset, list.m_size * sizeof(T)));
        return boost::span<T>(std::launder(reinterpret_cast<T*>(m_base + list.m_offset)), list.m_size);
    }

    // The same for every view of one tree, and different from any other tree's while it lives
    [[nodiscard]] const void* Id() const { return m_base; }

    // Views of the same tree
    friend bool operator==(const ArenaView& lhs, const ArenaView& rhs) { return lhs.m_base == rhs.m_base; }
    friend bool operator!=(const ArenaView& lhs, const ArenaView& rhs) { return !(lhs == rhs); }

private:
    friend class NodeArena;
    friend struct detail::ArenaTestAccess;
    friend class SealedArena;

    ArenaView(std::byte* base, std::uint32_t size)
        : m_base(base)
        , m_size(size)
    {
    }

    [[nodiscard]] bool InRange(std::uint32_t offset, std::size_t bytes) const
    {
        return offset >= sizeof(detail::ArenaHeader) && offset <= m_size && bytes <= m_size - offset;
    }

    // Range, alignment and kind
    template<typename T>
    void CheckRef(NodeRef<T> ref) const
    {
        if (!ref || !InRange(ref.m_offset, sizeof(T)) || ref.m_offset % alignof(T) != 0)
        {
            throw InvalidNodeRef();
        }
        if constexpr (detail::HasKindCheck<T>)
        {
            // The kind byte starts the header
            const auto kind = static_cast<NodeKind>(m_base[ref.m_offset + detail::HeaderOffsetOf<T>]);
            if (!KindIs<T>(kind))
            {
                throw InvalidNodeRef();
            }
        }
        // A node Seal did not reach: a handle VisitRefs does not list, or a stray one
        if constexpr (NodeRefChecks >= 2)
        {
            if (!IsValidated(ref.m_offset + detail::HeaderOffsetOf<T>))
            {
                throw InvalidNodeRef();
            }
        }
    }

    template<typename T>
    void CheckSpan(ArenaSpan<T> list) const
    {
        if (list.empty())
        {
            if (list.m_offset != 0)
            {
                throw InvalidNodeRef();
            }
            return;
        }
        if (!InRange(list.m_offset, std::size_t{ list.m_size } * sizeof(T)) || list.m_offset % alignof(T) != 0 || !IsValidated(list.m_offset))
        {
            throw InvalidNodeRef();
        }
    }

    // FULL: whether Seal reached the node whose header, or the list that, starts at `offset`
    [[nodiscard]] bool IsValidated(std::uint32_t offset) const
    {
        if (offset % detail::BitGranule != 0)
        {
            return false;
        }
        detail::ArenaHeader header{};
        std::memcpy(&header, m_base, sizeof(header));
        const std::size_t idx = offset / detail::BitGranule;
        std::uint64_t word = 0;
        std::memcpy(&word, m_base + detail::ValidatedBitsAt(header.size, header.objects) + (idx / 64 * sizeof(word)), sizeof(word));
        return ((word >> (idx % 64)) & 1U) != 0;
    }

    // Whether a node's header starts at `offset`: a search of the sorted table of nodes
    [[nodiscard]] bool IsObjectStart(std::uint32_t offset) const;

    std::byte* m_base = nullptr;
    std::uint32_t m_size = 0;
};

// A template's tree after the parse: one buffer, immutable, so concurrent renders read it
// without locks. Destroys its nodes
class SealedArena
{
public:
    SealedArena() = default;
    SealedArena(const SealedArena&) = delete;
    SealedArena(SealedArena&& other) noexcept
        : m_buffer(std::move(other.m_buffer))
        , m_view(std::exchange(other.m_view, ArenaView()))
    {
    }
    SealedArena& operator=(const SealedArena&) = delete;
    SealedArena& operator=(SealedArena&& other) noexcept
    {
        if (this != &other)
        {
            DestroyNodes();
            m_buffer = std::move(other.m_buffer);
            m_view = std::exchange(other.m_view, ArenaView());
        }
        return *this;
    }
    ~SealedArena() { DestroyNodes(); }

    // Read on every render, so kept rather than read from the buffer's header
    [[nodiscard]] const ArenaView& View() const { return m_view; }
    template<typename T>
    T& operator[](NodeRef<T> ref) const
    {
        return View()[ref];
    }
    template<typename T>
    boost::span<const T> operator[](ArenaSpan<T> list) const
    {
        return View()[list];
    }

private:
    friend class NodeArena;
    friend struct detail::ArenaTestAccess;

    explicit SealedArena(std::unique_ptr<std::byte[]> buffer)
        : m_buffer(std::move(buffer))
        , m_view(m_buffer.get(), Header().size)
    {
    }

    [[nodiscard]] detail::ArenaHeader Header() const
    {
        detail::ArenaHeader header{};
        std::memcpy(&header, m_buffer.get(), sizeof(header));
        return header;
    }

    void DestroyNodes() noexcept;

    std::unique_ptr<std::byte[]> m_buffer;
    ArenaView m_view;
};

// Makes the nodes of one template while it is parsed. Nodes are made during the parse
// only; Seal moves them into the template's SealedArena
class NodeArena : public detail::ArenaAccess<NodeArena>
{
public:
    NodeArena()
    {
        m_blocks.push_back({ m_inline.data(), m_used, nullptr });
    }
    // Handles point into the inline block
    NodeArena(const NodeArena&) = delete;
    NodeArena(NodeArena&&) = delete;
    NodeArena& operator=(const NodeArena&) = delete;
    NodeArena& operator=(NodeArena&&) = delete;
    ~NodeArena() { DestroyNodes(); }

    template<typename T, typename... Args>
    NodeRef<T> Make(Args&&... args)
    {
        return MakeSized<T>(sizeof(T), std::forward<Args>(args)...);
    }

    // An F, seen through its interface I: a class of a family the arena moves through the
    // object's vptr (ArenaObjectBase)
    template<typename I, typename F, typename... Args>
    NodeRef<I> MakeObject(Args&&... args)
    {
        static_assert(std::is_base_of_v<ArenaObjectBase, I> && std::is_base_of_v<I, F>, "an object of a family the arena moves");
        using Object = detail::ArenaObject<F>;
        const auto ref = MakeSized<Object>(sizeof(Object), std::forward<Args>(args)...);
        // The interface may not start the object, wherever the compiler puts the bases
        auto& object = *std::launder(reinterpret_cast<Object*>(Locate(ref.m_offset)));
        auto& face = static_cast<I&>(object);
        if constexpr (NodeRefChecks >= 2)
        {
            if (detail::HeaderOffset(face) != std::ptrdiff_t{ detail::HeaderOffsetOf<I> })
            {
                throw std::logic_error("an arena interface whose header is not after its vptr");
            }
        }
        const auto delta = reinterpret_cast<std::byte*>(&face) - reinterpret_cast<std::byte*>(&object);
        return NodeRef<I>(ref.m_offset + static_cast<std::uint32_t>(delta));
    }

    // A node followed by a copy of items in the same allocation, wherever the node goes: T
    // is constructed from the item count and reads the items right after itself
    template<typename T, typename Item>
    NodeRef<T> MakeWithItems(boost::span<const Item> items)
    {
        static_assert(std::is_final_v<T>, "the items follow the most derived object");
        static_assert(std::is_trivially_copyable_v<Item> && std::is_trivially_destructible_v<Item>, "Seal copies the items with the node's bytes");
        static_assert(sizeof(T) % alignof(Item) == 0, "the items are aligned right after the node");
        if (items.size() > (std::numeric_limits<std::uint32_t>::max() - sizeof(T)) / sizeof(Item))
        {
            throw std::length_error("a template list too long");
        }
        const auto ref = MakeSized<T>(sizeof(T) + items.size_bytes(), static_cast<std::uint32_t>(items.size()));
        // Right after the node: one past it, in the same allocation
        T* const node = std::launder(reinterpret_cast<T*>(Locate(ref.m_offset)));
        std::uninitialized_copy(items.begin(), items.end(), reinterpret_cast<Item*>(node + 1));
        return ref;
    }

    // A copy of items the arena keeps
    template<typename T>
    ArenaSpan<T> MakeSpan(boost::span<const T> items)
    {
        assert(!m_sealed);
        if (items.empty())
        {
            return {};
        }
        static_assert(alignof(T) <= Alignment, "the arena aligns lists to 8 bytes");
        if (items.size() > std::numeric_limits<std::uint32_t>::max() / sizeof(T))
        {
            throw std::length_error("a template list too long");
        }
        std::uint32_t offset = 0;
        T* data = static_cast<T*>(Allocate(items.size_bytes(), offset));
        std::uninitialized_copy(items.begin(), items.end(), data);
        return ArenaSpan<T>(offset, static_cast<std::uint32_t>(items.size()));
    }
    template<typename Container>
    auto MakeSpan(const Container& items)
    {
        using T = typename Container::value_type;
        return MakeSpan(boost::span<const T>(items.data(), items.size()));
    }

    template<typename T>
    T& operator[](NodeRef<T> ref) const
    {
        if constexpr (NodeRefChecks >= 2)
        {
            if (!ref || ref.m_offset < sizeof(detail::ArenaHeader) || sizeof(T) > m_used - ref.m_offset)
            {
                throw InvalidNodeRef();
            }
            if constexpr (detail::HasKindCheck<T>)
            {
                if (!KindIs<T>(static_cast<NodeKind>(*(Locate(ref.m_offset) + detail::HeaderOffsetOf<T>))))
                {
                    throw InvalidNodeRef();
                }
            }
        }
        assert(ref);
        return *std::launder(reinterpret_cast<T*>(Locate(ref.m_offset)));
    }
    template<typename T>
    boost::span<const T> operator[](ArenaSpan<T> list) const
    {
        if (list.empty())
        {
            return {};
        }
        return boost::span<const T>(std::launder(reinterpret_cast<const T*>(Locate(list.m_offset))), list.m_size);
    }

    // The parse is over: moves every node and list into one buffer of the size they take,
    // at the offsets their handles hold. Nothing is made after this. Unless NodeRefChecks
    // is OFF, checks every handle the nodes hold and the roots, the handles held outside
    // the tree, and throws InvalidNodeRef if one does not point at a node of its type
    template<typename... Roots>
    SealedArena Seal(Roots... roots)
    {
        const std::array<detail::RootRef, sizeof...(Roots)> list{ RootOf(roots)... };
        return SealWith(boost::span<const detail::RootRef>(list.data(), list.size()));
    }

private:
    struct Block
    {
        std::byte* data = nullptr;
        // The offset of the block's first byte
        std::uint32_t start = 0;
        std::unique_ptr<std::byte[]> owned;
    };

    template<typename T>
    static detail::RootRef RootOf(NodeRef<T> ref)
    {
        return { [](detail::RefChecker& refs, std::uint32_t offset, std::uint32_t /*size*/) { refs(NodeRef<T>(offset)); }, ref.m_offset, 0 };
    }
    template<typename T>
    static detail::RootRef RootOf(ArenaSpan<T> list)
    {
        return { [](detail::RefChecker& refs, std::uint32_t offset, std::uint32_t size) { refs(ArenaSpan<T>(offset, size)); }, list.m_offset, list.m_size };
    }

    SealedArena SealWith(boost::span<const detail::RootRef> roots);
    // Seal's first pass: moves the nodes over their copies in `base`, checking their handles
    // when refs is set
    void MoveNodes(std::byte* base, detail::RefChecker* refs);

    // A node in `size` bytes, sizeof(T) or more
    template<typename T, typename... Args>
    NodeRef<T> MakeSized(std::size_t size, Args&&... args)
    {
        static_assert(std::is_base_of_v<ArenaNode, T>, "an arena node starts with the ArenaNode header");
        static_assert(alignof(T) <= Alignment, "the arena aligns nodes to 8 bytes");
        assert(!m_sealed);
        if constexpr (!std::is_base_of_v<ArenaObjectBase, T>)
        {
            assert(detail::OpsOf(T::Kind).destroy == &detail::Destroy<T>);
        }
        // Room for the record first, so that it cannot fail once the node exists; a node
        // that throws from its constructor leaves only unused bytes behind
        m_objects.Reserve();
        std::uint32_t offset = 0;
        T* node = new (Allocate(size, offset)) T(std::forward<Args>(args)...);
        if constexpr (NodeRefChecks >= 2 && !std::is_base_of_v<ArenaObjectBase, T>)
        {
            if (detail::HeaderOffset(*node) != std::ptrdiff_t{ detail::HeaderOffsetOf<T> })
            {
                node->~T();
                throw std::logic_error("an arena node whose header is not where its handles look for it");
            }
        }
        auto& header = static_cast<ArenaNode&>(*node);
        header.m_kind = T::Kind;
        m_objects.Push(offset + static_cast<std::uint32_t>(detail::HeaderOffset(*node)));
        return NodeRef<T>(offset);
    }

    // Raw storage from the current block, or from a new one twice as large. Every size is
    // rounded up to Alignment, so every result stays aligned to it. An object never spans
    // two blocks, so a handle resolves within one
    void* Allocate(std::size_t size, std::uint32_t& offset)
    {
        size = (size + Alignment - 1) & ~(Alignment - 1);
        if (size > std::numeric_limits<std::uint32_t>::max() - m_used)
        {
            throw std::length_error("a template too large for its arena");
        }
        if (size > m_left)
        {
            const std::size_t blockSize = std::max(m_nextBlock, size);
            auto& block = m_blocks.emplace_back();
            block.owned.reset(new std::byte[blockSize]);
            block.data = block.owned.get();
            block.start = m_used;
            m_nextBlock = std::min(blockSize * 2, MaxBlockSize);
            m_free = block.data;
            m_left = blockSize;
        }
        void* ptr = m_free;
        offset = m_used;
        m_free += size;
        m_left -= size;
        m_used += static_cast<std::uint32_t>(size);
        return ptr;
    }

    // Where the object at `offset` is while the arena is built
    [[nodiscard]] std::byte* Locate(std::uint32_t offset) const
    {
        assert(offset >= sizeof(detail::ArenaHeader) && offset < m_used);
        const auto& last = m_blocks.back();
        if (offset >= last.start)
        {
            return last.data + (offset - last.start);
        }
        const auto next = std::upper_bound(m_blocks.begin(), m_blocks.end(), offset, [](std::uint32_t value, const Block& block) { return value < block.start; });
        const auto& block = *(next - 1);
        return block.data + (offset - block.start);
    }

    // Calls f(node, offset) for every node, newest first, walking the blocks once
    template<typename F>
    void ForEachNodeNewestFirst(const F& f) noexcept
    {
        auto block = m_blocks.end() - 1;
        for (auto idx = m_objects.size(); idx != 0; --idx)
        {
            const auto offset = m_objects[idx - 1];
            while (offset < block->start)
            {
                --block;
            }
            f(*std::launder(reinterpret_cast<ArenaNode*>(block->data + (offset - block->start))), offset);
        }
    }

    void DestroyNodes() noexcept;

    // Enough for any node, also on 32-bit targets where pointers align to 4 but an
    // InternalValue to 8; new[] gives every block at least this
    static constexpr std::size_t Alignment = 8;
    static_assert(sizeof(detail::ArenaHeader) % Alignment == 0);
    // Small templates fit in it and allocate no block while parsed
    static constexpr std::size_t InlineSize = 512;
    static constexpr std::size_t MaxBlockSize = std::size_t{ 64 } * 1024;

    alignas(Alignment) std::array<std::byte, InlineSize> m_inline{};
    boost::container::small_vector<Block, 4> m_blocks;
    std::byte* m_free = m_inline.data();
    std::size_t m_left = InlineSize;
    std::size_t m_nextBlock = InlineSize * 2;
    // Offsets start after the header the sealed buffer begins with
    std::uint32_t m_used = sizeof(detail::ArenaHeader);
    // The header offsets of the nodes, in the order they were made
    detail::OffsetList m_objects;
    bool m_sealed = false;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_NODE_ARENA_H
