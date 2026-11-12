// Renders whose sink suspends them in Write and resumes them later (RenderOptions::suspension,
// docs/tasks/0457): each runs on a fiber of the Python module's switch (python/src/fiber), and
// interleaves with the others and with plain renders on the thread
#include "gtest/gtest.h"

#include <fiber.h>

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace jinja2;
using jinja2cpp_fiber::Fiber;

namespace
{
struct Cancelled : std::runtime_error
{
    Cancelled()
        : std::runtime_error("cancelled")
    {
    }
};

// A render on a fiber that suspends at each piece of output: Next() gives the pieces one by one, as
// the Python module's generate() does
template<typename CharT>
class LazyRender
{
public:
    using String = std::basic_string<CharT>;

    LazyRender(const BasicTemplate<CharT>& tpl, ValuesMap params, RenderOptions options = {}, std::size_t stackSize = Fiber::DefaultStackSize)
        : m_tpl(tpl)
        , m_params(std::move(params))
        , m_options(std::move(options))
        , m_sink(*this)
        , m_fiber([this] { Run(); }, stackSize)
    {
    }
    ~LazyRender()
    {
        if (m_fiber.Started() && !m_fiber.Finished())
        {
            Cancel();
        }
    }
    LazyRender(const LazyRender&) = delete;
    LazyRender(LazyRender&&) = delete;
    LazyRender& operator=(const LazyRender&) = delete;
    LazyRender& operator=(LazyRender&&) = delete;

    // The next piece, or nothing once the render ended
    std::optional<String> Next()
    {
        if (m_fiber.Finished())
        {
            return std::nullopt;
        }
        m_piece.reset();
        m_fiber.Resume();
        return std::move(m_piece);
    }
    // Every piece left, concatenated
    String Rest()
    {
        String text;
        while (auto piece = Next())
        {
            text += *piece;
        }
        return text;
    }
    // Ends the render at the Write it is suspended in, as the module's close() does
    void Cancel()
    {
        m_cancelled = true;
        if (!m_fiber.Finished())
        {
            m_fiber.Resume();
        }
    }
    [[nodiscard]] bool Done() const { return m_fiber.Finished(); }
    [[nodiscard]] const std::optional<Result<void, CharT>>& Outcome() const { return m_result; }
    [[nodiscard]] std::size_t Writes() const { return m_writes; }

private:
    class Sink final : public BasicOutputSink<CharT>
    {
    public:
        explicit Sink(LazyRender& owner)
            : m_owner(owner)
        {
        }
        void Write(std::basic_string_view<CharT> text) override
        {
            if (m_owner.m_cancelled)
            {
                throw Cancelled();
            }
            ++m_owner.m_writes;
            m_owner.m_piece = String(text);
            m_owner.m_fiber.Suspend();
            if (m_owner.m_cancelled)
            {
                throw Cancelled();
            }
        }

    private:
        LazyRender& m_owner;
    };

    void Run()
    {
        m_suspension.stackLow = m_fiber.StackLow();
        m_suspension.stackSize = m_fiber.StackSize();
        m_options.suspension = &m_suspension;
        m_options.writeEachPiece = true;
        m_result = m_tpl.RenderTo(m_sink, m_params, m_options);
    }

