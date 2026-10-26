#include "embedding_lifecycle_test.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace jinja2;
using namespace embedding_lifecycle;

// ---------------------------------------------------------------------------------------------------------------------
// E. Values across the boundary

// 3.10 values 3 and 7: a callable returns reflected data, an rvalue (owned), a const& to a host object (borrowed) and a
// shared_ptr (shared); later statements of the render and an include use it
TEST(EmbeddingLifecycleTest, E1_ReflectedReturnValues)
{
    const auto repeat = [](const char* text) {
        std::string result;
        for (int i = 0; i < 14; ++i)
        {
            result += text;
        }
        return result;
    };
    const Person hostPerson{ repeat("bor"), { "p", "q", "r" } };

    CountingFs fs;
    fs.Set("inc.j2", "{{ o.name }}{{ b.tags|length }}{{ s.tags[0] }}{% for t in o.tags %}{{ t }}{% endfor %}");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    env.AddGlobal("owned", MakeCallable([&] { return Reflect(Person{ repeat("own"), { "x", "y" } }); }));
    env.AddGlobal("borrowed", MakeCallable([&hostPerson] { return Reflect(hostPerson); }));
    env.AddGlobal("shared", MakeCallable([&] { return Reflect(std::make_shared<Person>(Person{ repeat("shr"), { "m" } })); }));
    auto tpl = env.FromString(
        "{% set o = owned() %}{% set b = borrowed() %}{% set s = shared() %}{{ o.name }},{{ b.name }},{{ s.name }}|{{ o.tags|join('+') }},{{ b.tags|join('+') }},{{ s.tags|join('+') }}|{% include 'inc.j2' %}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    const std::string expected = repeat("own") + "," + repeat("bor") + "," + repeat("shr") + "|x+y,p+q+r,m|" + repeat("own") + "3mxy";
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(expected, Out(tpl->RenderAsString({})));
    }
}

// 3.10 values 2: lists, dicts, ranges, map results, namespaces and loop items the template made, kept by a callable, are
// readable after the render, the template and the environment are gone, and after another render reused the thread's tables
TEST(EmbeddingLifecycleTest, E2_TemplateMadeValuesOutliveRenderAndEnv)
{
    const std::string a(40, 'a');
    const std::string b(40, 'b');
    struct Case
    {
        const char* source;
        std::string expected;
    };
    const Case cases[] = {
        { "{{ keep([1, 2, [3, 4]]) }}", "[1,2,[3,4,],]" },
        { "{{ keep({'a': [1, 2]}) }}", "{a:[1,2,],}" },
        { "{{ keep(range(3)) }}", "[0,1,2,]" },
        { "{{ keep(items | map('upper')) }}", "[" + std::string(40, 'A') + "," + std::string(40, 'B') + ",]" },
        { "{% set l = [1, 2] %}{{ keep(l) }}", "[1,2,]" },
        { "{% for x in [[1], [2]] %}{{ keep(x) }}{% endfor %}", "[1,];[2,]" },
        { "{{ keep(namespace(x=5)) }}", "{x:5,}" },
        { "{{ keep(dict(x=[7])) }}", "{x:[7,],}" },
    };
    for (const auto& testCase : cases)
    {
        SCOPED_TRACE(testCase.source);
        auto kept = std::make_shared<Kept>();
        {
            TemplateEnv env;
            Template tpl(&env);
            ASSERT_TRUE(tpl.Load(testCase.source));
            const ValuesMap params{ { "items", ValuesList{ a, b } }, { "keep", KeepCallable(kept) } };
            ASSERT_TRUE(tpl.RenderAsString(params));
            // A second render on the thread reuses the workspace tables
            Template other;
            ASSERT_TRUE(other.Load("{% set q = [9, 9, 9] %}{{ q }}{{ {'zz': 'yy'} }}"));
            ASSERT_TRUE(other.RenderAsString({}));
        }
        std::string text;
        for (const auto& value : kept->values)
        {
            text += (text.empty() ? "" : ";") + Dump(value);
        }
        EXPECT_EQ(testCase.expected, text);
    }
}

