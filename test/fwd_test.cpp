// jinja2cpp/fwd.h is enough to declare functions over the public types and to specialise the
// extension points; jinja2cpp/jinja2cpp.h then brings the whole API (docs/tasks/0076).
// Built with -Wmismatched-tags (CMakeLists.txt), so fwd.h must use each type's own class-key.
#include <jinja2cpp/fwd.h>

namespace fwd_test
{
struct Local
{
    int answer = 0;
};

jinja2::Value Describe(const jinja2::Value& value, jinja2::Template* tpl, jinja2::TemplateEnv& env, const jinja2::ValuesMap& params, const jinja2::ValuesList& items);
} // namespace fwd_test

template<>
struct jinja2::Reflector<fwd_test::Local>
{
    static Value Create(const fwd_test::Local& val);
};

#include <jinja2cpp/jinja2cpp.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

static_assert(std::is_same_v<jinja2::Template, jinja2::BasicTemplate<char>>);
static_assert(std::is_same_v<jinja2::ErrorInfoW, jinja2::BasicErrorInfo<wchar_t>>);
static_assert(std::is_same_v<jinja2::FilesystemHandlerPtr, std::shared_ptr<jinja2::IFilesystemHandler>>);

jinja2::Value jinja2::Reflector<fwd_test::Local>::Create(const fwd_test::Local& val)
{
    return ValuesMap{ { "answer", static_cast<int64_t>(val.answer) } };
}

jinja2::Value fwd_test::Describe(const jinja2::Value& value, jinja2::Template* tpl, jinja2::TemplateEnv& /*env*/, const jinja2::ValuesMap& params, const jinja2::ValuesList& items)
{
    auto loaded = tpl->Load("{{ value.answer }} {{ name }} {{ items | join(',') }}");
    if (!loaded)
    {
        return jinja2::Value(std::string("load failed"));
    }
    jinja2::ValuesMap all = params;
    all["value"] = value;
    all["items"] = items;
    auto rendered = tpl->RenderAsString(all);
    return rendered ? jinja2::Value(rendered.value()) : jinja2::Value(std::string("render failed"));
}

TEST(FwdTest, DeclaredThroughFwdHeaderUsedThroughUmbrella)
{
    jinja2::TemplateEnv env;
    jinja2::Template tpl(&env);
    const auto out = fwd_test::Describe(jinja2::Reflect(fwd_test::Local{ 42 }), &tpl, env, { { "name", "fwd" } }, { 1, 2 });
    EXPECT_EQ("42 fwd 1,2", out.AsString());
}

// Public functions defined in the library must be exported: a shared build hides the rest
// (-fvisibility=hidden), and UserCallable's inline copy assignment calls operator==
TEST(FwdTest, PublicComparisonsLinkAgainstSharedLibrary)
{
    EXPECT_TRUE(jinja2::Value(1).IsEqual(jinja2::Value(1)));

    jinja2::UserCallable first;
    jinja2::UserCallable second;
    second = first;
    EXPECT_TRUE(first == second);
    EXPECT_FALSE(first != second);
}
