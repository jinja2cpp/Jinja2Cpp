#include "gtest/gtest.h"
#include "test_tools.h"

#include <jinja2cpp/version.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <istream>
#include <memory>
#include <optional>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace jinja2;

static_assert(std::is_same_v<Template, BasicTemplate<char>>);
static_assert(std::is_same_v<TemplateW, BasicTemplate<wchar_t>>);
static_assert(std::is_same_v<ResultW<int>, Result<int, wchar_t>>);
static_assert(std::is_same_v<Result<int>, Result<int, char>>);
static_assert(std::is_same_v<decltype(std::declval<const Template&>().RenderAsString(ValuesMap{})), Result<std::string>>);
static_assert(std::is_same_v<decltype(std::declval<const TemplateW&>().RenderAsString(ValuesMap{})), ResultW<std::wstring>>);
static_assert(std::is_same_v<decltype(std::declval<const Template&>().RenderTo(std::declval<std::string&>(), ValuesMap{})), Result<void>>);
static_assert(std::is_same_v<decltype(std::declval<const TemplateW&>().RenderTo(std::declval<std::wstring&>(), ValuesMap{})), ResultW<void>>);
static_assert(std::is_same_v<decltype(std::declval<const Template&>().RenderTo(std::declval<OutputSink&>(), ValuesMap{})), Result<void>>);
static_assert(std::is_same_v<decltype(std::declval<const TemplateW&>().RenderTo(std::declval<OutputSinkW&>(), ValuesMap{})), ResultW<void>>);
static_assert(std::is_same_v<decltype(std::declval<const Template&>().RenderTo(std::declval<std::string&>(), std::declval<const GenericMap&>())), Result<void>>);
static_assert(std::is_same_v<decltype(std::declval<const Template&>().RenderTo(std::declval<OutputSink&>(), std::declval<const GenericMap&>())), Result<void>>);
static_assert(std::is_same_v<decltype(ReleaseThreadCaches()), bool>);
static_assert(noexcept(ReleaseThreadCaches()));
// A sink is never deleted through the base, nor made on its own
static_assert(!std::is_destructible_v<OutputSink>);
static_assert(std::is_abstract_v<OutputSink>);
static_assert(std::is_same_v<ContainerSink<std::wstring>, ContainerSink<std::wstring, wchar_t>>);

namespace
{
// RenderTo(std::string{}, ...) must not compile: the output would be lost
template<typename Out, typename = void>
struct CanRenderTo : std::false_type
{
};
template<typename Out>
struct CanRenderTo<Out, std::void_t<decltype(std::declval<const Template&>().RenderTo(std::declval<Out>(), ValuesMap{}))>> : std::true_type
{
};
} // namespace
static_assert(CanRenderTo<std::string&>::value);
static_assert(!CanRenderTo<std::string&&>::value);
static_assert(!CanRenderTo<const std::string&>::value);
static_assert(!CanRenderTo<std::wstring&>::value);
static_assert(CanRenderTo<ContainerSink<std::string>&>::value);

TEST(TemplateApiTest, LoadAcceptsEveryStringForm)
{
    const char* literal = "{{ x }}";
    std::string str = "{{ x }}";
    std::string_view view = str;

    Template fromLiteral;
    ASSERT_TRUE(!!fromLiteral.Load(literal));
    EXPECT_EQ("1", fromLiteral.RenderAsString({ { "x", 1 } }).value());

    Template fromString;
    ASSERT_TRUE(!!fromString.Load(str, "named.j2tpl"));
    EXPECT_EQ("2", fromString.RenderAsString({ { "x", 2 } }).value());

    // The view need not be null-terminated
    Template fromView;
    ASSERT_TRUE(!!fromView.Load(view.substr(0, 7)));
    EXPECT_EQ("3", fromView.RenderAsString({ { "x", 3 } }).value());

    TemplateW wide;
    ASSERT_TRUE(!!wide.Load(std::wstring_view(L"{{ x }}!!", 7)));
    EXPECT_EQ(L"4", wide.RenderAsString({ { "x", 4 } }).value());

    std::istringstream stream("{{ x }}");
    Template fromStream;
    ASSERT_TRUE(!!fromStream.Load(stream));
    EXPECT_EQ("5", fromStream.RenderAsString({ { "x", 5 } }).value());
}

TEST(TemplateApiTest, RenderIsConst)
{
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{{ x }}"));
    const Template& ctpl = tpl;

    std::ostringstream os;
    ASSERT_TRUE(!!ctpl.Render(os, { { "x", "a" } }));
    EXPECT_EQ("a", os.str());
    EXPECT_EQ("b", ctpl.RenderAsString({ { "x", "b" } }).value());
    EXPECT_EQ("", ctpl.RenderAsString({}).value());
    std::string out;
    ASSERT_TRUE(!!ctpl.RenderTo(out, { { "x", "c" } }));
    ASSERT_TRUE(!!ctpl.RenderTo(out, {}));
    ContainerSink sink(out);
    ASSERT_TRUE(!!ctpl.RenderTo(sink, { { "x", "d" } }));
    EXPECT_EQ("cd", out);
    EXPECT_TRUE(!!ctpl.GetMetadata());
    EXPECT_TRUE(!!ctpl.GetMetadataRaw());
}