// 2.1 and values 4: callables that throw jinja2::Exception, std::runtime_error and a type that is no exception give the
// error code, the location and the exception_ptr; the template and the thread's caches keep working
TEST(EmbeddingLifecycleTest, E4_ThrowingCallables)
{
    TemplateEnv env;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("ab\n{{ f() }}", "e.j2"));
    const auto thrower = [](int kind) {
        return MakeCallable([kind]() -> std::string {
            if (kind == 0)
            {
                throw Exception(ErrorCode::InvalidValueType, "typed failure");
            }
            if (kind == 1)
            {
                throw std::runtime_error("std failure");
            }
            throw 42;
        });
    };

    auto typed = tpl.RenderAsString({ { "f", thrower(0) } });
    ASSERT_FALSE(typed);
    EXPECT_EQ(ErrorCode::InvalidValueType, typed.error().GetCode());
    EXPECT_EQ("e.j2", typed.error().GetErrorLocation().fileName);
    EXPECT_EQ(2u, typed.error().GetErrorLocation().line);
    ASSERT_FALSE(typed.error().GetExtraParams().empty());
    EXPECT_EQ("typed failure", typed.error().GetExtraParams()[0].AsString());

    auto standard = tpl.RenderAsString({ { "f", thrower(1) } });
    ASSERT_FALSE(standard);
    EXPECT_EQ(ErrorCode::UserCallableException, standard.error().GetCode());
    EXPECT_EQ("e.j2", standard.error().GetErrorLocation().fileName);
    EXPECT_EQ(2u, standard.error().GetErrorLocation().line);
    ASSERT_FALSE(standard.error().GetExtraParams().empty());
    EXPECT_EQ("std failure", standard.error().GetExtraParams()[0].AsString());
    ASSERT_TRUE(standard.error().GetException());
    EXPECT_THROW(std::rethrow_exception(standard.error().GetException()), std::runtime_error);

    auto foreign = tpl.RenderAsString({ { "f", thrower(2) } });
    ASSERT_FALSE(foreign);
    EXPECT_EQ("e.j2", foreign.error().GetErrorLocation().fileName);
    EXPECT_EQ(2u, foreign.error().GetErrorLocation().line);
    ASSERT_TRUE(foreign.error().GetException());
    try
    {
        std::rethrow_exception(foreign.error().GetException());
        ADD_FAILURE() << "rethrow_exception returned";
    }
    catch (int value)
    {
        EXPECT_EQ(42, value);
    }

    // The same template and thread render again, and so does another template of the environment
    EXPECT_EQ("ab\nok", Out(tpl.RenderAsString({ { "f", MakeCallable([]() { return std::string("ok"); }) } })));
    EXPECT_EQ("fine", Out(env.FromString("fine").value().RenderAsString({})));
    EXPECT_TRUE(ReleaseThreadCaches());
}

// 3.10 values 6: exports made with MakeModule(params) that refer to the params are read after the params, the template
// and the module are gone; an item read from an exported list is valid while the list lives
TEST(EmbeddingLifecycleTest, E5_ModuleExportsOwnTheirStorage)
{
    const std::string a(40, 'a');
    const std::string b(40, 'b');
    std::optional<TemplateModule> module;
    {
        Template tpl;
        ASSERT_TRUE(tpl.Load("{% set x = items %}{% set y = items | map('upper') | list %}{% set z = data %}"));
        const ValuesMap params{ { "items", ValuesList{ a, b } }, { "data", ValuesMap{ { "k", ValuesList{ 1, 2 } } } } };
        auto made = tpl.MakeModule(params);
        ASSERT_TRUE(made) << made.error().ToString();
        module = std::move(*made);
    }
    const std::string upper = "[" + std::string(40, 'A') + "," + std::string(40, 'B') + ",]";
    EXPECT_EQ("[" + a + "," + b + ",]", Dump(module->exports["x"]));
    EXPECT_EQ(upper, Dump(module->exports["y"]));
    EXPECT_EQ("{k:[1,2,],}", Dump(module->exports["z"]));

    Value x = module->exports["x"];
    Value y = module->exports["y"];
    Value z = module->exports["z"];
    GenericList xs = x.ToGenericList();
    std::optional<Value> firstItem;
    for (const auto& item : xs)
    {
        firstItem = item;
        break;
    }
    module.reset();
    EXPECT_TRUE(ReleaseThreadCaches());
    EXPECT_EQ("[" + a + "," + b + ",]", Dump(x));
    EXPECT_EQ(upper, Dump(y));
    EXPECT_EQ("{k:[1,2,],}", Dump(z));
    ASSERT_TRUE(firstItem.has_value());
    EXPECT_EQ(a, Dump(firstItem.value()));
}

namespace
{
struct StringMapAccessor : IMapItemAccessor
{
    std::map<std::string, std::string> items;

    [[nodiscard]] size_t GetSize() const override { return items.size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return items.count(name) != 0; }
    [[nodiscard]] Value GetValueByName(const std::string& name) const override
    {
        const auto p = items.find(name);
        return p == items.end() ? Value() : Value(p->second);
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> keys;
        keys.reserve(items.size());
        for (const auto& item : items)
        {
            keys.push_back(item.first);
        }
        return keys;
    }
};

struct IntMapAccessor : IMapItemAccessor
{
    std::map<std::string, int64_t> items;

