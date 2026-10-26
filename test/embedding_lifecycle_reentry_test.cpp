#include "embedding_lifecycle_test.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace jinja2;
using namespace embedding_lifecycle;

// ---------------------------------------------------------------------------------------------------------------------
// D. Reentrancy

// 3.10 reentrancy: a callable renders the same template recursively (bounded by a parameter) and another template of the
// environment; the nested render keeps the limits of its own template
TEST(EmbeddingLifecycleTest, D1_NestedRendersOfTheSameAndOtherTemplates)
{
    TemplateEnv env;
    auto main = env.FromString("{% if n > 0 %}[{{ n }}{{ rec(n - 1) }}<{{ other(n) }}>]{% endif %}");
    auto other = env.FromString("o{{ n * 2 }}");
    ASSERT_TRUE(main && other);
    env.AddGlobal("rec", MakeCallable([&main](int64_t n) { return Text(main->RenderAsString({ { "n", n } })); }, ArgInfo{ "n" }));
    env.AddGlobal("other", MakeCallable([&other](int64_t n) { return Text(other->RenderAsString({ { "n", n } })); }, ArgInfo{ "n" }));
    EXPECT_EQ("[3[2[1<o2>]<o4>]<o6>]", Out(main->RenderAsString({ { "n", int64_t{ 3 } } })));

    // The inner template comes from an environment with a small output limit; the outer one has none
    TemplateEnv limited;
    limited.GetSettings().limits.maxOutputSize = 5;
    auto inner = limited.FromString("xxxxxxxxxxxx");
    ASSERT_TRUE(inner) << inner.error().ToString();
    TemplateEnv free;
    auto outer = free.FromString("a{{ bridge() }}b");
    ASSERT_TRUE(outer) << outer.error().ToString();
    std::optional<ErrorInfo> innerError;
    const ValuesMap params{ { "bridge", MakeCallable([&]() {
                                  auto r = inner->RenderAsString({});
                                  if (r)
                                  {
                                      return std::string("fits");
                                  }
                                  innerError = r.error();
                                  return std::string(r.error().GetExtraParams().at(1).AsString());
                              }) } };
    EXPECT_EQ("amaxOutputSizeb", Out(outer->RenderAsString(params)));
    ASSERT_TRUE(innerError.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, innerError.value().GetCode());
}