TEST(TemplateApiTest, RenderWithGenericMapContext)
{
    TestStruct data;
    data.intValue = 10;
    data.strValue = "hello";
    auto reflected = Reflect(data);
    const auto& context = reflected.As<GenericMap>();

    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{{ intValue }} {{ strValue }} {{ missing is undefined }}"));
    EXPECT_EQ("10 hello True", tpl.RenderAsString(context).value());

    std::ostringstream os;
    ASSERT_TRUE(!!tpl.Render(os, context));
    EXPECT_EQ("10 hello True", os.str());

    std::string out = "> ";
    ASSERT_TRUE(!!tpl.RenderTo(out, context));
    EXPECT_EQ("> 10 hello True", out);
    std::string streamed;
    ContainerSink sink(streamed);
    ASSERT_TRUE(!!tpl.RenderTo(sink, context));
    EXPECT_EQ("10 hello True", streamed);

    TemplateW wtpl;
    ASSERT_TRUE(!!wtpl.Load(L"{{ intValue }} {{ strValue }}"));
    EXPECT_EQ(L"10 hello", wtpl.RenderAsString(context).value());
    std::wstring wout;
    ASSERT_TRUE(!!wtpl.RenderTo(wout, context));
    EXPECT_EQ(L"10 hello", wout);
    ContainerSink wsink(wout);
    ASSERT_TRUE(!!wtpl.RenderTo(wsink, context));
    EXPECT_EQ(L"10 hello10 hello", wout);

    // A failed render truncates through the GenericMap overload too
    Template failing;
    ASSERT_TRUE(!!failing.Load("{{ intValue }}{{ intValue() }}"));
    out = "kept";
    EXPECT_FALSE(failing.RenderTo(out, context));
    EXPECT_EQ("kept", out);
}

TEST(TemplateApiTest, GenericMapContextSeesEnvironmentGlobals)
{
    TemplateEnv env;
    env.AddGlobal("greeting", "hi");
    env.AddGlobal("intValue", 1);

    TestStruct data;
    data.intValue = 7;
    auto reflected = Reflect(data);

    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{{ greeting }} {{ intValue }}"));
    EXPECT_EQ("hi 7", tpl.RenderAsString(reflected.As<GenericMap>()).value());
}

// The globals are converted once per change and kept between renders (docs/tasks/0139)
TEST(TemplateApiTest, GlobalsChangedBetweenRenders)
{
    TemplateEnv env;
    env.AddGlobal("g", "a");
    TemplateEnv other;
    other.AddGlobal("g", "other");

    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{{ g }}"));
    Template otherTpl(&other);
    ASSERT_TRUE(!!otherTpl.Load("{{ g }}"));
    EXPECT_EQ("a", tpl.RenderAsString({}).value());
    EXPECT_EQ("other", otherTpl.RenderAsString({}).value());
    EXPECT_EQ("a", tpl.RenderAsString({}).value());
    env.AddGlobal("g", "b");
    EXPECT_EQ("b", tpl.RenderAsString({}).value());
    env.RemoveGlobal("g");
    EXPECT_EQ("", tpl.RenderAsString({}).value());
    EXPECT_EQ("other", otherTpl.RenderAsString({}).value());
}

// A global the template changes in place is changed for that render only
TEST(TemplateApiTest, GlobalChangedInPlaceForOneRender)
{
    TemplateEnv env;
    env.AddGlobal("l", ValuesList{ 1 });
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{{ l.append(2) }}{{ l }}|{% include 'x' ignore missing %}{{ l }}"));
    EXPECT_EQ("None[1, 2]|[1, 2]", tpl.RenderAsString({}).value());
    EXPECT_EQ("None[1, 2]|[1, 2]", tpl.RenderAsString({}).value());
}

// A global changed in place in an include, a block or an imported macro is seen by the rest of the render
TEST(TemplateApiTest, GlobalChangedInPlaceSeenEverywhere)
{
    TemplateEnv env;
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("inc", "{{ gl.append(3) }}");
    fs->AddFile("lib", "{% macro m() %}{{ gl.append(4) }}{% endmacro %}");
    env.AddFilesystemHandler(std::string(), fs);
    env.AddGlobal("gl", ValuesList{ 1, 2 });

    auto render = [&env](const std::string& source) {
        Template tpl(&env);
        EXPECT_TRUE(!!tpl.Load(source));
        return tpl.RenderAsString({}).value();
    };
    for (int i = 0; i < 2; ++i)
    {
        EXPECT_EQ("[1, 2]None[1, 2, 3]None[1, 2, 3, 3]", render("{% for i in [1, 2] %}{{ gl }}{% include 'inc' %}{% endfor %}{{ gl }}"));
        EXPECT_EQ("[1, 2]None[1, 2, 4]None", render("{% import 'lib' as l %}{% for i in [1, 2] %}{{ gl }}{{ l.m() }}{% endfor %}"));
        EXPECT_EQ("[1, 2]NONE[1, 2, 1]NONE[1, 2, 1, 2]", render("{% for i in [1, 2] %}{{ gl }}{% filter upper %}{{ gl.append(i) }}{% endfilter %}{% endfor %}{{ gl }}"));
    }
}

// A global replaced while other threads render: each render sees one state or the other,
// and the values it converted stay alive
TEST(TemplateApiTest, GlobalsChangedWhileRendering)
{
    TemplateEnv env;
    env.AddGlobal("g", std::string(64, 'a'));
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{% for i in range(20) %}{{ g }}{% endfor %}"));

    std::atomic<bool> stop{ false };
    std::atomic<int> bad{ 0 };
    std::vector<std::thread> threads;
    threads.reserve(3);
    for (int t = 0; t < 3; ++t)
    {
        threads.emplace_back([&] {
            while (!stop)
            {
                auto out = tpl.RenderAsString({}).value();
                constexpr size_t size = size_t{ 64 } * 20;
                if (out != std::string(size, 'a') && out != std::string(size, 'b'))
                {
                    ++bad;
                }
            }
        });
    }
    for (int i = 0; i < 2000; ++i)
    {
        env.AddGlobal("g", std::string(64, i % 2 ? 'a' : 'b'));
    }
    stop = true;
    for (auto& thread : threads)
    {
        thread.join();
    }
    EXPECT_EQ(0, bad.load());
}