    const BasicTemplate<CharT>& m_tpl;
    ValuesMap m_params;
    RenderOptions m_options;
    RenderSuspension m_suspension;
    Sink m_sink;
    std::optional<String> m_piece;
    std::optional<Result<void, CharT>> m_result;
    std::size_t m_writes = 0;
    bool m_cancelled = false;
    Fiber m_fiber;
};

template<typename CharT>
class PieceSink final : public BasicOutputSink<CharT>
{
public:
    void Write(std::basic_string_view<CharT> text) override { pieces.emplace_back(text); }
    std::vector<std::basic_string<CharT>> pieces;
};

std::string ErrorText(const std::optional<Result<void, char>>& outcome)
{
    if (!outcome)
    {
        return "not finished";
    }
    return *outcome ? std::string() : outcome->error().ToString();
}

// The code of a render that failed, or nothing
std::optional<ErrorCode> FailureCode(const std::optional<Result<void, char>>& outcome)
{
    if (!outcome || *outcome)
    {
        return std::nullopt;
    }
    return outcome->error().GetCode();
}

struct TemplateCase
{
    const char* name;
    const char* source;
};

// One template for each statement that holds state across its body's writes
const TemplateCase Templates[] = {
    { "LoopWithSet", "{% for i in range(3) %}{% set x = i * 2 %}<{{ x }}>{% endfor %}{{ 'end' }}" },
    { "Include", "a{% include 'inc.j2' %}b{% include 'inc.j2' %}" },
    { "Extends", "{% extends 'base.j2' %}{% block b %}B{{ 1 }}|{{ super() }}{% endblock %}" },
    { "Macro", "{% macro m(a) %}[{{ a }}]{% endmacro %}{% for i in range(3) %}{{ m(i) }}-{% endfor %}" },
    { "CallBlock", "{% macro c(n) %}[{{ caller(n) }}|{{ n }}]{% endmacro %}{% for i in range(2) %}{% call(k) c(i) %}{{ k * 10 }}{% endcall %}{% endfor %}" },
    { "SetAndFilterBlocks", "{% set s %}a{{ 1 }}b{% endset %}{{ s }}|{% filter upper %}x{{ 'y' }}{% endfilter %}{{ s }}" },
    { "RecursiveLoop", "{% for n in tree recursive %}({{ n.v }}{{ loop(n.c) }}){% endfor %}" },
    { "Namespace", "{% set ns = namespace(n=0, l=[]) %}{% for i in range(4) %}{% set ns.n = ns.n + i %}{{ ns.n }}{% endfor %}{{ ns.l }}" },
    { "ImportedMacro", "{% import 'lib.j2' as lib %}{% for i in range(2) %}{{ lib.twice(i) }}{% endfor %}{% from 'lib.j2' import twice %}{{ twice(9) }}" },
};

ValuesMap Params()
{
    const ValuesMap leaf1 = { { "v", 2 }, { "c", ValuesList{} } };
    const ValuesMap leaf2 = { { "v", 3 }, { "c", ValuesList{} } };
    const ValuesMap root = { { "v", 1 }, { "c", ValuesList{ leaf1, leaf2 } } };
    return { { "tree", ValuesList{ root, ValuesMap{ { "v", 4 }, { "c", ValuesList{} } } } } };
}

class SuspendableRenderTest : public ::testing::TestWithParam<TemplateCase>
{
protected:
    void SetUp() override
    {
        if (!Fiber::Available())
        {
            GTEST_SKIP() << "no fibers on this platform";
        }
        auto fs = std::make_shared<MemoryFileSystem>();
        fs->AddFile("inc.j2", "[{% for i in range(2) %}{{ i }}{% endfor %}]");
        fs->AddFile("base.j2", "<{% block b %}base{{ 2 }}{% endblock %}>{% block c %}C{{ 3 }}{% endblock %}");
        fs->AddFile("lib.j2", "{% macro twice(x) %}{{ x }}{{ x }}{% endmacro %}");
        m_env.AddFilesystemHandler({}, fs);
        ASSERT_TRUE(m_tpl.Load(GetParam().source)) << GetParam().name;
        auto eager = m_tpl.RenderAsString(Params());
        ASSERT_TRUE(eager) << eager.error().ToString();
        m_expected = eager.value();
        RenderOptions options;
        options.writeEachPiece = true;
        ASSERT_TRUE(m_tpl.RenderTo(m_pieces, Params(), options));
    }

