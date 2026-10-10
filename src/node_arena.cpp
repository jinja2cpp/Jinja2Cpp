#include "node_arena.h"

#include "expression_evaluator.h"
#include "renderer.h"
#include "statements.h"

#include <boost/core/span.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace jinja2
{
namespace detail
{
void OffsetList::Grow()
{
    const auto capacity = m_capacity * 2;
    std::unique_ptr<std::uint32_t[]> heap(new std::uint32_t[capacity]);
    std::copy(m_data, m_data + m_size, heap.get());
    m_heap = std::move(heap);
    m_data = m_heap.get();
    m_capacity = capacity;
}

namespace
{
template<typename... Ts>
struct TypeList
{
    template<typename T>
    static constexpr bool Contains()
    {
        return (std::is_same_v<T, Ts> || ...);
    }
};

// The node classes, one per kind
template<typename... Ts>
struct NodeClasses
{
    // Every class is listed if and only if the arena never destroys it
    template<typename... Trivial>
    static constexpr bool TrivialExactly(TypeList<Trivial...> /*list*/)
    {
        return ((IsTriviallyDestructibleNode<Ts> == TypeList<Trivial...>::template Contains<Ts>()) && ...) && (NodeClasses::Has<Trivial>() && ...);
    }
    template<typename T>
    static constexpr bool Has()
    {
        return (std::is_same_v<T, Ts> || ...);
    }
    static constexpr auto MakeOpsTable()
    {
        std::array<NodeOps, std::size_t{ NodeKindCount }> table{};
        ((table[static_cast<std::size_t>(Ts::Kind)] = NodeOps{ &Relocate<Ts>, DestroyFor<Ts>() }), ...);
        return table;
    }
    // Every kind but None has its class, and only one
    static constexpr bool CoverEveryKind()
    {
        std::array<bool, std::size_t{ NodeKindCount }> seen{};
        ((seen[static_cast<std::size_t>(Ts::Kind)] = true), ...);
        for (std::size_t kind = 1; kind != seen.size(); ++kind)
        {
            if (!seen[kind])
            {
                return false;
            }
        }
        return !seen[0] && sizeof...(Ts) + 1 == seen.size();
    }
    // A family's range holds the kinds of its subclasses and no other
    template<typename Owner>
    static constexpr bool FamilyFits()
    {
        return ((std::is_base_of_v<Owner, Ts> == Owner::Family.Contains(Ts::Kind)) && ...);
    }
};

using AllNodeClasses = NodeClasses<FullExpressionEvaluator, ValueRefExpression, SelfRefExpression, SubscriptExpression, LoopAttrExpression, FilteredExpression, ConstantExpression, ScalarConstantExpression, TupleCreator, DictCreator, UnaryExpression, IsExpression, BinaryExpression, InLiteralExpression, ConstFormatExpression, CompareExpression, SliceExpression, CallExpression, ExpressionFilter, IfExpression, ComposedRenderer, RawTextRenderer, ExpressionRenderer, FinalizedExpressionRenderer, TemplateRenderer, ForStatement, IfStatement, ElseBranchStatement, SetLineStatement, SetRawBlockStatement, SetFilteredBlockStatement, BlockStatement, ExtendsStatement, IncludeStatement, ImportStatement, MacroStatement, MacroCallStatement, DoStatement, TransStatement, LoopControlStatement, WithStatement, FilterStatement, AutoescapeStatement, ExpressionFilter::IExpressionFilter, IsExpression::ITester>;
static_assert(AllNodeClasses::CoverEveryKind(), "a node kind without its class in AllNodeClasses");
static_assert(AllNodeClasses::FamilyFits<Expression>() && AllNodeClasses::FamilyFits<IRendererBase>(), "a family whose kinds are not in order");
// The nodes that own nothing, which the arena never destroys (0118 P5b): exactly these.
// A member that owns memory added to one of them must go to the arena instead. The others
// own: string literals and other non-scalar constants (InternalValue), attribute names
// (looked up as std::string), compiled `%` formats and `in` literal lists, the finalize
// callable, macros, which still hold names and parameter lists, and the filter and test
// objects
using TrivialNodes = TypeList<FullExpressionEvaluator, ValueRefExpression, SelfRefExpression, FilteredExpression, ScalarConstantExpression, TupleCreator, DictCreator, UnaryExpression, IsExpression, BinaryExpression, CompareExpression, SliceExpression, CallExpression, ExpressionFilter, IfExpression, ComposedRenderer, RawTextRenderer, ExpressionRenderer, TemplateRenderer, ForStatement, IfStatement, ElseBranchStatement, SetLineStatement, SetRawBlockStatement, SetFilteredBlockStatement, BlockStatement, ExtendsStatement, IncludeStatement, ImportStatement, DoStatement, TransStatement, LoopControlStatement, WithStatement, FilterStatement, AutoescapeStatement>;
static_assert(AllNodeClasses::TrivialExactly(TrivialNodes()), "TrivialNodes must list exactly the node classes that own nothing");

// The operations of each kind, at the index of the kind
constexpr auto OpsTable = AllNodeClasses::MakeOpsTable();
} // namespace

const NodeOps& OpsOf(NodeKind kind)
{
    // Only a node the arena made is relocated or destroyed, and Make records its kind
    assert(kind != NodeKind::None);
    return OpsTable[static_cast<std::size_t>(kind)];
}

namespace
{
ArenaNode& HeaderAt(std::byte* base, std::uint32_t offset)
{
    return *std::launder(reinterpret_cast<ArenaNode*>(base + offset));
}
} // namespace
} // namespace detail

void SealedArena::DestroyNodes() noexcept
{
    if (!m_buffer)
    {
        return;
    }
    const auto header = Header();
    std::byte* const base = m_buffer.get();
    const detail::OffsetTable table(base + detail::LayoutOf(header.size, header.objects).cleanup, header.objects);
    // Newest first, as the nodes were made
    for (auto idx = table.size(); idx != 0; --idx)
    {
        auto& node = detail::HeaderAt(base, table[idx - 1]);
        detail::OpsOf(node.GetKind()).destroy(node);
    }
    m_buffer.reset();
    m_view = ArenaView();
}

void NodeArena::DestroyNodes() noexcept
{
    // A parse that failed: rare, so the nodes that own nothing are skipped one by one
    ForEachNodeNewestFirst(m_objects, [](ArenaNode& node, std::uint32_t /*offset*/) {
        if (const auto destroy = detail::OpsOf(node.GetKind()).destroy)
        {
            destroy(node);
        }
    });
    m_objects.Clear();
    m_owners = 0;
}

void NodeArena::MoveNodes(std::byte* base, detail::RefChecker* refs, std::byte* cleanup) const
{
    const detail::OffsetTable owners(cleanup, m_owners);
    std::size_t owner = 0;
    std::size_t moved = 0;
    try
    {
        // The objects are in the order of their offsets, so the blocks are walked once
        std::size_t blockIdx = 0;
        for (; moved != m_objects.size(); ++moved)
        {
            const auto offset = m_objects[moved];
            while (blockIdx + 1 != m_blocks.size() && offset >= m_blocks[blockIdx + 1].start)
            {
                ++blockIdx;
            }
            const auto& block = m_blocks[blockIdx];
            auto& from = detail::HeaderAt(block.data, offset - block.start);
            const auto& ops = detail::OpsOf(from.GetKind());
            ops.relocate(from, base + offset, refs);
            if (ops.destroy)
            {
                // Make counted the owners by the same classes: a table too short is a bug
                if (owner == m_owners)
                {
                    throw std::logic_error("an arena node that owns something Make did not count");
                }
                owners.Set(owner++, offset);
            }
        }
    }
    catch (...)
    {
        while (owner != 0)
        {
            auto& node = detail::HeaderAt(base, owners[--owner]);
            detail::OpsOf(node.GetKind()).destroy(node);
        }
        throw;
    }
    assert(owner == m_owners);
}

SealedArena NodeArena::SealWith(boost::span<const detail::RootRef> roots)
{
    assert(!m_sealed);
    const auto layout = detail::LayoutOf(m_used, m_owners);
    std::unique_ptr<std::byte[]> buffer(new std::byte[layout.end]);
    std::byte* const base = buffer.get();

    const detail::ArenaHeader header{ m_used, m_owners };
    std::memcpy(base, &header, sizeof(header));
    // The lists and every byte of the nodes; the nodes are then moved over their copies
    for (std::size_t idx = 0; idx != m_blocks.size(); ++idx)
    {
        const auto& block = m_blocks[idx];
        const std::uint32_t end = idx + 1 == m_blocks.size() ? m_used : m_blocks[idx + 1].start;
        std::memcpy(base + block.start, block.data, end - block.start);
    }

    // The bitmaps live in the sealed buffer: where the nodes start, which the checks need
    // first as handles point forward too, and in FULL what the checks reached
    const std::size_t words = detail::BitWords(m_used);
    std::uint64_t* starts = nullptr;
    std::uint64_t* validated = nullptr;
    if constexpr (NodeRefChecks >= 1)
    {
        auto* const bits = reinterpret_cast<std::uint64_t*>(base + layout.starts);
        std::uninitialized_value_construct_n(bits, words);
        starts = std::launder(bits);
        for (std::size_t idx = 0; idx != m_objects.size(); ++idx)
        {
            detail::SetBit(starts, m_objects[idx]);
        }
    }
    if constexpr (NodeRefChecks >= 2)
    {
        auto* const bits = reinterpret_cast<std::uint64_t*>(base + layout.validated);
        std::uninitialized_value_construct_n(bits, words);
        validated = std::launder(bits);
    }
    detail::RefChecker checker(base, m_used, starts, validated);
    MoveNodes(base, NodeRefChecks >= 1 ? &checker : nullptr, base + layout.cleanup);
    if constexpr (NodeRefChecks >= 1)
    {
        for (const auto& root : roots)
        {
            root.check(checker, root.offset, root.size);
        }
    }

    SealedArena sealed(std::move(buffer));
    // Every node is in place: destroys the originals that own something
    ForEachNodeNewestFirst(detail::OffsetTable(base + layout.cleanup, m_owners),
                           [](ArenaNode& from, std::uint32_t /*offset*/) { detail::OpsOf(from.GetKind()).destroy(from); });
    m_objects.Clear();
    m_owners = 0;
    m_blocks.erase(m_blocks.begin() + 1, m_blocks.end());
    m_sealed = true;
    // `sealed` destroys the moved nodes
    if (!checker.Ok())
    {
        throw InvalidNodeRef();
    }
    return sealed;
}
} // namespace jinja2