// Equal while the objects share one loaded template: a copy, until one of them loads again
TEST(TemplateApiTest, EqualityComparesTheSharedTemplate)
{
    Template a;
    ASSERT_TRUE(!!a.Load("x"));
    Template b = a;
    Template c;
    ASSERT_TRUE(!!c.Load("x"));
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

// A failed reload keeps the previous template: its tree points into its source, which must
// stay alive (task 0134; the source is long enough to be outside the small-string buffer)
TEST(TemplateApiTest, FailedReloadKeepsPreviousTemplate)
{
    const std::string text(200, 'A');
    Template tpl;
    ASSERT_TRUE(!!tpl.Load(text + "{{ x }}", "first.j2tpl"));
    Template copy = tpl;

    auto reload = tpl.Load("{{ broken", "second.j2tpl");
    ASSERT_FALSE(!!reload);
    EXPECT_EQ("second.j2tpl", reload.error().GetErrorLocation().fileName);

    EXPECT_EQ(text + "1", tpl.RenderAsString({ { "x", 1 } }).value());
    EXPECT_EQ(text + "2", copy.RenderAsString({ { "x", 2 } }).value());

    TemplateW wide;
    const std::wstring wideText(200, L'A');
    ASSERT_TRUE(!!wide.Load(wideText + L"{{ x }}"));
    ASSERT_FALSE(!!wide.Load(L"{% if %}"));
    EXPECT_EQ(wideText + L"3", wide.RenderAsString({ { "x", 3 } }).value());

    // A template whose first Load fails stays unloaded
    Template never;
    ASSERT_FALSE(!!never.Load("{{ broken"));
    auto unloaded = never.RenderAsString({});
    ASSERT_FALSE(!!unloaded);
    EXPECT_EQ(ErrorCode::TemplateNotParsed, unloaded.error().GetCode());
}

// The first Load of a template parses into the template's own source string (0515). Host code that
// Load calls may load the same template again (here the autoescape selector): that Load must not
// reuse the string the first one is parsing, whether it succeeds or fails, and the outer Load's
// result is the one kept. Both a source inside the small-string buffer and a longer one
TEST(TemplateApiTest, LoadFromTheSelectorOfTheFirstLoad)
{
    for (const std::string& outerText : { std::string("O{{ x }}"), std::string(100, 'O') + "{{ x }}" })
    {
        for (const bool innerFails : { false, true })
        {
            TemplateEnv env;
            std::optional<Template> tpl;
            bool nested = false;
            bool innerLoaded = false;
            // Set before the template is made, which takes the environment's settings then
            env.GetSettings().autoescapeSelector = [&](const std::string&) {
                if (!nested)
                {
                    nested = true;
                    innerLoaded = !!tpl->Load(innerFails ? std::string("{{ broken") : std::string(60, 'I') + "{{ x }}");
                }
                return false;
            };
            tpl.emplace(&env);
            ASSERT_TRUE(!!tpl->Load(outerText));
            EXPECT_EQ(!innerFails, innerLoaded);
            EXPECT_EQ(outerText.substr(0, outerText.size() - 7) + "5", tpl->RenderAsString({ { "x", 5 } }).value());
        }
    }

    // The outer Load fails after the inner one succeeded: the inner template stays loaded
    TemplateEnv env;
    std::optional<Template> tpl;
    bool nested = false;
    env.GetSettings().autoescapeSelector = [&](const std::string&) {
        if (!nested)
        {
            nested = true;
            EXPECT_TRUE(!!tpl->Load("i{{ x }}"));
        }
        return false;
    };
    tpl.emplace(&env);
    ASSERT_FALSE(!!tpl->Load("{{ broken"));
    EXPECT_EQ("i6", tpl->RenderAsString({ { "x", 6 } }).value());
}

// Host code inside the first Load loads the same template twice or more: the second nested Load
// replaces the template the first Load still parses into, which stays until that Load returns
// (docs/tasks/0517). The last Load that replaced the template is the one it renders
template<typename CharT>
void LoadTwiceFromTheSelectorOfTheFirstLoad()
{
    using Tpl = std::conditional_t<std::is_same_v<CharT, char>, Template, TemplateW>;
    using Str = std::basic_string<CharT>;
    const auto widen = [](const std::string& text) { return Str(text.begin(), text.end()); };
    for (const int nestedLoads : { 2, 3 })
    {
        for (const bool release : { false, true })
        {
            for (const bool outerFails : { false, true })
            {
                SCOPED_TRACE(std::to_string(nestedLoads) + (release ? " release" : "") + (outerFails ? " outer fails" : ""));
                TemplateEnv env;
                std::optional<Tpl> tpl;
                bool ran = false;
                env.GetSettings().autoescapeSelector = [&](const std::string&) {
                    if (!ran)
                    {
                        ran = true;
                        for (int n = 0; n < nestedLoads; ++n)
                        {
                            EXPECT_TRUE(!!tpl->Load(widen(std::string(1, static_cast<char>('a' + n)) + std::string(40, 'n') + "{{ x }}")));
                            if (release)
                            {
                                ReleaseThreadCaches();
                            }
                        }
                    }
                    return false;
                };
                tpl.emplace(&env);
                EXPECT_EQ(!outerFails, !!tpl->Load(widen(outerFails ? "{{ broken" : "OUT{{ x }}")));
                const auto last = std::string(1, static_cast<char>('a' + nestedLoads - 1)) + std::string(40, 'n') + "5";
                EXPECT_EQ(widen(last), tpl->RenderAsString({ { "x", 5 } }).value());
                // Once the Load returned, the template it parsed into goes with the thread's caches
                EXPECT_TRUE(ReleaseThreadCaches());
                ASSERT_TRUE(!!tpl->Load(widen("again{{ x }}")));
                EXPECT_EQ(widen("again7"), tpl->RenderAsString({ { "x", 7 } }).value());
            }
        }
    }
}

TEST(TemplateApiTest, LoadTwiceFromTheSelectorOfTheFirstLoad)
{
    LoadTwiceFromTheSelectorOfTheFirstLoad<char>();
    LoadTwiceFromTheSelectorOfTheFirstLoad<wchar_t>();
}

// Host code a first Load calls replaces the template it parses into on a worker thread it joins,
// or assigns another template to the object: that Load finishes on the template it was parsing,
// and the object keeps what was loaded or assigned last (docs/tasks/0522)
template<typename CharT>
void ReplaceFromTheSelectorOfTheFirstLoad()
{
    using Tpl = std::conditional_t<std::is_same_v<CharT, char>, Template, TemplateW>;
    using Str = std::basic_string<CharT>;
    const auto widen = [](const std::string& text) { return Str(text.begin(), text.end()); };
    enum class Replace
    {
        LoadOnWorker,
        LoadTwiceOnWorker,
        Assign,
        AssignTemporary,
        AssignThenLoad
    };
    for (const auto replace : { Replace::LoadOnWorker, Replace::LoadTwiceOnWorker, Replace::Assign, Replace::AssignTemporary, Replace::AssignThenLoad })
    {
        for (const bool outerFails : { false, true })
        {
            SCOPED_TRACE(std::to_string(static_cast<int>(replace)) + (outerFails ? " outer fails" : ""));
            TemplateEnv env;
            std::optional<Tpl> tpl;
            Tpl other;
            ASSERT_TRUE(!!other.Load(widen("other" + std::string(40, 'o') + "{{ x }}")));
            bool ran = false;
            env.GetSettings().autoescapeSelector = [&](const std::string&) {
                if (ran)
                {
                    return false;
                }
                ran = true;
                switch (replace)
                {
                case Replace::LoadOnWorker:
                case Replace::LoadTwiceOnWorker:
                    std::thread([&] {
                        EXPECT_TRUE(!!tpl->Load(widen("a" + std::string(40, 'n') + "{{ x }}")));
                        if (replace == Replace::LoadTwiceOnWorker)
                        {
                            EXPECT_TRUE(!!tpl->Load(widen("b" + std::string(40, 'n') + "{{ x }}")));
                        }
                        // The worker's caches go with it
                        EXPECT_TRUE(ReleaseThreadCaches());
                    }).join();
                    break;
                case Replace::Assign:
                    *tpl = other;
                    break;
                case Replace::AssignTemporary:
                    *tpl = Tpl(other);
                    break;
                case Replace::AssignThenLoad:
                    *tpl = other;
                    EXPECT_TRUE(!!tpl->Load(widen("c" + std::string(40, 'n') + "{{ x }}")));
                    break;
                }
                EXPECT_TRUE(ReleaseThreadCaches());
                return false;
            };
            tpl.emplace(&env);
            EXPECT_EQ(!outerFails, !!tpl->Load(widen(outerFails ? "{{ broken" + std::string(40, 'f') : "OUT" + std::string(40, 'u') + "{{ x }}")));
            Str expected;
            switch (replace)
            {
            case Replace::LoadOnWorker:
                // One Load parses in place, into the template the first Load parses into: the
                // first one's tree replaces it, unless it fails
                expected = widen((outerFails ? "a" + std::string(40, 'n') : "OUT" + std::string(40, 'u')) + "5");
                break;
            case Replace::LoadTwiceOnWorker:
                expected = widen("b" + std::string(40, 'n') + "5");
                break;
            case Replace::Assign:
            case Replace::AssignTemporary:
                expected = widen("other" + std::string(40, 'o') + "5");
                break;
            case Replace::AssignThenLoad:
                expected = widen("c" + std::string(40, 'n') + "5");
                break;
            }
            EXPECT_EQ(expected, tpl->RenderAsString({ { "x", 5 } }).value());
            EXPECT_EQ(widen("other" + std::string(40, 'o') + "6"), other.RenderAsString({ { "x", 6 } }).value());
            EXPECT_TRUE(ReleaseThreadCaches());
            ASSERT_TRUE(!!tpl->Load(widen("again{{ x }}")));
            EXPECT_EQ(widen("again7"), tpl->RenderAsString({ { "x", 7 } }).value());
        }
    }
}

TEST(TemplateApiTest, ReplaceFromTheSelectorOfTheFirstLoad)
{
    ReplaceFromTheSelectorOfTheFirstLoad<char>();
    ReplaceFromTheSelectorOfTheFirstLoad<wchar_t>();
}

// The same, from the selector of a Load nested in the first one: the template stays kept until the
// first Load returned, not the nested one
template<typename CharT>
void ReplaceFromTheSelectorOfANestedLoad()
{
    using Tpl = std::conditional_t<std::is_same_v<CharT, char>, Template, TemplateW>;
    using Str = std::basic_string<CharT>;
    const auto widen = [](const std::string& text) { return Str(text.begin(), text.end()); };
    const auto padded = [](const std::string& head, char pad) { return head + std::string(40, pad); };
    enum class Replace
    {
        Assign,
        LoadTwice,
        LoadTwiceOnWorker
    };
    for (const auto replace : { Replace::Assign, Replace::LoadTwice, Replace::LoadTwiceOnWorker })
    {
        SCOPED_TRACE(static_cast<int>(replace));
        TemplateEnv env;
        std::optional<Tpl> tpl;
        Tpl other;
        ASSERT_TRUE(!!other.Load(widen(padded("other", 'o') + "{{ x }}")));
        int calls = 0;
        env.GetSettings().autoescapeSelector = [&](const std::string&) {
            ++calls;
            if (calls == 1)
            {
                // From the first Load: a nested Load, whose selector is the second call
                EXPECT_TRUE(!!tpl->Load(widen(padded("nested", 'n') + "{{ x }}")));
            }
            else if (calls == 2)
            {
                const auto loadTwice = [&] {
                    EXPECT_TRUE(!!tpl->Load(widen(padded("a", 'a') + "{{ x }}")));
                    EXPECT_TRUE(!!tpl->Load(widen(padded("b", 'b') + "{{ x }}")));
                };
                switch (replace)
                {
                case Replace::Assign:
                    *tpl = other;
                    break;
                case Replace::LoadTwice:
                    loadTwice();
                    break;
                case Replace::LoadTwiceOnWorker:
                    std::thread([&] {
                        loadTwice();
                        EXPECT_TRUE(ReleaseThreadCaches());
                    }).join();
                    break;
                }
                EXPECT_TRUE(ReleaseThreadCaches());
            }
            return false;
        };
        tpl.emplace(&env);
        EXPECT_TRUE(!!tpl->Load(widen(padded("OUT", 'u') + "{{ x }}")));
        const auto expected = replace == Replace::Assign ? padded("other", 'o') : padded("b", 'b');
        EXPECT_EQ(widen(expected + "5"), tpl->RenderAsString({ { "x", 5 } }).value());
        EXPECT_TRUE(ReleaseThreadCaches());
        ASSERT_TRUE(!!tpl->Load(widen("again{{ x }}")));
        EXPECT_EQ(widen("again7"), tpl->RenderAsString({ { "x", 7 } }).value());
    }
}

TEST(TemplateApiTest, ReplaceFromTheSelectorOfANestedLoad)
{
    ReplaceFromTheSelectorOfANestedLoad<char>();
    ReplaceFromTheSelectorOfANestedLoad<wchar_t>();
}

// Reloads of each source size, inside the small-string buffer and outside it, and failed reloads
// keep rendering the loaded template
TEST(TemplateApiTest, ReloadsKeepEachSourceSize)
{
    const std::string longText(100, 'L');
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("a{{ x }}"));
    EXPECT_EQ("a1", tpl.RenderAsString({ { "x", 1 } }).value());
    ASSERT_TRUE(!!tpl.Load(longText + "{{ x }}"));
    EXPECT_EQ(longText + "2", tpl.RenderAsString({ { "x", 2 } }).value());
    ASSERT_FALSE(!!tpl.Load("{{ broken"));
    EXPECT_EQ(longText + "3", tpl.RenderAsString({ { "x", 3 } }).value());
    ASSERT_TRUE(!!tpl.Load("b{{ x }}"));
    EXPECT_EQ("b4", tpl.RenderAsString({ { "x", 4 } }).value());
    ASSERT_FALSE(!!tpl.Load(longText + "{% if %}"));
    EXPECT_EQ("b5", tpl.RenderAsString({ { "x", 5 } }).value());
    ASSERT_TRUE(!!tpl.Load("c{{ x }}\r\n"));
    EXPECT_EQ("c6", tpl.RenderAsString({ { "x", 6 } }).value());
    ASSERT_TRUE(!!tpl.Load("d{{ x }}"));
    EXPECT_EQ("d7", tpl.RenderAsString({ { "x", 7 } }).value());

    TemplateW wide;
    ASSERT_TRUE(!!wide.Load(L"w{{ x }}"));
    ASSERT_TRUE(!!wide.Load(L"v{{ x }}"));
    ASSERT_FALSE(!!wide.Load(L"{{ broken"));
    EXPECT_EQ(L"v8", wide.RenderAsString({ { "x", 8 } }).value());
}