    TemplateEnv m_env;
    Template m_tpl{ &m_env };
    std::string m_expected;
    PieceSink<char> m_pieces;
};

TEST_P(SuspendableRenderTest, Sequential)
{
    LazyRender<char> lazy(m_tpl, Params());
    std::vector<std::string> pieces;
    while (auto piece = lazy.Next())
    {
        pieces.push_back(*piece);
    }
    EXPECT_EQ(ErrorText(lazy.Outcome()), "");
    EXPECT_EQ(pieces, m_pieces.pieces);
    // The thread holds nothing of it afterwards
    EXPECT_TRUE(ReleaseThreadCaches());
}

TEST_P(SuspendableRenderTest, Interleaved)
{
    // A starts, B starts, A finishes, B finishes, and a third one steps along with both: each render's slot
    // frames and maps stay its own (the RenderWorkspace::Release order of docs/tasks/0457)
    LazyRender<char> a(m_tpl, Params());
    LazyRender<char> b(m_tpl, Params());
    LazyRender<char> c(m_tpl, Params());
    std::string textA;
    std::string textB;
    std::string textC;
    for (int step = 0; !a.Done() || !b.Done() || !c.Done(); ++step)
    {
        if (auto piece = a.Next())
        {
            textA += *piece;
        }
        if (step % 2 == 1)
        {
            if (auto piece = b.Next())
            {
                textB += *piece;
            }
        }
        if (auto piece = c.Next())
        {
            textC += *piece;
        }
        if (auto piece = c.Next())
        {
            textC += *piece;
        }
    }
    EXPECT_EQ(ErrorText(a.Outcome()), "");
    EXPECT_EQ(ErrorText(b.Outcome()), "");
    EXPECT_EQ(ErrorText(c.Outcome()), "");
    EXPECT_EQ(textA, m_expected);
    EXPECT_EQ(textB, m_expected);
    EXPECT_EQ(textC, m_expected);
    EXPECT_TRUE(ReleaseThreadCaches());
}

TEST_P(SuspendableRenderTest, ResumedFromCallableOfPlainRender)
{
    LazyRender<char> lazy(m_tpl, Params());
    Template driver(&m_env);
    ASSERT_TRUE(driver.Load("{% for i in range(3) %}({{ step() }}){% endfor %}"));
    std::string fromDriver;
    const ValuesMap driverParams = { { "step", MakeCallable([&lazy, &fromDriver] {
                                           auto piece = lazy.Next().value_or(std::string());
                                           fromDriver += piece;
                                           return piece;
                                       }) } };
    auto driven = driver.RenderAsString(driverParams);
    ASSERT_TRUE(driven) << driven.error().ToString();
    const std::string rest = lazy.Rest();
    EXPECT_EQ(ErrorText(lazy.Outcome()), "");
    EXPECT_EQ(fromDriver + rest, m_expected);
}

TEST_P(SuspendableRenderTest, StartedInsideCallableOfAnother)
{
    // The inner render starts in a callable of the outer one, gives one piece there, and finishes after the
    // outer one ended: it took nothing of the outer render's state
    std::optional<LazyRender<char>> inner;
    std::string innerText;
    Template outerTpl(&m_env);
    ASSERT_TRUE(outerTpl.Load("o{% for i in range(2) %}<{{ start() }}>{% endfor %}O"));
    const ValuesMap outerParams = { { "start", MakeCallable([&] {
                                          if (!inner)
                                          {
                                              inner.emplace(m_tpl, Params());
                                          }
                                          auto piece = inner->Next().value_or(std::string());
                                          innerText += piece;
                                          return piece;
                                      }) } };
    LazyRender<char> outer(outerTpl, outerParams);
    std::string outerText;
    std::size_t outerPieces = 0;
    while (auto piece = outer.Next())
    {
        outerText += *piece;
        ++outerPieces;
        // Between the outer render's pieces the inner one moves on too, from here
        if (inner && outerPieces % 2 == 0)
        {
            if (auto innerPiece = inner->Next())
            {
                innerText += *innerPiece;
            }
        }
    }
    EXPECT_EQ(ErrorText(outer.Outcome()), "");
    ASSERT_TRUE(inner);
    if (inner)
    {
        innerText += inner->Rest();
        EXPECT_EQ(ErrorText(inner->Outcome()), "");
    }
    EXPECT_EQ(innerText, m_expected);
}

TEST_P(SuspendableRenderTest, PlainRenderAndCacheReleaseWhileSuspended)
{
    LazyRender<char> lazy(m_tpl, Params());
    std::string text = lazy.Next().value_or(std::string());
    // The thread is free: a plain render of the same template runs, and the thread's caches can go
    auto plain = m_tpl.RenderAsString(Params());
    ASSERT_TRUE(plain) << plain.error().ToString();
    EXPECT_EQ(plain.value(), m_expected);
    EXPECT_TRUE(ReleaseThreadCaches());
    text += lazy.Next().value_or(std::string());
    EXPECT_TRUE(ReleaseThreadCaches());
    text += lazy.Rest();
    EXPECT_EQ(ErrorText(lazy.Outcome()), "");
    EXPECT_EQ(text, m_expected);
}

TEST_P(SuspendableRenderTest, CancelledAtEachWrite)
{
    for (std::size_t at = 1; at <= m_pieces.pieces.size(); ++at)
    {
        {
            LazyRender<char> lazy(m_tpl, Params());
            for (std::size_t idx = 0; idx != at; ++idx)
            {
                ASSERT_TRUE(lazy.Next());
            }
            lazy.Cancel();
            ASSERT_TRUE(lazy.Done());
            ASSERT_TRUE(lazy.Outcome() && !*lazy.Outcome()) << at;
        }
        // The render unwound on its fiber: the thread is clean, and the next render matches
        EXPECT_TRUE(ReleaseThreadCaches()) << at;
        EXPECT_EQ(m_tpl.RenderAsString(Params()).value(), m_expected) << at;
    }
}

INSTANTIATE_TEST_SUITE_P(Statements, SuspendableRenderTest, ::testing::ValuesIn(Templates), [](const auto& param) { return std::string(param.param.name); });

class SuspendableRenderLimitsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (!Fiber::Available())
        {
            GTEST_SKIP() << "no fibers on this platform";
        }
    }
};

