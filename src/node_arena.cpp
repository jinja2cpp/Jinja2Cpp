#include "node_arena.h"

#include "expression_evaluator.h"
#include "renderer.h"
#include "statements.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
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
// The node classes, one per kind
template<typename... Ts>
struct NodeClasses
{
    static constexpr auto MakeOpsTable()
    {
        std::array<NodeOps, std::size_t{ NodeKindCount }> table{};
        ((table[static_cast<std::size_t>(Ts::Kind)] = NodeOps{ &Relocate<Ts>, RelocatedFor<Ts>(), &Destroy<Ts> }), ...);
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

using AllNodeClasses = NodeClasses<FullExpressionEvaluator, ValueRefExpression, SelfRefExpression, SubscriptExpression, LoopAttrExpression, FilteredExpression, ConstantExpression, TupleCreator, DictCreator, UnaryExpression, IsExpression, BinaryExpression, CompareExpression, SliceExpression, CallExpression, ExpressionFilter, IfExpression, ComposedRenderer, RawTextRenderer, ExpressionRenderer, FinalizedExpressionRenderer, TemplateRenderer, ForStatement, IfStatement, ElseBranchStatement, SetLineStatement, SetRawBlockStatement, SetFilteredBlockStatement, BlockStatement, ExtendsStatement, IncludeStatement, ImportStatement, MacroStatement, MacroCallStatement, DoStatement, TransStatement, LoopControlStatement, WithStatement, FilterStatement, AutoescapeStatement, ExpressionFilter::IExpressionFilter, IsExpression::ITester>;
static_assert(AllNodeClasses::CoverEveryKind(), "a node kind without its class in AllNodeClasses");
static_assert(AllNodeClasses::FamilyFits<Expression>() && AllNodeClasses::FamilyFits<IRendererBase>(), "a family whose kinds are not in order");

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

bool ArenaView::IsObjectStart(std::uint32_t offset) const
{
    if (!m_base)
    {
        return false;
    }
    detail::ArenaHeader header{};
    std::memcpy(&header, m_base, sizeof(header));
    // The cleanup table follows the nodes, in the order of their offsets
    const std::byte* const table = m_base + header.size;
    std::uint32_t first = 0;
    std::uint32_t count = header.objects;
    while (count != 0)
    {
        const auto half = count / 2;
        std::uint32_t value = 0;
        std::memcpy(&value, table + std::size_t{ first + half } * sizeof(value), sizeof(value));
        if (value == offset)
        {
            return true;
        }
        if (value < offset)
        {
            first += half + 1;
            count -= half + 1;
        }
        else
        {
            count = half;
        }
    }
    return false;
}

void SealedArena::DestroyNodes() noexcept
{
    if (!m_buffer)
    {
        return;
    }
    const auto header = Header();
    std::byte* const base = m_buffer.get();
    // The cleanup table follows the nodes; read by memcpy, as Seal wrote it
    const auto* const table = reinterpret_cast<const std::uint32_t*>(base + header.size);
    // Newest first, as the nodes were made
    for (auto idx = header.objects; idx != 0; --idx)
    {
        std::uint32_t offset = 0;
        std::memcpy(&offset, table + (idx - 1), sizeof(offset));
        auto& node = detail::HeaderAt(base, offset);
        detail::OpsOf(node.GetKind()).destroy(node);
    }
    m_buffer.reset();
    m_view = ArenaView();
}

void NodeArena::DestroyNodes() noexcept
{
    ForEachNodeNewestFirst([](ArenaNode& node, std::uint32_t /*offset*/) { detail::OpsOf(node.GetKind()).destroy(node); });
    m_objects.Clear();
}

SealedArena NodeArena::Seal()
{
    assert(!m_sealed);
    const auto objects = static_cast<std::uint32_t>(m_objects.size());
    const std::size_t tableSize = std::size_t{ objects } * sizeof(std::uint32_t);
    std::unique_ptr<std::byte[]> buffer(new std::byte[m_used + tableSize]);
    std::byte* const base = buffer.get();

    const detail::ArenaHeader header{ m_used, objects };
    std::memcpy(base, &header, sizeof(header));
    // The lists and every byte of the nodes; the nodes are then moved over their copies
    for (std::size_t idx = 0; idx != m_blocks.size(); ++idx)
    {
        const auto& block = m_blocks[idx];
        const std::uint32_t end = idx + 1 == m_blocks.size() ? m_used : m_blocks[idx + 1].start;
        std::memcpy(base + block.start, block.data, end - block.start);
    }

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
            detail::OpsOf(from.GetKind()).relocate(from, base + offset);
        }
    }
    catch (...)
    {
        while (moved != 0)
        {
            auto& node = detail::HeaderAt(base, m_objects[--moved]);
            detail::OpsOf(node.GetKind()).destroy(node);
        }
        throw;
    }
    std::memcpy(base + m_used, m_objects.data(), tableSize);

    SealedArena sealed(std::move(buffer));
    // Every node is in place: fixes up the ones that hold addresses inside themselves and
    // destroys the originals, in one pass
    const auto view = sealed.View();
    ForEachNodeNewestFirst([base, &view](ArenaNode& from, std::uint32_t offset) {
        const auto& ops = detail::OpsOf(from.GetKind());
        if (ops.relocated)
        {
            ops.relocated(detail::HeaderAt(base, offset), view);
        }
        ops.destroy(from);
    });
    m_objects.Clear();
    m_blocks.erase(m_blocks.begin() + 1, m_blocks.end());
    m_sealed = true;
    return sealed;
}
} // namespace jinja2