// Load replaces the template for that object only: copies, the environment's cache, the
// includes of the name and modules keep the tree they had, as Python's Template objects are
// immutable (docs/tasks/0321, embedding contract 3.1.5)
TEST(TemplateApiTest, LoadReplacesOnlyThisObject)
{
    Template a;
    ASSERT_TRUE(!!a.Load("old {{ x }}"));
    Template b = a;
    ASSERT_TRUE(!!a.Load("new {{ x }}"));
    EXPECT_TRUE(a != b);
    EXPECT_EQ("new 1", a.RenderAsString({ { "x", 1 } }).value());
    EXPECT_EQ("old 2", b.RenderAsString({ { "x", 2 } }).value());

    // The cache: Load on a template LoadTemplate returned
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("part.j2", "cached{% macro m() %}macro{% endmacro %}");
    fs->AddFile("main.j2", "[{% include 'part.j2' %}]");
    TemplateEnv env;
    env.AddFilesystemHandler({}, fs);
    auto cached = env.LoadTemplate("part.j2").value();
    auto module = cached.MakeModule().value();
    auto host = env.LoadTemplate("part.j2").value();
    ASSERT_TRUE(host == cached);
    ASSERT_TRUE(!!host.Load("host's{% macro m() %}host's macro{% endmacro %}"));
    EXPECT_EQ("host's", host.RenderAsString({}).value());
    EXPECT_EQ("cached", env.LoadTemplate("part.j2").value().RenderAsString({}).value());
    EXPECT_EQ("[cached]", env.LoadTemplate("main.j2").value().RenderAsString({}).value());
    // A module made before the Load keeps working on its tree
    UserCallableParams noArgs;
    EXPECT_EQ("macro", module.exports["m"].As<UserCallable>().callable(noArgs).AsString());
    EXPECT_EQ("host's macro", host.MakeModule().value().exports["m"].As<UserCallable>().callable(noArgs).AsString());

    TemplateW wide;
    ASSERT_TRUE(!!wide.Load(L"old"));
    TemplateW wideCopy = wide;
    ASSERT_TRUE(!!wide.Load(L"new"));
    EXPECT_EQ(L"new", wide.RenderAsString({}).value());
    EXPECT_EQ(L"old", wideCopy.RenderAsString({}).value());
}

