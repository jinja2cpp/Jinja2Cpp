#include "../src/expression_evaluator.h"
#include "../src/internal_value.h"
#include "../src/node_arena.h"
#include "../src/renderer.h"
#include "../src/slot_frame.h"
#include "../src/statements.h"
#include "../src/template_impl.h"
#include "../src/template_slots.h"

#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <array>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace jinja2;

namespace jinja2::detail
{
// Handles from raw offsets and the bytes of a sealed tree, to corrupt a tree on purpose
struct ArenaTestAccess
{
    template<typename T>
    static NodeRef<T> Ref(std::uint32_t offset)
    {
        return NodeRef<T>(offset);
    }
    template<typename T>
    static std::uint32_t Offset(NodeRef<T> ref)
    {
        return ref.m_offset;
    }
    template<typename T>
    static std::uint32_t Offset(ArenaSpan<T> list)
    {
        return list.m_offset;
    }
    template<typename T>
    static ArenaSpan<T> Span(std::uint32_t offset, std::uint32_t size)
    {
        return ArenaSpan<T>(offset, size);
    }
    static std::byte* Bytes(const SealedArena& tree) { return tree.m_buffer.get(); }
    // The nodes a sealed tree destroys
    static std::uint32_t Owners(const ArenaView& tree)
    {
        ArenaHeader header{};
        std::memcpy(&header, tree.m_base, sizeof(header));
        return header.objects;
    }
};
} // namespace jinja2::detail

namespace
{
using Access = jinja2::detail::ArenaTestAccess;
} // namespace

// The handles of a parse tree (docs/design/0118-parse-tree-arena-plan.md, phase P3)

// The node classes' vtables are not exported from a shared library, so the tests that make
// nodes run against the static one only
#ifndef JINJA2CPP_LINK_AS_SHARED
TEST(NodeArenaTest, MakeRecordsTheKind)
{
    NodeArena nodes;
    auto constant = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 5 }));
    NodeRef<Expression> expr = constant;
    EXPECT_EQ(NodeKind::ConstantExpr, nodes[expr].GetKind());
    EXPECT_EQ(5, *GetIf<int64_t>(nodes[expr].GetConstant(nodes)));
}

TEST(NodeArenaTest, DowncastChecksTheKind)
{
    NodeArena nodes;
    NodeRef<Expression> name = ValueRefExpression::Make(nodes, "x");
    NodeRef<Expression> self = SelfRefExpression::Make(nodes);
    NodeRef<Expression> constant = nodes.Make<ConstantExpression>(InternalValue());

    // `self` is a variable too, but a variable is not `self`
    EXPECT_TRUE(nodes.Is<ValueRefExpression>(name));
    EXPECT_TRUE(nodes.Is<ValueRefExpression>(self));
    EXPECT_TRUE(nodes.Is<SelfRefExpression>(self));
    EXPECT_FALSE(nodes.Is<SelfRefExpression>(name));
    EXPECT_FALSE(nodes.Is<ValueRefExpression>(constant));

    EXPECT_TRUE(nodes.As<ConstantExpression>(constant));
    EXPECT_FALSE(nodes.As<ConstantExpression>(name));
    EXPECT_FALSE(nodes.As<ConstantExpression>(NodeRef<Expression>()));
    EXPECT_EQ("x", nodes.Get<ValueRefExpression>(name).GetName(nodes));
}

TEST(NodeArenaTest, DowncastToAClassWithSubclasses)
{
    NodeArena nodes;
    NodeRef<IRendererBase> macro = nodes.Make<MacroStatement>("m", MacroParams());
    NodeRef<IRendererBase> call = nodes.Make<MacroCallStatement>("m", CallParamsInfo(), MacroParams());
    TargetNode target;
    target.name = nodes.MakeText("x");
    target.hash = HashedName::Hash("x");
    NodeRef<IRendererBase> rawSet = nodes.Make<SetRawBlockStatement>(nodes.MakeSpan(std::array<TargetNode, 1>{ target }));

    // A call block's caller is a macro; a macro is not a call block
    EXPECT_TRUE(nodes.Is<MacroStatement>(macro));
    EXPECT_TRUE(nodes.Is<MacroStatement>(call));
    EXPECT_TRUE(nodes.Is<MacroCallStatement>(call));
    EXPECT_FALSE(nodes.Is<MacroCallStatement>(macro));

    EXPECT_TRUE(nodes.Is<SetBlockStatement>(rawSet));
    EXPECT_FALSE(nodes.Is<SetFilteredBlockStatement>(rawSet));
    EXPECT_FALSE(nodes.Is<SetBlockStatement>(macro));
}

