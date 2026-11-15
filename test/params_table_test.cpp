#include "test_tools.h"

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// The params of a render as one level of names (docs/tasks/0513): a scan up to
// ParamsTable::LinearMax (8) params, an index past it. Each case renders with 0, 1, 8, 9 and
// 40 unused params besides its own, so that both ways are crossed. The expected output is
// Python Jinja2 3.1.6's
namespace
{
struct ParamsCase
{
    const char* tpl;
    jinja2::ValuesMap params;
    const char* expected;
};

std::wstring Widen(const std::string& text)
{
    return { text.begin(), text.end() };
}

class ParamsTableTest : public TemplateEnvFixture
{
protected:
    void SetUp() override
    {
        TemplateEnvFixture::SetUp();
        AddFile("inc", "[{{ hidden }}]");
        AddFile("mod", "{% set v = p %}");
        m_env.AddGlobal("g", 1);
        auto settings = m_env.GetSettings();
        settings.extensions.doStatement = true;
        m_env.SetSettings(settings);
    }

    void Check(const ParamsCase& test, size_t padding)
    {
        auto params = test.params;
        for (size_t idx = 0; idx != padding; ++idx)
        {
            params["pad" + std::to_string(idx)] = static_cast<int64_t>(idx);
        }
        SCOPED_TRACE(std::string(test.tpl) + " with " + std::to_string(padding) + " more params");
        jinja2::Template tpl(&m_env);
        ASSERT_TRUE(!!tpl.Load(test.tpl));
        const auto narrow = tpl.RenderAsString(params);
        ASSERT_TRUE(!!narrow) << narrow.error();
        EXPECT_EQ(test.expected, narrow.value());

        jinja2::TemplateW wtpl(&m_env);
        ASSERT_TRUE(!!wtpl.Load(Widen(test.tpl)));
        const auto wide = wtpl.RenderAsString(params);
        ASSERT_TRUE(!!wide);
        EXPECT_EQ(Widen(test.expected), wide.value());
    }
};

const std::vector<ParamsCase>& Cases()
{
    static const std::vector<ParamsCase> cases{
        // A name the template sets hides the param
        { R"({% set message = "a" %}{{ message }})", { { "message", "b" } }, "a" },
        // An include sees the params the includer never names
        { R"({% include "inc" %})", { { "hidden", 1 } }, "[1]" },
        { R"({% include "inc" without context %})", { { "hidden", 1 } }, "[]" },
        // An import with context sees them, one without context does not
        { R"({% import "mod" as m with context %}{{ m.v }})", { { "p", 3 } }, "3" },
        { R"({% import "mod" as m %}[{{ m.v }}])", { { "p", 3 } }, "[]" },
        // A param hides a builtin and a global
        { R"({{ range }})", { { "range", 7 } }, "7" },
        { R"({{ g }})", { { "g", 2 } }, "2" },
        { R"({{ g }})", {}, "1" },
        // A macro reads the top-level name set after it, before the param
        { R"({% macro f() %}{{ p }}{% endmacro %}{% set p = 5 %}{{ f() }})", { { "p", 1 } }, "5" },
        { R"({% macro f() %}{{ p }}{% endmacro %}{{ f() }})", { { "p", 1 } }, "1" },
        // A list param changed in place
        { R"({% do l.append(2) %}{{ l }})", { { "l", jinja2::ValuesList{ 1 } } }, "[1, 2]" },
        { R"({{ missing is undefined }})", {}, "True" },
        { R"({% for x in [1] %}{{ p }}{% endfor %})", { { "p", 4 } }, "4" },
    };
    return cases;
}

TEST_F(ParamsTableTest, LookupsAcrossTheIndexThreshold)
{
    for (const auto& test : Cases())
    {
        for (const size_t padding : { size_t{ 0 }, size_t{ 1 }, size_t{ 8 }, size_t{ 9 }, size_t{ 40 } })
        {
            Check(test, padding);
        }
    }
}

// A GenericMap whose keys repeat a name: the param is there once
struct RepeatingAccessor : jinja2::IMapItemAccessor
{
    std::map<std::string, int64_t> items;

    [[nodiscard]] size_t GetSize() const override { return items.size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return items.count(name) != 0; }
    [[nodiscard]] jinja2::Value GetValueByName(const std::string& name) const override
    {
        auto p = items.find(name);
        return p == items.end() ? jinja2::Value() : jinja2::Value(p->second);
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> keys;
        for (const auto& item : items)
        {
            keys.push_back(item.first);
            keys.push_back(item.first);
        }
        return keys;
    }
};

TEST_F(ParamsTableTest, GenericMapParams)
{
    for (const size_t count : { size_t{ 1 }, size_t{ 4 }, size_t{ 5 }, size_t{ 12 } })
    {
        SCOPED_TRACE(count);
        RepeatingAccessor accessor;
        for (size_t idx = 0; idx != count; ++idx)
        {
            accessor.items["k" + std::to_string(idx)] = static_cast<int64_t>(idx * 10);
        }
        const jinja2::GenericMap context([&accessor]() { return &accessor; });
        jinja2::Template tpl(&m_env);
        ASSERT_TRUE(!!tpl.Load(R"({{ k0 }}|{{ k1 | default("-") }}|{% import "mod" as m with context %}{{ m.v | default("") }}|{{ missing is undefined }})"));
        const auto result = tpl.RenderAsString(context);
        ASSERT_TRUE(!!result) << result.error();
        EXPECT_EQ(count > 1 ? "0|10||True" : "0|-||True", result.value());
    }
}
} // namespace