// A host callable loads the template that is rendering: the render finishes on the tree it
// started with, and the next render uses the new one (docs/tasks/0321, contract 3.3; ASan)
TEST(TemplateApiTest, LoadOfTheRenderingTemplateFromACallable)
{
    Template tpl;
    std::vector<bool> loaded;
    ValuesMap params{ { "reload", MakeCallable([&tpl, &loaded]() {
                            // Long enough to be outside the small-string buffer
                            loaded.push_back(!!tpl.Load(std::string(100, 'n') + "{{ reload() }}"));
                            return Value(std::string("|"));
                        }) } };
    const std::string oldText(100, 'o');
    ASSERT_TRUE(!!tpl.Load(oldText + "{% for i in range(3) %}{{ reload() }}{{ i }}{% endfor %}" + oldText));
    EXPECT_EQ(oldText + "|0|1|2" + oldText, tpl.RenderAsString(params).value());
    EXPECT_EQ((std::vector<bool>{ true, true, true }), loaded);
    EXPECT_EQ(std::string(100, 'n') + "|", tpl.RenderAsString(params).value());

    std::string out;
    ASSERT_TRUE(!!tpl.Load(oldText + "{{ reload() }}" + oldText));
    ASSERT_TRUE(!!tpl.RenderTo(out, params));
    EXPECT_EQ(oldText + "|" + oldText, out);
}