TEST(NodeArenaTest, SpanKeepsACopyOfTheItems)
{
    NodeArena nodes;
    std::vector<NodeRef<Expression>> items{ nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 })),
                                            nodes.Make<ConstantExpression>(InternalValue(int64_t{ 2 })) };
    auto span = nodes.MakeSpan(items);
    items.clear();

    ASSERT_EQ(2u, span.size());
    const auto view = nodes[span];
    EXPECT_EQ(1, *GetIf<int64_t>(nodes[view[0]].GetConstant(nodes)));
    EXPECT_EQ(2, *GetIf<int64_t>(nodes[view[1]].GetConstant(nodes)));
    EXPECT_TRUE(nodes.MakeSpan(items).empty());
}

TEST(NodeArenaTest, SealingKeepsTheHandles)
{
    NodeArena nodes;
    std::vector<NodeRef<Expression>> items;
    // Enough nodes and lists to fill several blocks
    for (int64_t n = 0; n != 2000; ++n)
    {
        items.emplace_back(nodes.Make<ConstantExpression>(InternalValue(std::string(40, static_cast<char>('a' + (n % 26))) + std::to_string(n))));
    }
    const auto span = nodes.MakeSpan(items);

    // The test reads the nodes through the list: no node holds it
    SealedArena sealed = nodes.Seal(span);
    SealedArena moved(std::move(sealed));
    SealedArena assigned;
    assigned = std::move(moved);

    const auto view = assigned.View();
    const auto list = view[span];
    ASSERT_EQ(2000u, list.size());
    EXPECT_EQ(std::string(40, 'x') + "1999", AsString(*view[list[1999]].GetConstant(nodes)));
    EXPECT_EQ(std::string(40, 'a') + "0", AsString(*view[items[0]].GetConstant(nodes)));
}

// The sealed tree has no slack: it is as large as the objects in it
TEST(NodeArenaTest, SealedTreeIsExact)
{
    NodeArena nodes;
    auto first = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    auto second = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 2 }));
    const NodeRef<Expression> exprs[] = { first, second };
    const auto span = nodes.MakeSpan(boost::span<const NodeRef<Expression>>(exprs));
    const SealedArena sealed = nodes.Seal(span);

    const auto view = sealed.View();
    EXPECT_EQ(2, *GetIf<int64_t>(view[view[span][1]].GetConstant(nodes)));
    EXPECT_EQ(NodeKind::ConstantExpr, view[first].GetKind());
}

// A template whose nodes fill several blocks: Seal moves every node out of them
TEST(NodeArenaTest, TemplateOfSeveralBlocksRendersAfterSealing)
{
    std::string source;
    std::string expected;
    for (int n = 0; n != 200; ++n)
    {
        source += "{% for k in ['k" + std::to_string(n) + "'] %}{{ k ~ x }}{% endfor %},";
        expected += "k" + std::to_string(n) + "!,";
    }
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    EXPECT_EQ(expected, tpl.RenderAsString({ { "x", "!" } }).value());
}

// A loop's names point into its targets, wherever the node is: the `set` makes `k` a
// lookup by name, which finds the loop's `k` by those names
TEST(NodeArenaTest, LoopNamesFollowTheLoop)
{
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load("{% for k, v in d|dictsort %}{% set k = k ~ v %}{{ k }};{% endfor %}"));
    EXPECT_EQ("a1;b2;", tpl.RenderAsString({ { "d", jinja2::ValuesMap{ { "a", 1 }, { "b", 2 } } } }).value());
}