TEST_F(SuspendableRenderLimitsTest, DeepRecursionOnSmallStackIsRecursionError)
{
    // No depth count to stop it first: only the stack check, against the fiber's bounds, does
    TemplateEnv env;
    env.GetSettings().limits.maxRenderDepth = 0;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% macro r(n) %}{{ r(n + 1) }}{% endmacro %}{{ r(0) }}"));
    for (const std::size_t size : { std::size_t{ 256 } << 10, std::size_t{ 1 } << 20, std::size_t{ 4 } << 20 })
    {
        LazyRender<char> lazy(tpl, {}, {}, size);
        EXPECT_FALSE(lazy.Next()) << size;
        ASSERT_TRUE(lazy.Outcome() && !*lazy.Outcome()) << size;
        EXPECT_EQ(FailureCode(lazy.Outcome()), ErrorCode::RecursionLimitExceeded) << size;
    }
    // A stack below the engine's reserve fails at the first check
    LazyRender<char> tiny(tpl, {}, {}, std::size_t{ 64 } << 10);
    EXPECT_FALSE(tiny.Next());
    ASSERT_TRUE(tiny.Outcome() && !*tiny.Outcome());
    EXPECT_EQ(FailureCode(tiny.Outcome()), ErrorCode::RecursionLimitExceeded);
}