// Host code may run after the render's statements: a host exception's what(), read when the
// error is made, loads the template that is rendering. The render still ends on its own template
// (Codex review of #665; ASan)
TEST(TemplateApiTest, LoadFromAHostExceptionOfTheRenderingTemplate)
{
    Template tpl;
    struct LoadingFailure final : std::exception
    {
        explicit LoadingFailure(Template* t)
            : tpl(t)
        {
        }
        [[nodiscard]] const char* what() const noexcept override
        {
            (void)tpl->Load(std::string(100, 'n'));
            return "failed";
        }
        Template* tpl;
    };
    struct ThrowingAccessor final : IMapItemAccessor
    {
        explicit ThrowingAccessor(Template* t)
            : tpl(t)
        {
        }
        [[nodiscard]] size_t GetSize() const override { return 1; }
        [[nodiscard]] bool HasValue(const std::string& /*name*/) const override { return true; }
        [[nodiscard]] Value GetValueByName(const std::string& /*name*/) const override { throw LoadingFailure(tpl); }
        [[nodiscard]] std::vector<std::string> GetKeys() const override { return { "x" }; }
        [[nodiscard]] bool IsEqual(const IComparable& other) const override { return this == &other; }
        Template* tpl;
    };
    const ThrowingAccessor accessor(&tpl);
    const GenericMap params([&accessor]() -> const IMapItemAccessor* { return &accessor; });
    // Named, long enough to be outside the small-string buffer: the error copies the name
    const std::string name = "a template whose name the error copies.j2";
    ASSERT_TRUE(!!tpl.Load("{{ x }}", name));
    auto result = tpl.RenderAsString(params);
    ASSERT_FALSE(!!result);
    EXPECT_EQ(name, result.error().GetErrorLocation().fileName);
    EXPECT_EQ(std::string(100, 'n'), tpl.RenderAsString(ValuesMap{}).value());
}

// A stream's buffer is host code: reading it loads the same template again, which replaces the
// template the outer Load started with. The outer Load still ends well (Codex review of #665; ASan)
TEST(TemplateApiTest, LoadFromTheStreamOfALoad)
{
    Template tpl;
    class LoadingBuffer final : public std::streambuf
    {
    public:
        explicit LoadingBuffer(Template* t)
            : m_tpl(t)
        {
        }

    protected:
        int_type underflow() override
        {
            if (m_done)
            {
                return traits_type::eof();
            }
            m_done = true;
            (void)m_tpl->Load(std::string(100, 'i'));
            setg(m_text.data(), m_text.data(), m_text.data() + m_text.size());
            return traits_type::to_int_type(m_text[0]);
        }

    private:
        Template* m_tpl;
        std::string m_text = std::string(100, 's');
        bool m_done = false;
    };
    ASSERT_TRUE(!!tpl.Load(std::string(100, 'o')));
    LoadingBuffer buffer(&tpl);
    std::istream stream(&buffer);
    ASSERT_TRUE(!!tpl.Load(stream));
    EXPECT_EQ(std::string(100, 's'), tpl.RenderAsString({}).value());
}