// Seal destroys and keeps a cleanup entry for the nodes that own something only (0118 P5b)
TEST(NodeArenaTest, SealKeepsCleanupOnlyForOwners)
{
    NodeArena nodes;
    const NodeRef<Expression> first = MakeConstant(nodes, InternalValue(int64_t{ 1 }));
    const NodeRef<Expression> second = MakeConstant(nodes, InternalValue(std::string(40, 'x')));
    const NodeRef<IRendererBase> renderer = nodes.Make<ExpressionRenderer>(first);
    const NodeRef<Expression> exprs[] = { first, second };
    const auto span = nodes.MakeSpan(boost::span<const NodeRef<Expression>>(exprs));
    const SealedArena tree = nodes.Seal(span, renderer);

    // Only the string owns something: the number is kept in its node
    EXPECT_EQ(1U, Access::Owners(tree.View()));
    EXPECT_EQ(NodeKind::ScalarConstantExpr, tree[first].GetKind());
    EXPECT_EQ(1, *GetIf<int64_t>(tree[first].GetConstant(nodes)));
    EXPECT_EQ(std::string(40, 'x'), AsString(*tree[second].GetConstant(nodes)));
}

namespace
{
struct OwnersCase
{
    const char* source;
    std::uint32_t owners;
    // A wide template: a wide literal format is not prepared at Load
    std::uint32_t wideOwners;
};

template<typename CharT>
std::uint32_t OwnersOf(const std::string& source)
{
    TemplateImpl<CharT> impl(nullptr);
    const auto error = impl.Load(std::basic_string<CharT>(source.begin(), source.end()), std::string());
    EXPECT_FALSE(error.has_value()) << source;
    return Access::Owners(impl.Nodes());
}
} // namespace

// A template keeps a destructor entry only for the nodes that own memory: names, call
// arguments, comparisons and scalar literals live in the tree (0118 P5b)
TEST(NodeArenaTest, TemplateKeepsOnlyOwners)
{
    const OwnersCase cases[] = {
        { "{{ a_long_variable_name_x }}", 0, 0 },
        { "{{ self }}", 0, 0 },
        { "{{ 1 }}{{ 2.5 }}{{ true }}{{ none }}", 0, 0 },
        { "{{ 'a_long_string_literal_xyz' }}", 1, 1 },
        { "{{ f(1, a_long_keyword_name=2) }}", 0, 0 },
        { "{{ a < b <= c }}", 0, 0 },
        { "{{ a + 1 }}", 0, 0 },
        { "{{ x | upper }}", 1, 1 },
        { "{{ x | int(1, 2, 3, 4) }}", 1, 1 },
        { "{{ x in [1, 2] }}", 1, 1 },
        { "{{ '%d' % x }}", 2, 1 },
        { "{{ x.y }}", 1, 1 },
        { "{% for a_long_loop_variable_name, (b, c) in x %}{{ b }}{% endfor %}", 0, 0 },
        { "{% set a_long_set_target_name = 1 %}{% set ns.attr = 2 %}", 0, 0 },
        { "{% set a, b %}t{% endset %}{% set c | upper %}t{% endset %}", 1, 1 },
    };
    for (const auto& c : cases)
    {
        EXPECT_EQ(c.owners, OwnersOf<char>(c.source)) << c.source;
        EXPECT_EQ(c.wideOwners, OwnersOf<wchar_t>(c.source)) << c.source;
    }
}

namespace
{
// Two templates with the same nodes at the same places, which read the names in another
// order: one loaded where the other was freed is likely to reuse its addresses
constexpr const char* LookupFirst = "{{ a }}{{ b }}{% for x in [a, b] %}{{ x }}{% endfor %}";
constexpr const char* LookupSecond = "{{ b }}{{ a }}{% for x in [b, a] %}{{ x }}{% endfor %}";
ValuesMap LookupParams()
{
    return { { "a", 1 }, { "b", 2 } };
}
} // namespace