    [[nodiscard]] size_t GetSize() const override { return items.size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return items.count(name) != 0; }
    [[nodiscard]] Value GetValueByName(const std::string& name) const override
    {
        const auto p = items.find(name);
        return p == items.end() ? Value() : Value(p->second);
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> keys;
        keys.reserve(items.size());
        for (const auto& item : items)
        {
            keys.push_back(item.first);
        }
        return keys;
    }
};

template<typename CharT>
void CheckGenericContainersAcrossStatements()
{
    // Borrowed: the host's vector and accessor outlive every render. Owned: the callable and accessor live in the Value
    const std::vector<std::string> hostList{ "a", "b" };
    StringMapAccessor hostMap;
    hostMap.items = { { "a", "A" }, { "b", "B" } };
    const std::vector<int64_t> ownedItems{ 1, 2, 3 };
    IntMapAccessor ownedMap;
    ownedMap.items = { { "x", 1 }, { "y", 2 } };

    CountingFs fs;
    fs.Set("inc.j2", "{% for k, v in mo|dictsort %}{{ k }}={{ v }};{% endfor %}{{ mb.a }}{{ lb|length }}");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    auto tpl = FromStringIn(env, "{% macro show(l) %}{% for x in l %}{{ x }},{% endfor %}{% endmacro %}{{ show(lo) }}|{{ show(lb) }}|{% include 'inc.j2' %}", Tag<CharT>{});
    ASSERT_TRUE(tpl);
    for (int i = 0; i < 3; ++i)
    {
        // A generated list is walked once, so each render gets its own
        ValuesMap params{
            { "lo", MakeGenericList([items = ownedItems, pos = size_t{ 0 }]() mutable -> std::optional<Value> {
                  if (pos == items.size())
                  {
                      return std::nullopt;
                  }
                  return Value(items[pos++]);
              }) },
            { "lb", MakeGenericList(hostList.begin(), hostList.end()) },
            { "mo", GenericMap([accessor = ownedMap]() { return &accessor; }) },
            { "mb", GenericMap([&hostMap]() { return &hostMap; }) },
        };
        EXPECT_EQ(Str<CharT>("1,2,3,|a,b,|x=1;y=2;A2"), Text(tpl->RenderAsString(params)));
    }
}
} // namespace

// 3.10 values 1: GenericList and GenericMap params, owned and borrowed, read across macros, loops and an include, again on later renders
TEST(EmbeddingLifecycleTest, E6_GenericContainersAcrossStatements)
{
    CheckGenericContainersAcrossStatements<char>();
    CheckGenericContainersAcrossStatements<wchar_t>();
}

// ---------------------------------------------------------------------------------------------------------------------
// F. Stored macro, self, super and caller crossing frames. The plain forms are in the parity corpus; these need the host

// 3.10 threads 1 and values: a macro stored in a namespace inside a loop and called after it, with the frames reused
// by a second loop of another shape; on four threads
TEST(EmbeddingLifecycleTest, F1_MacroStoredInLoopCalledAfterIt)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% set ns = namespace(m=none) %}{% set base = 'B' %}{% for i in [1, 2, 3] %}{% macro mm(x) %}[{{ base }}:{{ x }}]{% endmacro %}{% if i == 2 %}{% set ns.m = mm %}{% endif %}{% endfor %}"
                         "{% for a, b in [(10, 20), (30, 40)] %}{% set t = a + b %}{{ t }},{% endfor %}{{ ns.m(w) }}"));
    Latch start;
    Failures failures;
    std::vector<std::thread> workers;
    workers.reserve(4);
    for (int t = 0; t < 4; ++t)
    {
        workers.emplace_back([&, t] {
            EXPECT_TRUE(start.Wait());
            for (int i = 0; i < 50; ++i)
            {
                const std::string w = "w" + std::to_string((t * 100) + i);
                const std::string actual = Out(tpl.RenderAsString({ { "w", w } }));
                if (actual != "30,70,[B:" + w + "]")
                {
                    failures.Add(actual);
                }
            }
        });
    }
    start.CountDown();
    for (auto& worker : workers)
    {
        worker.join();
    }
    EXPECT_EQ("", failures.Summary());
}

