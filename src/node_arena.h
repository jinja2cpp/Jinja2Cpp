#ifndef JINJA2CPP_SRC_NODE_ARENA_H
#define JINJA2CPP_SRC_NODE_ARENA_H

#include <boost/container/small_vector.hpp>
#include <boost/core/span.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

// The handles a parse tree is linked with (docs/design/0118-parse-tree-arena-plan.md).
//
// Every node of a template's tree belongs to the template's NodeArena, and nodes refer to
// each other by NodeRef and ArenaSpan, never by owning pointers. A handle is resolved
// only through the arena that made it: NodeArena at Load, ArenaView (RenderContext::Nodes)
// during a render. In phase P3 a handle still wraps a pointer and each node is a heap
// object the arena owns (lists and the records of what to destroy are bump-allocated
// from a few blocks); P4 makes the arena one buffer and the handles 32-bit offsets into
// it, without changing how nodes use them.

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

// A link to a node of T, or a null link
template<typename T>
class NodeRef
{
public:
    NodeRef() = default;
    // To a base of T
    template<typename U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
    NodeRef(NodeRef<U> other) // NOLINT(google-explicit-constructor)
        : m_node(other.m_node)
    {
    }

    explicit operator bool() const { return m_node != nullptr; }

private:
    friend class ArenaView;
    friend class NodeArena;
    template<typename U>
    friend class NodeRef;

    explicit NodeRef(T* node)
        : m_node(node)
    {
    }

    T* m_node = nullptr;
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

    ArenaSpan(const T* data, std::uint32_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    const T* m_data = nullptr;
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
} // namespace detail

// Resolves the handles of one template's tree
class ArenaView
{
public:
    template<typename T>
    T& operator[](NodeRef<T> ref) const
    {
        assert(ref);
        return *ref.m_node;
    }

    template<typename T>
    boost::span<const T> operator[](ArenaSpan<T> list) const
    {
        return boost::span<const T>(list.m_data, list.m_size);
    }

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
        return KindIs<T>((*this)[ref].GetKind());
    }

    // True if a node of this kind is a T
    template<typename T>
    [[nodiscard]] static bool KindIs(NodeKind kind)
    {
        if constexpr (detail::HasKindMatch<T>::value)
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
        return Is<T>(ref) ? NodeRef<T>(static_cast<T*>(ref.m_node)) : NodeRef<T>();
    }

    // The node, which is a T
    template<typename T, typename U>
    [[nodiscard]] T& Get(NodeRef<U> ref) const
    {
        const auto node = As<T>(ref);
        assert(node);
        return (*this)[node];
    }
};

// Owns the nodes of one template. Nodes are made during the parse only; after Seal the
// tree is immutable, so concurrent renders read it without locks
class NodeArena
{
public:
    NodeArena() = default;
    NodeArena(const NodeArena&) = delete;
    NodeArena(NodeArena&& other) noexcept
        : m_blocks(std::move(other.m_blocks))
        , m_free(std::exchange(other.m_free, nullptr))
        , m_left(std::exchange(other.m_left, 0))
        , m_nextBlock(other.m_nextBlock)
        , m_owned(std::exchange(other.m_owned, nullptr))
        , m_sealed(other.m_sealed)
    {
        other.m_blocks.clear();
    }
    NodeArena& operator=(const NodeArena&) = delete;
    NodeArena& operator=(NodeArena&& other) noexcept
    {
        if (this != &other)
        {
            DestroyNodes();
            m_blocks = std::move(other.m_blocks);
            other.m_blocks.clear();
            m_free = std::exchange(other.m_free, nullptr);
            m_left = std::exchange(other.m_left, 0);
            m_nextBlock = other.m_nextBlock;
            m_owned = std::exchange(other.m_owned, nullptr);
            m_sealed = other.m_sealed;
        }
        return *this;
    }
    ~NodeArena() { DestroyNodes(); }

    template<typename T, typename... Args>
    NodeRef<T> Make(Args&&... args)
    {
        static_assert(std::is_base_of_v<ArenaNode, T>, "an arena node starts with the ArenaNode header");
        assert(!m_sealed);
        auto* record = static_cast<Owned*>(Allocate(sizeof(Owned)));
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        static_cast<ArenaNode&>(*node).m_kind = T::Kind;
        NodeRef<T> ref(node.get());
        m_owned = new (record) Owned{ node.release(), [](void* ptr) noexcept { delete static_cast<T*>(ptr); }, m_owned };
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
        assert(items.size() <= std::numeric_limits<std::uint32_t>::max());
        static_assert(alignof(T) <= Alignment, "the arena aligns lists to 8 bytes");
        T* data = static_cast<T*>(Allocate(items.size_bytes()));
        std::uninitialized_copy(items.begin(), items.end(), data);
        return ArenaSpan<T>(data, static_cast<std::uint32_t>(items.size()));
    }
    template<typename Container>
    auto MakeSpan(const Container& items)
    {
        using T = typename Container::value_type;
        return MakeSpan(boost::span<const T>(items.data(), items.size()));
    }

    // The parse is over: no node is made after this
    void Seal() { m_sealed = true; }

    // Holds the arena's base once handles are offsets (0118 P4)
    [[nodiscard]] ArenaView View() const { return {}; } // NOLINT(readability-convert-member-functions-to-static)
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
    template<typename T, typename U>
    [[nodiscard]] bool Is(NodeRef<U> ref) const
    {
        return View().Is<T>(ref);
    }
    template<typename T, typename U>
    [[nodiscard]] NodeRef<T> As(NodeRef<U> ref) const
    {
        return View().As<T>(ref);
    }
    template<typename T, typename U>
    [[nodiscard]] T& Get(NodeRef<U> ref) const
    {
        return View().Get<T>(ref);
    }

private:
    // Which node to destroy, newest first
    struct Owned
    {
        void* node;
        void (*destroy)(void*) noexcept;
        Owned* next;
    };
    static_assert(std::is_trivially_destructible_v<Owned>);

    // Raw storage from the current block, or from a new one twice as large. Every size is
    // rounded up to Alignment, so every result stays aligned to it
    void* Allocate(std::size_t size)
    {
        size = (size + Alignment - 1) & ~(Alignment - 1);
        if (size > m_left)
        {
            const std::size_t blockSize = std::max(m_nextBlock, size);
            std::unique_ptr<std::byte[]> block(new std::byte[blockSize]);
            m_blocks.push_back(std::move(block));
            m_nextBlock = std::min(blockSize * 2, MaxBlockSize);
            m_free = m_blocks.back().get();
            m_left = blockSize;
        }
        void* ptr = m_free;
        m_free += size;
        m_left -= size;
        return ptr;
    }

    void DestroyNodes() noexcept
    {
        for (const Owned* record = m_owned; record; record = record->next)
        {
            record->destroy(record->node);
        }
        m_owned = nullptr;
    }

    static constexpr std::size_t Alignment = alignof(void*);
    static_assert(alignof(Owned) <= Alignment);
    static constexpr std::size_t FirstBlockSize = 512;
    static constexpr std::size_t MaxBlockSize = std::size_t{ 64 } * 1024;

    boost::container::small_vector<std::unique_ptr<std::byte[]>, 4> m_blocks;
    std::byte* m_free = nullptr;
    std::size_t m_left = 0;
    std::size_t m_nextBlock = FirstBlockSize;
    Owned* m_owned = nullptr;
    bool m_sealed = false;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_NODE_ARENA_H