TEST(TemplateApiTest, MetadataStaysValidAcrossCalls)
{
    Template tpl;
    ASSERT_TRUE(!!tpl.Load(R"({% meta %}{"name": "first", "list": [1, 2]}{% endmeta %}x)"));
    auto first = tpl.GetMetadata();
    auto second = tpl.GetMetadata();
    ASSERT_TRUE(!!first);
    ASSERT_TRUE(!!second);
    // The first map must still be readable after the second call
    EXPECT_EQ("first", ToString(first.value()["name"]));
    EXPECT_EQ(std::optional<size_t>(2), first.value()["list"].As<GenericList>().GetSize());
    EXPECT_EQ("first", ToString(second.value()["name"]));

    ASSERT_TRUE(!!tpl.Load(R"({% meta %}{"name": "second"}{% endmeta %}y)"));
    EXPECT_EQ("second", ToString(tpl.GetMetadata().value()["name"]));
}

// Several threads render one template (and the templates it includes and extends) at
// once. Run under -DJINJA2CPP_WITH_SANITIZERS=thread, which CI does, to check for races.
TEST(TemplateApiTest, ConcurrentRenderOfOneTemplate)
{
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("base.j2tpl", "<{% block body %}{% endblock %}>");
    fs->AddFile("item.j2tpl", "[{{ item | upper }}]");
    TemplateEnv env;
    env.AddFilesystemHandler(std::string(), fs);
    env.AddGlobal("sep", ",");

    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load(R"({% meta %}{"kind": "test"}{% endmeta %}{% extends "base.j2tpl" %}{% block body %})"
                           R"({% macro m(v) %}{{ v * 2 }}{% endmacro %})"
                           R"({% for item in items %}{% include "item.j2tpl" %}{{ m(loop.index) }}{{ sep if not loop.last }}{% endfor %})"
                           R"({% set ns = namespace(total=0) %}{% for i in items %}{% set ns.total = ns.total + 1 %}{% endfor %}={{ ns.total }})"
                           R"(;{{ items | map('upper') | join }};{{ items | select('equalto', 't0') | join }})"
                           R"({% endblock %})"));
    const Template& shared = tpl;

    static constexpr std::size_t threadCount = 4;
    static constexpr int iterations = 50;
    std::vector<std::string> failures(threadCount);
    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (std::size_t t = 0; t < threadCount; ++t)
    {
        threads.emplace_back([&shared, &failures, t] {
            ValuesList items;
            for (std::size_t n = 0; n <= t; ++n)
            {
                items.emplace_back("t" + std::to_string(n));
            }
            std::string expected = "<";
            for (std::size_t n = 0; n <= t; ++n)
            {
                expected += "[T" + std::to_string(n) + "]" + std::to_string((n + 1) * 2) + (n == t ? "" : ",");
            }
            expected += "=" + std::to_string(t + 1) + ";";
            for (std::size_t n = 0; n <= t; ++n)
            {
                expected += "T" + std::to_string(n);
            }
            expected += ";t0>";

            for (int i = 0; i < iterations; ++i)
            {
                auto result = shared.RenderAsString({ { "items", items } });
                if (!result)
                    failures[t] = result.error().ToString();
                else if (result.value() != expected)
                    failures[t] = result.value() + " != " + expected;
                auto metadata = shared.GetMetadata();
                if (!metadata || !metadata.value().HasValue("kind"))
                    failures[t] = "metadata";
                if (!failures[t].empty())
                    return;
            }
        });
    }
    for (auto& th : threads)
        th.join();

    for (std::size_t t = 0; t < threadCount; ++t)
    {
        EXPECT_EQ("", failures[t]) << "thread " << t;
    }
}

// JINJA2CPP_VERSION is generated from project(VERSION); it was a stale 10100 (1.1.0)
// while the project was 1.3.2 (docs/tasks/0069)
TEST(TemplateApiTest, VersionMacrosMatchProjectVersion)
{
    static_assert(JINJA2CPP_VERSION == (JINJA2CPP_VERSION_MAJOR * 10000) + (JINJA2CPP_VERSION_MINOR * 100) + JINJA2CPP_VERSION_PATCH);
    // the inline ABI namespace is v2 (docs/tasks/0076); a new major retags it with scripts/abi_namespace.py
    static_assert(JINJA2CPP_VERSION_MAJOR == 2);
    const auto expected = std::to_string(JINJA2CPP_VERSION_MAJOR) + "." + std::to_string(JINJA2CPP_VERSION_MINOR) + "." + std::to_string(JINJA2CPP_VERSION_PATCH);
    EXPECT_EQ(expected, JINJA2CPP_VERSION_STRING);
}

// The oracle pair comes from .python-version and test/parity/requirements.txt (docs/tasks/0186)
TEST(TemplateApiTest, OracleVersionMacrosNameThePinnedPair)
{
    const std::string python = JINJA2CPP_ORACLE_PYTHON;
    const std::string jinja2 = JINJA2CPP_ORACLE_JINJA2;
    EXPECT_EQ(python.find_first_not_of("0123456789."), std::string::npos);
    EXPECT_EQ(1U, std::count(python.begin(), python.end(), '.'));
    EXPECT_EQ(jinja2.find_first_not_of("0123456789."), std::string::npos);
    EXPECT_GE(std::count(jinja2.begin(), jinja2.end(), '.'), 1);
}