TEST_F(SuspendableRenderLimitsTest, LimitsCountPerRenderAcrossSuspensions)
{
    TemplateEnv env;
    Template probe(&env);
    ASSERT_TRUE(probe.Load("{% for i in range(n) %}{{ i % 10 }}{% endfor %}"));
    // The smallest step budget that renders 300 items
    auto fits = [&](std::uint64_t steps, std::int64_t items) {
        env.GetSettings().limits.maxRenderSteps = steps;
        Template tpl(&env);
        EXPECT_TRUE(tpl.Load("{% for i in range(n) %}{{ i % 10 }}{% endfor %}"));
        return static_cast<bool>(tpl.RenderAsString({ { "n", items } }));
    };
    std::uint64_t budget = 1;
    while (!fits(budget, 300))
    {
        budget *= 2;
    }
    ASSERT_FALSE(fits(budget, 600));
    env.GetSettings().limits.maxRenderSteps = budget;
    env.GetSettings().limits.maxOutputSize = 400;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% for i in range(n) %}{{ i % 10 }}{% endfor %}"));

    // Two renders interleaved each count their own steps and output
    LazyRender<char> a(tpl, { { "n", 300 } });
    LazyRender<char> b(tpl, { { "n", 300 } });
    std::string textA;
    std::string textB;
    while (!a.Done() || !b.Done())
    {
        textA += a.Next().value_or(std::string());
        textB += b.Next().value_or(std::string());
    }
    EXPECT_EQ(ErrorText(a.Outcome()), "");
    EXPECT_EQ(ErrorText(b.Outcome()), "");
    EXPECT_EQ(textA.size(), 300U);
    EXPECT_EQ(textB.size(), 300U);

    // One render past either limit fails, suspended or not
    env.GetSettings().limits.maxOutputSize = 0;
    Template steps(&env);
    ASSERT_TRUE(steps.Load("{% for i in range(n) %}{{ i % 10 }}{% endfor %}"));
    LazyRender<char> tooLong(steps, { { "n", 600 } });
    tooLong.Rest();
    ASSERT_TRUE(tooLong.Outcome() && !*tooLong.Outcome());
    EXPECT_EQ(FailureCode(tooLong.Outcome()), ErrorCode::ResourceLimitExceeded);

    env.GetSettings().limits.maxRenderSteps = 0;
    env.GetSettings().limits.maxOutputSize = 400;
    Template output(&env);
    ASSERT_TRUE(output.Load("{% for i in range(n) %}{{ i % 10 }}{% endfor %}"));
    LazyRender<char> tooMuch(output, { { "n", 600 } });
    tooMuch.Rest();
    ASSERT_TRUE(tooMuch.Outcome() && !*tooMuch.Outcome());
    EXPECT_EQ(FailureCode(tooMuch.Outcome()), ErrorCode::ResourceLimitExceeded);
    EXPECT_TRUE(ReleaseThreadCaches());
}

TEST_F(SuspendableRenderLimitsTest, PollRunsForItsRenderOnlyAndMayStopIt)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% for i in range(5000) %}{{ i % 10 }}{% endfor %}"));
    int pollsA = 0;
    RenderOptions withPoll;
    withPoll.poll = [&pollsA] {
        if (++pollsA == 3)
        {
            throw Cancelled();
        }
    };
    LazyRender<char> a(tpl, {}, withPoll);
    LazyRender<char> b(tpl, {});
    std::string textB;
    while (!a.Done() || !b.Done())
    {
        a.Next();
        textB += b.Next().value_or(std::string());
    }
    ASSERT_TRUE(a.Outcome() && !*a.Outcome());
    EXPECT_EQ(pollsA, 3);
    EXPECT_LT(a.Writes(), 5000U);
    // B, without a poll of its own, was never polled with A's, and rendered everything
    EXPECT_EQ(ErrorText(b.Outcome()), "");
    EXPECT_EQ(textB.size(), 5000U);
    EXPECT_TRUE(ReleaseThreadCaches());
}

TEST_F(SuspendableRenderLimitsTest, PollOfSuspendedRenderIsNotCalledByOthers)
{
    // B counts steps (limits of its own) while A, with a poll, is suspended: A's poll is not B's
    TemplateEnv envA;
    Template a;
    ASSERT_TRUE(a.Load("{% for i in range(5000) %}{{ i % 10 }}{% endfor %}"));
    int pollsA = 0;
    RenderOptions withPoll;
    withPoll.poll = [&pollsA] { ++pollsA; };
    LazyRender<char> la(a, {}, withPoll);
    ASSERT_TRUE(la.Next());
    const int before = pollsA;
    TemplateEnv envB;
    envB.GetSettings().limits.maxRenderSteps = 1000000000;
    Template b(&envB);
    ASSERT_TRUE(b.Load("{% for i in range(5000) %}{{ i % 10 }}{% endfor %}"));
    LazyRender<char> lb(b, {});
    lb.Rest();
    EXPECT_EQ(ErrorText(lb.Outcome()), "");
    auto plain = b.RenderAsString({});
    EXPECT_TRUE(plain);
    EXPECT_EQ(pollsA, before);
}