// 3.10 caching 1 and 3: `self` stored by an include and called after the include ended (0207), with the include
// reloaded in between; the stored one keeps the old template
TEST(EmbeddingLifecycleTest, F2_SelfStoredByIncludeSurvivesReload)
{
    CountingFs fs(true);
    fs.Set("inc.j2", "{% block blk %}B{{ y }}{% endblock %}{% set ns.s = self %}");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    env.AddGlobal("reload", MakeCallable([&]() {
                      fs.Set("inc.j2", "{% block blk %}N{{ y }}{% endblock %}{% set ns.s = self %}");
                      fs.Touch("inc.j2");
                      return env.LoadTemplate("inc.j2").has_value() ? std::string() : std::string("reload failed");
                  }));
    auto tpl = env.FromString("{% set ns = namespace(s=none) %}{% include 'inc.j2' %}{{ reload() }}|{{ ns.s.blk() }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    // The variable is a render parameter: one the includer sets itself is not seen by the stored `self` (see the task notes)
    const ValuesMap params{ { "y", "Y" } };
    EXPECT_EQ("BY|BY", Out(tpl->RenderAsString(params)));
    EXPECT_EQ("NY|NY", Out(tpl->RenderAsString(params)));
}

// 3.10 destruction order: `super` stored in a block and called after the block ended, while the environment, which
// cached the parent, is destroyed from inside the block
TEST(EmbeddingLifecycleTest, F3_SuperStoredInBlockAfterEnvDestroyed)
{
    CountingFs fs;
    fs.Set("parent.j2", "{% block b %}P{% endblock %}|{{ ns.s() }}");
    fs.Set("child.j2", "{% extends 'parent.j2' %}{% set ns = namespace(s=none) %}{% block b %}{% set ns.s = super %}C{{ super() }}{{ drop() }}{% endblock %}");
    auto env = std::make_unique<TemplateEnv>();
    env->AddFilesystemHandler("", fs);
    auto child = env->LoadTemplate("child.j2");
    ASSERT_TRUE(child) << child.error().ToString();
    const ValuesMap params{ { "drop", MakeCallable([&env]() {
                                  env.reset();
                                  return std::string();
                              }) } };
    EXPECT_EQ("CP|P", Out(child->RenderAsString(params)));
    EXPECT_FALSE(env);
    EXPECT_EQ("CP|P", Out(child->RenderAsString(params)));
}

// 3.10 caching 3 and 4: macros of two imported modules stored by a macro and called after it returned, with caching off,
// and after the environment object is gone
TEST(EmbeddingLifecycleTest, F4_ImportedMacrosCalledAfterImporterReturned)
{
    for (const bool dropEnv : { false, true })
    {
        SCOPED_TRACE(dropEnv);
        CountingFs fs;
        fs.Set("lib.j2", "{% macro hi(x) %}hi {{ x }}{% endmacro %}");
        fs.Set("lib2.j2", "{% macro hi(x) %}ho {{ x }}{% endmacro %}");
        auto env = std::make_unique<TemplateEnv>();
        env->GetSettings().cacheSize = 0;
        env->AddFilesystemHandler("", fs);
        auto tpl = env->FromString(
            "{% set ns = namespace(f=none, g=none) %}{% macro get() %}{% import 'lib.j2' as lib %}{% import 'lib2.j2' as lib2 %}{% set ns.f = lib.hi %}{% set ns.g = lib2.hi %}{% endmacro %}"
            "{{ get() }}{{ drop() }}{{ ns.f('a') }}|{{ ns.g('b') }}");
        ASSERT_TRUE(tpl) << tpl.error().ToString();
        const ValuesMap params{ { "drop", MakeCallable([&]() {
                                      if (dropEnv)
                                      {
                                          env.reset();
                                      }
                                      return std::string();
                                  }) } };
        EXPECT_EQ("hi a|ho b", Out(tpl->RenderAsString(params)));
        EXPECT_EQ("hi a|ho b", Out(tpl->RenderAsString(params)));
        EXPECT_EQ(!dropEnv, !!env);
    }
}

// 3.10 values: `caller` stored by a macro and called after the macro returned (a closure over the enclosing macro's names is
// not pinned: it fails on master, see the task notes)
TEST(EmbeddingLifecycleTest, F5_CallerCalledAfterItsMacroReturned)
{
    const char* source = "{% set ns = namespace(c=none) %}{% set v = 'V' %}{% macro m() %}{% set ns.c = caller %}x{% endmacro %}{% call m() %}body {{ v }}{% endcall %}|{{ ns.c() }}";
    Template narrow;
    ASSERT_TRUE(narrow.Load(source));
    TemplateW wide;
    ASSERT_TRUE(wide.Load(Str<wchar_t>(source)));
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ("x|body V", Out(narrow.RenderAsString({})));
        EXPECT_EQ(L"x|body V", Text(wide.RenderAsString({})));
    }
}