// What renders keep on a thread is freed by ReleaseThreadCaches (docs/tasks/0275)
TEST(TemplateApiTest, ReleaseThreadCachesBetweenRenders)
{
    TemplateEnv env;
    env.AddGlobal("g", "global");
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load(R"({% macro m(x) %}<{{ x | e }}>{% endmacro %}{% for i in items %}{{ m(i) }}{% for j in [1, 2] %}{{ loop.index }}{% endfor %}{% endfor %}{{ g }}{{ '<&>' * 300 | e | length }})"));
    const ValuesMap params{ { "items", ValuesList{ "a&", "b", "c" } } };
    const auto first = tpl.RenderAsString(params);
    ASSERT_TRUE(first);
    EXPECT_TRUE(ReleaseThreadCaches());
    EXPECT_TRUE(ReleaseThreadCaches());
    EXPECT_EQ(first.value(), tpl.RenderAsString(params).value());
    EXPECT_TRUE(ReleaseThreadCaches());
    EXPECT_EQ(first.value(), tpl.RenderAsString(params).value());
}

TEST(TemplateApiTest, ReleaseThreadCachesRefusedDuringRender)
{
    std::vector<bool> released;
    Template inner;
    ASSERT_TRUE(!!inner.Load("{{ release() }}"));
    ValuesMap params{ { "release", MakeCallable([&released]() {
                            released.push_back(ReleaseThreadCaches());
                            return Value(std::string("r"));
                        }) } };
    params["nested"] = MakeCallable([&inner, &params]() { return Value(inner.RenderAsString(params).value()); });

    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro m() %}{{ release() }}{% endmacro %}{{ release() }}{% for i in [1] %}{{ m() }}{% endfor %}{{ nested() }}"));
    EXPECT_EQ("rrr", tpl.RenderAsString(params).value());
    EXPECT_EQ((std::vector<bool>{ false, false, false }), released);

    // From a sink too
    struct ReleasingSink final : OutputSink
    {
        void Write(std::string_view) override { released = ReleaseThreadCaches(); }
        bool released = true;
    };
    ReleasingSink sink;
    Template plain;
    ASSERT_TRUE(!!plain.Load("text"));
    ASSERT_TRUE(!!plain.RenderTo(sink, {}));
    EXPECT_FALSE(sink.released);

    // and once the render ended
    EXPECT_TRUE(ReleaseThreadCaches());
}

TEST(TemplateApiTest, ReleaseThreadCachesLetsGoOfDestroyedEnvironment)
{
    // The thread keeps the globals of the last environment it rendered with, and with them what
    // their callables capture, after the environment is destroyed
    auto sentinel = std::make_shared<int>(42);
    std::weak_ptr<int> watch = sentinel;
    {
        TemplateEnv env;
        env.AddGlobal("answer", MakeCallable([held = std::move(sentinel)]() { return Value(int64_t{ *held }); }));
        Template tpl(&env);
        ASSERT_TRUE(!!tpl.Load("{{ answer() }}"));
        EXPECT_EQ("42", tpl.RenderAsString({}).value());
    }
    EXPECT_FALSE(watch.expired());
    EXPECT_TRUE(ReleaseThreadCaches());
    EXPECT_TRUE(watch.expired());
}

// A capture whose destructor calls ReleaseThreadCaches: the render's own copy of the callable,
// in the params table, is destroyed while the render empties that table
TEST(TemplateApiTest, ReleaseThreadCachesRefusedWhileRenderEmptiesItsTables)
{
    struct Releaser
    {
        Releaser(const bool* isActive, std::vector<bool>* results)
            : active(isActive)
            , released(results)
        {}
        Releaser(const Releaser&) = default;
        Releaser(Releaser&&) = default;
        Releaser& operator=(const Releaser&) = default;
        Releaser& operator=(Releaser&&) = default;
        ~Releaser()
        {
            if (*active)
            {
                released->push_back(ReleaseThreadCaches());
            }
        }
        const bool* active;
        std::vector<bool>* released;
    };
    bool rendering = false;
    std::vector<bool> released;
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{{ f() }}{% set x = 'y' %}{{ x }}"));
    for (int round = 0; round != 2; ++round)
    {
        ValuesMap params{ { "f", MakeCallable([releaser = Releaser(&rendering, &released)]() { return Value("f"); }) } };
        rendering = true;
        const auto result = tpl.RenderAsString(params);
        rendering = false;
        EXPECT_EQ("fy", result.value());
    }
    EXPECT_EQ(std::vector<bool>(released.size(), false), released);
    EXPECT_TRUE(ReleaseThreadCaches());
}

TEST(TemplateApiTest, ReleaseThreadCachesOnSeveralThreads)
{
    TemplateEnv env;
    env.AddGlobal("g", "x");
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{% for i in range(20) %}{{ g }}{{ i | e }}{% endfor %}"));
    const auto expected = tpl.RenderAsString({}).value();
    std::atomic<int> failures{ 0 };
    std::vector<std::thread> threads;
    for (int idx = 0; idx != 4; ++idx)
    {
        threads.emplace_back([&]() {
            for (int round = 0; round != 200; ++round)
            {
                std::string out;
                if (!tpl.RenderTo(out, {}) || out != expected || !ReleaseThreadCaches())
                {
                    ++failures;
                }
            }
        });
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
    EXPECT_EQ(0, failures.load());
}
