// jinja2cpp/fwd.h is enough to declare functions over the public types and to specialise the
// extension points; jinja2cpp/jinja2cpp.h then brings the whole API (docs/tasks/0076).
// Built with -Wmismatched-tags (CMakeLists.txt), so fwd.h must use each type's own class-key.
#include <jinja2cpp/fwd.h>

#include <string>

namespace fwd_test
{
struct Local
{
    int answer = 0;
};

jinja2::Value Describe(const jinja2::Value& value, jinja2::Template* tpl, jinja2::TemplateEnv& env, const jinja2::ValuesMap& params, const jinja2::ValuesList& items);
jinja2::ErrorCode CodeOf(const jinja2::Exception& ex);
bool Stream(const jinja2::Template& tpl, jinja2::OutputSink& sink);
bool StreamWide(const jinja2::TemplateW& tpl, jinja2::OutputSinkW& sink);
bool Collect(const jinja2::Template& tpl, jinja2::ContainerSink<std::string>& sink);
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
static_assert(std::is_same_v<jinja2::OutputSink, jinja2::BasicOutputSink<char>>);
static_assert(std::is_same_v<jinja2::OutputSinkW, jinja2::BasicOutputSink<wchar_t>>);
static_assert(std::is_same_v<jinja2::ContainerSink<std::string>, jinja2::ContainerSink<std::string, char>>);

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

jinja2::ErrorCode fwd_test::CodeOf(const jinja2::Exception& ex)
{
    return ex.Code();
}

bool fwd_test::Stream(const jinja2::Template& tpl, jinja2::OutputSink& sink)
{
    return !!tpl.RenderTo(sink, {});
}

bool fwd_test::StreamWide(const jinja2::TemplateW& tpl, jinja2::OutputSinkW& sink)
{
    return !!tpl.RenderTo(sink, {});
}

bool fwd_test::Collect(const jinja2::Template& tpl, jinja2::ContainerSink<std::string>& sink)
{
    return Stream(tpl, sink);
}

TEST(FwdTest, SinksDeclaredThroughFwdHeader)
{
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load("{{ 6 * 7 }}"));
    std::string narrow;
    jinja2::ContainerSink<std::string> narrowSink(narrow);
    ASSERT_TRUE(fwd_test::Collect(tpl, narrowSink));
    EXPECT_EQ("42", narrow);
    jinja2::TemplateW wide;
    ASSERT_TRUE(wide.Load(L"{{ 6 * 7 }}"));
    std::wstring out;
    jinja2::ContainerSink<std::wstring> sink(out);
    ASSERT_TRUE(fwd_test::StreamWide(wide, sink));
    EXPECT_EQ(L"42", out);
}

TEST(FwdTest, DeclaredThroughFwdHeaderUsedThroughUmbrella)
{
    jinja2::TemplateEnv env;
    jinja2::Template tpl(&env);
    const auto out = fwd_test::Describe(jinja2::Reflect(fwd_test::Local{ 42 }), &tpl, env, { { "name", "fwd" } }, { 1, 2 });
    EXPECT_EQ("42 fwd 1,2", out.AsString());
    EXPECT_EQ(jinja2::ErrorCode::UndefinedError, fwd_test::CodeOf(jinja2::Exception(jinja2::ErrorCode::UndefinedError, "x")));
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