// 3.10 reentrancy: a callable loads templates, builds one from a string and changes globals and filters mid-render; the
// running render keeps its snapshot and the next one sees the changes
TEST(EmbeddingLifecycleTest, D2_ConfigurationChangesFromACallable)
{
    CountingFs fs;
    fs.Set("x.j2", "X");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    env.AddGlobal("g", "old");
    env.AddGlobal("gone", "here");
    std::string inner;
    env.AddGlobal("mutate", MakeCallable([&]() {
                      env.AddGlobal("g", "new");
                      env.RemoveGlobal("gone");
                      env.AddFilter("shout", UserCallable([](const UserCallableParams& p) { return Value(p["s"].AsString() + "!"); }, { ArgInfo{ "s" } }));
                      auto loaded = env.LoadTemplate("x.j2");
                      auto built = env.FromString("{{ g }}{{ 'a'|shout }}");
                      inner = loaded && built ? Text(loaded->RenderAsString({})) + Text(built->RenderAsString({})) : std::string("load failed");
                      return std::string("M");
                  }));
    auto tpl = env.FromString("{{ g }}{{ gone }}{{ mutate() }}{{ g }}{{ gone }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    EXPECT_EQ("oldhereMoldhere", Out(tpl->RenderAsString({})));
    EXPECT_EQ("Xnewa!", inner);
    EXPECT_TRUE(env.FindFilter("shout"));
    EXPECT_EQ("newMnew", Out(tpl->RenderAsString({})));
}

// 3.10 threads 7 and reentrancy: macro -> callable -> the same module's macro; a nested render's error goes back to the
// outer callable, which carries on; an exception thrown inside a nested render reaches the outer render at the outer
// location
TEST(EmbeddingLifecycleTest, D3_ModuleReentryAndNestedErrors)
{
    std::weak_ptr<TemplateModule> watchModule;
    {
        TemplateEnv env;
        Template tpl(&env);
        ASSERT_TRUE(tpl.Load("{% macro f(n) %}{% if n > 0 %}{{ n }}{{ back(n - 1) }}{% endif %}{% endmacro %}"));
        auto module = std::make_shared<TemplateModule>();
        watchModule = module;
        env.AddGlobal("back", MakeCallable([module](int64_t k) { return CallValue(module->exports["f"], { k }); }, ArgInfo{ "k" }));
        *module = tpl.MakeModule().value();
        EXPECT_EQ("321", Dump(CallValue(module->exports["f"], { int64_t{ 3 } })));
        // The callable holds the module, and the module's macros hold the globals they were made with, `back` among
        // them: the host ends the cycle by removing the global and dropping the exports
        env.RemoveGlobal("back");
        module->exports.clear();
    }
    EXPECT_TRUE(ReleaseThreadCaches());
    EXPECT_TRUE(watchModule.expired());
    {
        TemplateEnv env;
        auto bad = env.FromString("{{ nosuch.attr }}", "inner.j2");
        ASSERT_TRUE(bad) << bad.error().ToString();
        std::optional<ErrorInfo> innerError;
        env.AddGlobal("tryInner", MakeCallable([&]() {
                          auto r = bad->RenderAsString({});
                          if (r)
                          {
                              return std::string("ok");
                          }
                          innerError = r.error();
                          return "E" + std::to_string(static_cast<int>(r.error().GetCode()));
                      }));
        auto outer = env.FromString("a{{ tryInner() }}b{{ tryInner() }}c");
        ASSERT_TRUE(outer) << outer.error().ToString();
        EXPECT_EQ("aE11bE11c", Out(outer->RenderAsString({})));
        ASSERT_TRUE(innerError.has_value());
        EXPECT_EQ(ErrorCode::UndefinedError, innerError.value().GetCode());
        EXPECT_EQ("inner.j2", innerError.value().GetErrorLocation().fileName);
    }
    {
        TemplateEnv env;
        auto deep = env.FromString("{{ boom() }}", "deep.j2");
        ASSERT_TRUE(deep) << deep.error().ToString();
        env.AddGlobal("boom", MakeCallable([]() -> std::string { throw std::runtime_error("deep"); }));
        env.AddGlobal("bridge", MakeCallable([&]() -> std::string {
                          auto r = deep->RenderAsString({});
                          if (!r)
                          {
                              std::rethrow_exception(r.error().GetException());
                          }
                          return *r;
                      }));
        auto outer = env.FromString("line1\n{{ bridge() }}", "outer.j2");
        ASSERT_TRUE(outer) << outer.error().ToString();
        auto result = outer->RenderAsString({});
        ASSERT_FALSE(result);
        EXPECT_EQ(ErrorCode::UserCallableException, result.error().GetCode());
        EXPECT_EQ("outer.j2", result.error().GetErrorLocation().fileName);
        EXPECT_EQ(2u, result.error().GetErrorLocation().line);
        ASSERT_FALSE(result.error().GetExtraParams().empty());
        EXPECT_EQ("deep", result.error().GetExtraParams()[0].AsString());
    }
}

namespace
{
// Renders a template on every Write, and records what that made
class NestingSink final : public OutputSink
{
public:
    explicit NestingSink(const Template& nested)
        : m_nested(nested)
    {
    }

    void Write(std::string_view text) override
    {
        m_out.append(text);
        m_expectedNested += "[" + std::to_string(text.size()) + "]";
        m_nestedOut += Text(m_nested.RenderAsString({ { "w", static_cast<int64_t>(text.size()) } }));
    }

    std::string m_out;
    std::string m_nestedOut;
    std::string m_expectedNested;

private:
    const Template& m_nested;
};
} // namespace

// 3.10 reentrancy: a render started from a sink's Write, from finalize, from a gettext callable, from the autoescape
// selector (at Load) and from a handler that loads another name of the same environment (no lock is held across the
// handler call; a deadlock would hang this row)
TEST(EmbeddingLifecycleTest, D4_RendersFromHostHooks)
{
    Template nested;
    ASSERT_TRUE(nested.Load("[{{ w }}]"));
    Template wrap;
    ASSERT_TRUE(wrap.Load("<{{ v }}>"));
    Template translated;
    ASSERT_TRUE(translated.Load("({{ m }})"));

    {
        TemplateEnv env;
        Template tpl(&env);
        ASSERT_TRUE(tpl.Load("ab{{ x }}cd{{ 'ef' }}"));
        NestingSink sink(nested);
        ASSERT_TRUE(tpl.RenderTo(sink, { { "x", "X" } }));
        EXPECT_EQ("abXcdef", sink.m_out);
        EXPECT_FALSE(sink.m_nestedOut.empty());
        EXPECT_EQ(sink.m_expectedNested, sink.m_nestedOut);
    }
    {
        TemplateEnv env;
        env.GetSettings().finalize = UserCallable([&wrap](const UserCallableParams& p) { return Value(Text(wrap.RenderAsString({ { "v", p["v"] } }))); }, { ArgInfo{ "v" } });
        auto tpl = env.FromString("{{ 1 }}-{{ 'a' }}");
        ASSERT_TRUE(tpl) << tpl.error().ToString();
        EXPECT_EQ("<1>-<a>", Out(tpl->RenderAsString({})));
    }
    {
        TemplateEnv env;
        env.GetSettings().extensions.i18n = true;
        env.InstallGettextCallables(UserCallable([&translated](const UserCallableParams& p) { return Value(Text(translated.RenderAsString({ { "m", p["message"] } }))); }, { ArgInfo{ "message" } }),
                                    UserCallable([](const UserCallableParams& p) { return p["s"]; }, { ArgInfo{ "s" }, ArgInfo{ "p" }, ArgInfo{ "n" } }));
        auto tpl = env.FromString("{{ gettext('hi') }}|{{ _('yo') }}");
        ASSERT_TRUE(tpl) << tpl.error().ToString();
        EXPECT_EQ("(hi)|(yo)", Out(tpl->RenderAsString({})));
    }
    {
        TemplateEnv env;
        std::vector<std::string> seen;
        std::string nestedOut;
        env.GetSettings().autoescapeSelector = AutoescapeSelector([&](const std::string& name) {
            seen.push_back(name);
            nestedOut += Text(nested.RenderAsString({ { "w", int64_t{ 7 } } }));
            return true;
        });
        auto tpl = env.FromString("{{ '<' }}", "page.html");
        ASSERT_TRUE(tpl) << tpl.error().ToString();
        EXPECT_EQ("&lt;", Out(tpl->RenderAsString({})));
        ASSERT_EQ(1u, seen.size());
        EXPECT_EQ("page.html", seen[0]);
        EXPECT_EQ("[7]", nestedOut);
    }
    {
        CountingFs fs;
        fs.Set("main.j2", "M{% include 'dep.j2' %}");
        fs.Set("dep.j2", "D");
        fs.Set("side.j2", "S");
        TemplateEnv env;
        env.AddFilesystemHandler("", fs);
        std::string sideOut;
        fs.SetOpenHook([&](const std::string& name) {
            if (name == "main.j2")
            {
                const auto side = env.LoadTemplate("side.j2");
                sideOut = side ? Out(side->RenderAsString({})) : "error: " + side.error().ToString();
            }
        });
        auto main = env.LoadTemplate("main.j2");
        ASSERT_TRUE(main) << main.error().ToString();
        EXPECT_EQ("S", sideOut);
        EXPECT_EQ("MD", Out(main->RenderAsString({})));
    }
}