TEST_F(SuspendableRenderLimitsTest, RenderDepthIsPerRender)
{
    // Three renders suspended at the deepest include each count their own depth against the limit
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("i1.j2", "a{% include \"i2.j2\" %}b");
    fs->AddFile("i2.j2", "c{% include \"i3.j2\" %}d");
    fs->AddFile("i3.j2", "e{% include \"i4.j2\" %}f");
    fs->AddFile("i4.j2", "g{{ 1 }}h");
    unsigned minDepth = 0;
    for (unsigned n = 1; n < 40 && !minDepth; ++n)
    {
        TemplateEnv env;
        env.AddFilesystemHandler({}, fs);
        env.GetSettings().limits.maxRenderDepth = n;
        Template t(&env);
        if (!t.Load("{% include \"i1.j2\" %}"))
        {
            continue;
        }
        if (t.RenderAsString({}))
        {
            minDepth = n;
        }
    }
    ASSERT_NE(minDepth, 0U);
    TemplateEnv env;
    env.AddFilesystemHandler({}, fs);
    env.GetSettings().limits.maxRenderDepth = minDepth;
    Template t(&env);
    ASSERT_TRUE(t.Load("{% include \"i1.j2\" %}"));
    LazyRender<char> x(t, {});
    LazyRender<char> y(t, {});
    LazyRender<char> z(t, {});
    ASSERT_TRUE(x.Next());
    ASSERT_TRUE(y.Next());
    ASSERT_TRUE(z.Next());
    x.Rest();
    y.Rest();
    z.Rest();
    EXPECT_EQ(ErrorText(x.Outcome()), "");
    EXPECT_EQ(ErrorText(y.Outcome()), "");
    EXPECT_EQ(ErrorText(z.Outcome()), "");
}

TEST_F(SuspendableRenderLimitsTest, SuspendableStartedDeepInsidePlainRender)
{
    TemplateEnv env;
    env.GetSettings().limits.maxRenderDepth = 0;
    Template p(&env);
    ASSERT_TRUE(p.Load("{% macro r(n) %}{% set a = n %}{% set b = n %}{% if n > 0 %}{{ r(n - 1) }}{% else %}{{ start() }}{% endif %}{% endmacro %}{{ r(150) }}"));
    Template s;
    ASSERT_TRUE(s.Load("{% macro m(a) %}{% set q = a %}[{{ q }}]{% endmacro %}{% for i in range(6) %}{% set x = i %}{{ m(x) }}{% endfor %}"));
    const std::string expected = s.RenderAsString({}).value();
    std::optional<LazyRender<char>> inner;
    std::string innerText;
    const ValuesMap params = { { "start", MakeCallable([&] {
                                     if (!inner)
                                     {
                                         inner.emplace(s, ValuesMap{});
                                     }
                                     auto piece = inner->Next().value_or(std::string());
                                     innerText += piece;
                                     return piece;
                                 }) } };
    // The plain render holds more than a chunk of slots when the suspendable one starts (RenderWorkspace's
    // current chunk is swapped too)
    auto r = p.RenderAsString(params);
    ASSERT_TRUE(r) << r.error().ToString();
    ASSERT_TRUE(inner);
    if (inner)
    {
        innerText += inner->Rest();
        EXPECT_EQ(ErrorText(inner->Outcome()), "");
    }
    EXPECT_EQ(innerText, expected);
}