// The lookup cache keys its entries by the address of a name node and drops none when a
// template is freed: each render takes an epoch of its own, so the next template at the
// same addresses never sees the entries of the freed one (0118 P5b)
TEST(NodeArenaTest, LookupCacheIgnoresAFreedTemplatesEntries)
{
    for (int round = 0; round != 16; ++round)
    {
        {
            Template first;
            ASSERT_TRUE(first.Load(LookupFirst));
            EXPECT_EQ("1212", first.RenderAsString(LookupParams()).value());
        }
        Template second;
        ASSERT_TRUE(second.Load(LookupSecond));
        EXPECT_EQ("2121", second.RenderAsString(LookupParams()).value());
    }
}

// The same when another thread frees the template and loads the next one, while one
// thread renders them all with its cache
TEST(NodeArenaTest, LookupCacheIgnoresEntriesOfATemplateFreedOnAnotherThread)
{
    std::mutex mutex;
    std::condition_variable changed;
    Template* job = nullptr;
    bool stop = false;
    std::string result;
    std::thread renderer([&mutex, &changed, &job, &stop, &result] {
        std::unique_lock<std::mutex> lock(mutex);
        for (;;)
        {
            changed.wait(lock, [&job, &stop] { return job || stop; });
            if (!job)
            {
                return;
            }
            auto rendered = job->RenderAsString(LookupParams());
            result = rendered ? *rendered : std::string("error");
            job = nullptr;
            changed.notify_all();
        }
    });
    auto renderThere = [&mutex, &changed, &job, &result](Template& tpl) {
        std::unique_lock<std::mutex> lock(mutex);
        job = &tpl;
        changed.notify_all();
        changed.wait(lock, [&job] { return !job; });
        return result;
    };
    for (int round = 0; round != 16; ++round)
    {
        {
            Template first;
            // Not ASSERT: the renderer thread must still be stopped and joined
            EXPECT_TRUE(first.Load(LookupFirst));
            EXPECT_EQ("1212", renderThere(first));
        }
        Template second;
        EXPECT_TRUE(second.Load(LookupSecond));
        EXPECT_EQ("2121", renderThere(second));
    }
    {
        const std::scoped_lock lock(mutex);
        stop = true;
    }
    changed.notify_all();
    renderer.join();
}

// A render links to the trees of the templates it runs by slot (phase P4b)
TEST(NodeArenaTest, TemplateSlotsOfOneRender)
{
    NodeArena first;
    first.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    NodeArena second;
    second.Make<ConstantExpression>(InternalValue(int64_t{ 2 }));
    const SealedArena firstTree = first.Seal();
    const SealedArena secondTree = second.Seal();

    TemplateSlots templates;
    const auto a = templates.Add(firstTree.View());
    const auto b = templates.Add(secondTree.View());
    EXPECT_NE(a.Slot(), b.Slot());
    EXPECT_TRUE(templates[a] == firstTree.View());
    EXPECT_TRUE(templates[b] == secondTree.View());
    // A template takes one slot however often the render asks for it
    const auto again = templates.Add(firstTree.View());
    EXPECT_EQ(a.Slot(), again.Slot());
    EXPECT_EQ(a.Generation(), again.Generation());
}

// Past a few templates the table finds a tree by hash, still one slot each
TEST(NodeArenaTest, TemplateSlotsOfManyTemplates)
{
    std::vector<SealedArena> trees;
    trees.reserve(20);
    for (int64_t n = 0; n != 20; ++n)
    {
        NodeArena nodes;
        nodes.Make<ConstantExpression>(InternalValue(n));
        trees.push_back(nodes.Seal());
    }

    TemplateSlots templates;
    std::vector<TemplateHandle> handles;
    handles.reserve(trees.size());
    for (const auto& tree : trees)
    {
        handles.push_back(templates.Add(tree.View()));
    }
    for (std::size_t n = 0; n != trees.size(); ++n)
    {
        EXPECT_EQ(n, handles[n].Slot());
        EXPECT_EQ(n, templates.Add(trees[n].View()).Slot());
        EXPECT_TRUE(templates[handles[n]] == trees[n].View());
    }
}

