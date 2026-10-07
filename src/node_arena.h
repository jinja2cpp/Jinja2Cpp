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

namespace jinja2
{
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
};
// The number of kinds, None included
constexpr std::uint8_t NodeKindCount = static_cast<std::uint8_t>(NodeKind::AutoescapeStmt) + 1;

// The header every arena node starts with, after its vptr
class ArenaNode
{
public:
    [[nodiscard]] NodeKind GetKind() const { return m_kind; }

private:
    friend class NodeArena;
    NodeKind m_kind = NodeKind::None;
};

class ArenaView;
class NodeArena;
class SealedArena;

namespace detail
{
template<typename Derived>
class ArenaAccess;
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
        if constexpr (HasKindMatch<T>::value)
        {
            return T::MatchesKind(kind);
        }
        else
        {
            return kind == T::Kind;
        }
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
    const Derived& Self() const { return static_cast<const Derived&>(*this); }
};

// How the arena moves and destroys a node of one kind
struct NodeOps
{
    // Moves the node to where the arena keeps its header at `to`, leaving the original to
    // be destroyed
    void (*relocate)(ArenaNode& from, std::byte* to);
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

// Where the ArenaNode header of a T lies in it
template<typename T>
std::ptrdiff_t HeaderOffset(T& node)
{
    return reinterpret_cast<std::byte*>(&static_cast<ArenaNode&>(node)) - reinterpret_cast<std::byte*>(&node);
}

template<typename T>
void Relocate(ArenaNode& from, std::byte* to)
{
    auto& node = static_cast<T&>(from);
    new (to - HeaderOffset(node)) T(std::move(node));
}

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
    OffsetList& operator=(const OffsetList&) = delete;

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
        assert(ref && InRange(ref.m_offset, sizeof(T)));
        auto& node = *std::launder(reinterpret_cast<T*>(m_base + ref.m_offset));
        // A handle from another template's tree rarely lands on a node of its type
        if constexpr (detail::HasKind<T>::value)
        {
            assert(KindIs<T>(static_cast<ArenaNode&>(node).GetKind()));
        }
        return node;
    }

    template<typename T>
    boost::span<const T> operator[](ArenaSpan<T> list) const
    {
        // An empty list has offset 0, which is still inside the buffer: no branch
        assert(list.empty() ? list.m_offset == 0 : InRange(list.m_offset, list.m_size * sizeof(T)));
        return boost::span<const T>(std::launder(reinterpret_cast<const T*>(m_base + list.m_offset)), list.m_size);
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

    // Views of the same tree
    friend bool operator==(const ArenaView& lhs, const ArenaView& rhs) { return lhs.m_base == rhs.m_base; }
    friend bool operator!=(const ArenaView& lhs, const ArenaView& rhs) { return !(lhs == rhs); }

private:
    friend class NodeArena;
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
        std::uninitialized_copy(items.begin(), items.end(), reinterpret_cast<Item*>(Locate(ref.m_offset) + sizeof(T)));
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
    // at the offsets their handles hold. Nothing is made after this
    SealedArena Seal();

private:
    struct Block
    {
        std::byte* data;
        // The offset of the block's first byte
        std::uint32_t start;
        std::unique_ptr<std::byte[]> owned;
    };

    // A node in `size` bytes, sizeof(T) or more
    template<typename T, typename... Args>
    NodeRef<T> MakeSized(std::size_t size, Args&&... args)
    {
        static_assert(std::is_base_of_v<ArenaNode, T>, "an arena node starts with the ArenaNode header");
        static_assert(alignof(T) <= Alignment, "the arena aligns nodes to 8 bytes");
        assert(!m_sealed);
        assert(detail::OpsOf(T::Kind).destroy == &detail::Destroy<T>);
        std::uint32_t offset = 0;
        void* place = Allocate(size, offset);
        // Room for the record first, so that it cannot fail once the node exists; a node
        // that throws from its constructor leaves only unused bytes behind
        m_objects.Reserve();
        T* node = new (place) T(std::forward<Args>(args)...);
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
    void ForEachNodeNewestFirst(F&& f) noexcept
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

    alignas(Alignment) std::array<std::byte, InlineSize> m_inline;
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