TEST_F(SuspendableRenderLimitsTest, WriteRunsAsIfNoRenderRan)
{
    // Inside Write the thread is free: its caches can be released, and a plain render there has its own limits
    Template inner;
    ASSERT_TRUE(inner.Load("{% for i in range(3) %}{{ i }}{% endfor %}"));
    class Checking final : public BasicOutputSink<char>
    {
    public:
        explicit Checking(const Template& tpl)
            : m_tpl(tpl)
        {
        }
        void Write(std::string_view text) override
        {
            released = released && ReleaseThreadCaches();
            nested += m_tpl.RenderAsString({}).value();
            output += text;
        }
        bool released = true;
        std::string nested;
        std::string output;

    private:
        const Template& m_tpl;
    } sink(inner);
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% for i in range(3) %}{% set x = i %}{{ x }}{% endfor %}"));
    RenderSuspension onThreadStack;
    RenderOptions options;
    options.suspension = &onThreadStack;
    options.writeEachPiece = true;
    ASSERT_TRUE(tpl.RenderTo(sink, {}, options));
    EXPECT_TRUE(sink.released);
    EXPECT_EQ(sink.output, "012");
    EXPECT_EQ(sink.nested, "012012012");
}

TEST_F(SuspendableRenderLimitsTest, LoadOfTheSameTemplateWhileSuspended)
{
    // The template a Load replaces stays alive for the suspended render that runs it
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% for i in range(3) %}<{{ i }}>{% endfor %}"));
    LazyRender<char> lazy(tpl, {});
    std::string text = lazy.Next().value_or(std::string());
    ASSERT_TRUE(tpl.Load("other"));
    EXPECT_EQ(tpl.RenderAsString({}).value(), "other");
    text += lazy.Rest();
    EXPECT_EQ(ErrorText(lazy.Outcome()), "");
    EXPECT_EQ(text, "<0><1><2>");
}

TEST_F(SuspendableRenderLimitsTest, WideTemplatesInterleave)
{
    TemplateW tpl;
    ASSERT_TRUE(tpl.Load(L"{% macro m(a) %}[{{ a }}]{% endmacro %}{% for i in range(4) %}{% set y = i %}{{ m(y) }}{% endfor %}"));
    const std::wstring expected = tpl.RenderAsString({}).value();
    LazyRender<wchar_t> a(tpl, {});
    LazyRender<wchar_t> b(tpl, {});
    std::wstring textA;
    std::wstring textB;
    while (!a.Done() || !b.Done())
    {
        textA += a.Next().value_or(std::wstring());
        textB += b.Next().value_or(std::wstring());
        textB += b.Next().value_or(std::wstring());
    }
    EXPECT_EQ(textA, expected);
    EXPECT_EQ(textB, expected);
}

TEST(FiberTest, ResumeSuspendAndErrors)
{
    if (!Fiber::Available())
    {
        GTEST_SKIP() << "no fibers on this platform";
    }
    std::vector<int> trace;
    Fiber fiber([&] {
        trace.push_back(1);
        // An exception thrown and caught on the fiber, then a suspension
        try
        {
            throw std::runtime_error("inside");
        }
        catch (const std::runtime_error&)
        {
            trace.push_back(2);
        }
    });
    EXPECT_FALSE(fiber.Started());
    fiber.Resume();
    EXPECT_TRUE(fiber.Finished());
    EXPECT_EQ(trace, (std::vector<int>{ 1, 2 }));
    EXPECT_THROW(fiber.Resume(), std::logic_error);

    std::optional<Fiber> thrower;
    thrower.emplace([] { throw std::runtime_error("out"); });
    EXPECT_THROW(thrower->Resume(), std::runtime_error);

    // Nested: a fiber resumed from another one goes back to it
    std::string order;
    std::optional<Fiber> inner;
    inner.emplace([&] {
        order += "i1";
        inner->Suspend();
        order += "i2";
    });
    Fiber outer([&] {
        order += "o1";
        inner->Resume();
        order += "o2";
    });
    outer.Resume();
    EXPECT_EQ(order, "o1i1o2");
    inner->Resume();
    EXPECT_EQ(order, "o1i1o2i2");
    EXPECT_TRUE(inner->Finished());
    // An unfinished fiber is freed without running more
    std::optional<Fiber> abandoned;
    bool after = false;
    abandoned.emplace([&] {
        abandoned->Suspend();
        after = true;
    });
    abandoned->Resume();
    abandoned.reset();
    EXPECT_FALSE(after);
}
} // namespace