// A handle of another render's table, or one no table made, resolves nowhere: its template
// may be gone
TEST(NodeArenaTest, TemplateSlotsRejectAnotherRendersHandle)
{
    NodeArena nodes;
    nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    const SealedArena tree = nodes.Seal();

    TemplateSlots render1;
    const auto handle = render1.Add(tree.View());
    TemplateSlots render2;
    EXPECT_THROW((void)render2[handle], std::logic_error);
    const TemplateHandle pastTheEnd(handle.Slot() + 1, handle.Generation());
    EXPECT_THROW((void)render1[pastTheEnd], std::logic_error);
    // The generation tells the renders apart unless the checks are OFF
    render2.Add(tree.View());
    if constexpr (NodeRefChecks >= 1)
    {
        EXPECT_THROW((void)render2[handle], std::logic_error);
        EXPECT_THROW((void)render1[TemplateHandle()], std::logic_error);
    }
}

namespace
{
using IExpressionFilter = ExpressionFilter::IExpressionFilter;

// Counts the filters alive, wherever the arena moved them
struct FilterCount
{
    int alive = 0;
};

class AliveMark
{
public:
    explicit AliveMark(FilterCount& count)
        : m_count(&count)
    {
        ++m_count->alive;
    }
    AliveMark(const AliveMark& other)
        : m_count(other.m_count)
    {
        ++m_count->alive;
    }
    AliveMark& operator=(const AliveMark&) = delete;
    ~AliveMark() { --m_count->alive; }

private:
    FilterCount* m_count;
};

class CountingFilter : public IExpressionFilter
{
public:
    CountingFilter(FilterCount& count, std::string text)
        : m_mark(count)
        , m_text(std::move(text))
    {
    }

    InternalValue Filter(const InternalValue& /*baseVal*/, RenderContext& /*context*/) override { return InternalValue(); }
    [[nodiscard]] std::string GetArgumentsError() const override { return m_text; }
    void VisitRefs(jinja2::detail::RefChecker& /*refs*/) const override {}

private:
    AliveMark m_mark;
    std::string m_text;
};

// A base before the interface, so that the interface does not start the object
struct LeadingBase
{
    LeadingBase() = default;
    LeadingBase(const LeadingBase&) = default;
    LeadingBase(LeadingBase&&) = default;
    LeadingBase& operator=(const LeadingBase&) = default;
    LeadingBase& operator=(LeadingBase&&) = default;
    virtual ~LeadingBase() = default;
    std::int64_t padding[3] = {};
};

class OffsetFilter
    : public LeadingBase
    , public CountingFilter
{
public:
    using CountingFilter::CountingFilter;
};

std::string LongText(int n)
{
    // Longer than a short string, so that a move that copied the bytes would show
    return "a filter's text, past the short string buffer, number " + std::to_string(n);
}
} // namespace

// Filters and tests are arena nodes that move themselves at Seal (phase P4c)
TEST(NodeArenaTest, FilterObjectsLiveInTheArena)
{
    constexpr int count = 300;
    FilterCount filters;
    for (const bool seal : { false, true })
    {
        {
            NodeArena nodes;
            std::vector<NodeRef<IExpressionFilter>> refs;
            refs.reserve(count);
            for (int n = 0; n != count; ++n)
            {
                refs.push_back(n % 2 ? nodes.MakeObject<IExpressionFilter, CountingFilter>(filters, LongText(n))
                                     : nodes.MakeObject<IExpressionFilter, OffsetFilter>(filters, LongText(n)));
                // Other nodes between them, so that they fill several blocks
                nodes.Make<ConstantExpression>(InternalValue(int64_t{ n }));
            }
            ASSERT_EQ(count, filters.alive);
            EXPECT_EQ(NodeKind::FilterObject, nodes[refs[0]].GetKind());
            EXPECT_EQ(LongText(0), nodes[refs[0]].GetArgumentsError());
            if (seal)
            {
                SealedArena sealed = nodes.Seal(nodes.MakeSpan(refs));
                SealedArena moved(std::move(sealed));
                const auto view = moved.View();
                EXPECT_EQ(count, filters.alive);
                for (int n = 0; n != count; ++n)
                {
                    EXPECT_EQ(LongText(n), view[refs[static_cast<std::size_t>(n)]].GetArgumentsError());
                }
                EXPECT_EQ(NodeKind::FilterObject, view[refs[1]].GetKind());
            }
        }
        EXPECT_EQ(0, filters.alive);
    }
}

// A filter's state prepared at Load moves with it: `format` parses a literal format once
TEST(NodeArenaTest, FilterKeepsItsStateAcrossSeal)
{
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load("{{ '%s=%d' | format(x, 2) }};{{ 'v' | default(x) | upper }};{{ x is equalto 'k' }}"));
    EXPECT_EQ("k=2;V;True", tpl.RenderAsString({ { "x", "k" } }).value());
}

// A link into another template's tree is checked where it is followed, unless the checks
// are OFF: its own template's Seal cannot check it (phase P4c)
TEST(NodeArenaTest, ResolveChecksTemplateNodes)
{
    NodeArena nodes;
    const NodeRef<Expression> constant = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 7 }));
    const NodeRef<IRendererBase> text = nodes.Make<RawTextRenderer>("text", std::size_t{ 4 });
    const SealedArena tree = nodes.Seal(constant, text);
    TemplateSlots templates;
    const auto handle = templates.Add(tree.View());

    const auto found = templates.Resolve(TemplateNode<Expression>{ handle, constant });
    EXPECT_TRUE(found.nodes == tree.View());
    EXPECT_EQ(7, *GetIf<int64_t>(found.node.GetConstant(nodes)));
    EXPECT_EQ(NodeKind::RawText, templates.Resolve(TemplateNode<IRendererBase>{ handle, text }).node.GetKind());

    if constexpr (NodeRefChecks >= 1)
    {
        // A node of another family, the middle of a node, past the end, no node at all
        const auto wrongFamily = Access::Ref<IRendererBase>(Access::Offset(constant));
        EXPECT_THROW((void)templates.Resolve(TemplateNode<IRendererBase>{ handle, wrongFamily }), InvalidNodeRef);
        const auto inside = Access::Ref<Expression>(Access::Offset(constant) + 8);
        EXPECT_THROW((void)templates.Resolve(TemplateNode<Expression>{ handle, inside }), InvalidNodeRef);
        const auto pastTheEnd = Access::Ref<Expression>(1U << 20);
        EXPECT_THROW((void)templates.Resolve(TemplateNode<Expression>{ handle, pastTheEnd }), InvalidNodeRef);
        EXPECT_THROW((void)templates.Resolve(TemplateNode<Expression>{ handle, NodeRef<Expression>() }), InvalidNodeRef);
    }
}

// With FULL checks every access checks its handle
TEST(NodeArenaTest, FullChecksEveryAccess)
{
    if constexpr (NodeRefChecks < 2)
    {
        GTEST_SKIP() << "node reference checks below FULL";
    }
    NodeArena nodes;
    std::vector<NodeRef<Expression>> items{ nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 })),
                                            nodes.Make<ConstantExpression>(InternalValue(int64_t{ 2 })) };
    const auto span = nodes.MakeSpan(items);
    const NodeRef<IRendererBase> text = nodes.Make<RawTextRenderer>("text", std::size_t{ 4 });
    EXPECT_THROW((void)nodes[Access::Ref<Expression>(Access::Offset(text))], InvalidNodeRef);
    EXPECT_THROW((void)nodes[Access::Ref<Expression>(1U << 20)], InvalidNodeRef);
    const SealedArena tree = nodes.Seal(span, text);
    const auto view = tree.View();

    EXPECT_THROW((void)view[Access::Ref<Expression>(Access::Offset(text))], InvalidNodeRef);
    EXPECT_THROW((void)view[Access::Ref<Expression>(1U << 20)], InvalidNodeRef);
    EXPECT_THROW((void)view[Access::Ref<Expression>(Access::Offset(items[0]) + 4)], InvalidNodeRef);
    EXPECT_THROW((void)view[Access::Span<NodeRef<Expression>>(Access::Offset(span), 1U << 20)], InvalidNodeRef);
    EXPECT_THROW((void)view.Text(Access::Span<char>(Access::Offset(span), 1U << 20)), InvalidNodeRef);

    // A list item that points into nowhere: the access that follows it throws
    const std::uint32_t bad = 1U << 20;
    std::memcpy(Access::Bytes(tree) + Access::Offset(span), &bad, sizeof(bad));
    const auto list = view[span];
    EXPECT_THROW((void)view[list[0]], InvalidNodeRef);
    EXPECT_EQ(2, *GetIf<int64_t>(view[list[1]].GetConstant(nodes)));
}

namespace
{
// A filter that holds a handle, as `map` holds its arguments
class RefHoldingFilter : public IExpressionFilter
{
public:
    explicit RefHoldingFilter(NodeRef<Expression> ref)
        : m_ref(ref)
    {
    }

    InternalValue Filter(const InternalValue& /*baseVal*/, RenderContext& /*context*/) override { return InternalValue(); }
    void VisitRefs(jinja2::detail::RefChecker& refs) const override { refs(m_ref); }

private:
    NodeRef<Expression> m_ref;
};

// Seals a tree whose `bad` node holds a handle that is not a node of its type
template<typename Make>
void ExpectSealRejects(const Make& makeBad)
{
    NodeArena nodes;
    const auto good = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    const auto text = nodes.Make<RawTextRenderer>("text", std::size_t{ 4 });
    makeBad(nodes, good, text);
    if constexpr (NodeRefChecks >= 1)
    {
        EXPECT_THROW((void)nodes.Seal(good), InvalidNodeRef);
    }
    else
    {
        EXPECT_NO_THROW((void)nodes.Seal(good));
    }
}
} // namespace

// Seal checks every handle a node holds, unless the checks are OFF
TEST(NodeArenaTest, SealRejectsOutOfRangeRef)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> /*good*/, NodeRef<RawTextRenderer> /*text*/) {
        nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, Access::Ref<Expression>(1U << 20));
    });
}

TEST(NodeArenaTest, SealRejectsRefIntoANode)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> good, NodeRef<RawTextRenderer> /*text*/) {
        nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, Access::Ref<Expression>(Access::Offset(good) + 8));
    });
}

TEST(NodeArenaTest, SealRejectsRefOfTheWrongFamily)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> /*good*/, NodeRef<RawTextRenderer> text) {
        nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, Access::Ref<Expression>(Access::Offset(text)));
    });
}

TEST(NodeArenaTest, SealRejectsSpanOutOfRange)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> good, NodeRef<RawTextRenderer> /*text*/) {
        nodes.Make<TupleCreator>(Access::Span<NodeRef<Expression>>(Access::Offset(good), 1U << 20));
    });
}

TEST(NodeArenaTest, SealRejectsCorruptRefInAFilterObject)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> /*good*/, NodeRef<RawTextRenderer> text) {
        nodes.MakeObject<IExpressionFilter, RefHoldingFilter>(Access::Ref<Expression>(Access::Offset(text)));
    });
}

TEST(NodeArenaTest, SealRejectsTextOutOfRange)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> /*good*/, NodeRef<RawTextRenderer> /*text*/) {
        nodes.Make<ValueRefExpression>(Access::Span<char>(1U << 20, 4), std::size_t{ 0 });
    });
}

TEST(NodeArenaTest, SealRejectsTextRunningPastTheTree)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> good, NodeRef<RawTextRenderer> /*text*/) {
        nodes.Make<ValueRefExpression>(Access::Span<char>(Access::Offset(good), 1U << 20), std::size_t{ 0 });
    });
}

TEST(NodeArenaTest, SealRejectsEmptyTextWithAnOffset)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> good, NodeRef<RawTextRenderer> /*text*/) {
        nodes.Make<ValueRefExpression>(Access::Span<char>(Access::Offset(good), 0), std::size_t{ 0 });
    });
}

TEST(NodeArenaTest, SealRejectsBadRefInACallKeyword)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> good, NodeRef<RawTextRenderer> text) {
        CallParamsInfo params;
        params.kwParams["a_long_keyword_name"] = Access::Ref<Expression>(Access::Offset(text));
        nodes.Make<CallExpression>(nodes, good, params);
    });
}

TEST(NodeArenaTest, SealRejectsCompareOperandsOutOfRange)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> good, NodeRef<RawTextRenderer> /*text*/) {
        nodes.Make<CompareExpression>(good, Access::Span<CompareExpression::Operand>(Access::Offset(good), 1U << 20));
    });
}

TEST(NodeArenaTest, SealRejectsTargetsOutOfRange)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> good, NodeRef<RawTextRenderer> /*text*/) {
        nodes.Make<SetLineStatement>(Access::Span<TargetNode>(Access::Offset(good), 1U << 20), good);
    });
}

TEST(NodeArenaTest, SealRejectsBadTextInATarget)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> good, NodeRef<RawTextRenderer> /*text*/) {
        TargetNode target;
        target.name = Access::Span<char>(1U << 20, 4);
        nodes.Make<SetLineStatement>(nodes.MakeSpan(std::array<TargetNode, 1>{ target }), good);
    });
}

TEST(NodeArenaTest, SealRejectsSlotNameTextOutOfRange)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> /*good*/, NodeRef<RawTextRenderer> /*text*/) {
        const auto macro = nodes.Make<MacroStatement>("m", MacroParams());
        const std::array<SlotName, 1> names{ SlotName{ Access::Span<char>(1U << 20, 3), 0 } };
        nodes[macro].BindSlots(nodes.MakeSpan(names));
    });
}

// A tree with good handles only seals, a filter's included
TEST(NodeArenaTest, SealAcceptsGoodRefs)
{
    NodeArena nodes;
    const NodeRef<Expression> good = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    const auto filter = nodes.MakeObject<IExpressionFilter, RefHoldingFilter>(good);
    const auto unary = nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, good);
    EXPECT_NO_THROW((void)nodes.Seal(filter, unary));
}

// With FULL checks a node Seal did not reach cannot be read: a handle that a node's
// VisitRefs forgets fails the first render that follows it
TEST(NodeArenaTest, FullRejectsAnUnvalidatedNode)
{
    if constexpr (NodeRefChecks < 2)
    {
        GTEST_SKIP() << "node reference checks below FULL";
    }
    NodeArena nodes;
    const NodeRef<Expression> reached = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    const NodeRef<Expression> forgotten = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 2 }));
    const SealedArena tree = nodes.Seal(reached);
    EXPECT_EQ(1, *GetIf<int64_t>(tree[reached].GetConstant(nodes)));
    EXPECT_THROW((void)tree[forgotten], InvalidNodeRef);
}
#endif // JINJA2CPP_LINK_AS_SHARED

namespace
{
// A node of kind T::Kind is a U exactly when T derives from U: catches a subclass that
// inherits its base's Kind or MatchesKind instead of declaring its own
template<typename T, typename... Us>
int CountKindMismatches()
{
    return ((ArenaView::KindIs<Us>(T::Kind) != std::is_base_of_v<Us, T> ? 1 : 0) + ... + 0);
}

template<typename... Ts>
int CountAllKindMismatches()
{
    return (CountKindMismatches<Ts, Ts...>() + ... + 0);
}
} // namespace

TEST(NodeArenaTest, EveryNodeClassHasItsOwnKind)
{
    const int mismatches = CountAllKindMismatches<
        FullExpressionEvaluator, ValueRefExpression, SelfRefExpression, SubscriptExpression, LoopAttrExpression, FilteredExpression, ConstantExpression,
        ScalarConstantExpression, TupleCreator, DictCreator, UnaryExpression, IsExpression, BinaryExpression, InLiteralExpression, ConstFormatExpression,
        CompareExpression, SliceExpression, CallExpression,
        ExpressionFilter, IfExpression, ComposedRenderer, RawTextRenderer, ExpressionRenderer, FinalizedExpressionRenderer, TemplateRenderer,
        ForStatement, IfStatement, ElseBranchStatement, SetLineStatement, SetRawBlockStatement, SetFilteredBlockStatement, BlockStatement,
        ExtendsStatement, IncludeStatement, ImportStatement, MacroStatement, MacroCallStatement, DoStatement, TransStatement,
        LoopControlStatement, WithStatement, FilterStatement, AutoescapeStatement, ExpressionFilter::IExpressionFilter, IsExpression::ITester>();
    EXPECT_EQ(0, mismatches);
}
