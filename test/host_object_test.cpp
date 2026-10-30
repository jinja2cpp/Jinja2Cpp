#include "../src/expression_evaluator.h"
#include "../src/host_object.h"
#include "../src/internal_value.h"
#include "../src/template_dict.h"
#include "../src/template_impl.h"

#include "gtest/gtest.h"

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace jinja2;

// Internal functions are not exported from a shared library; the Python module links the
// library statically
#ifndef JINJA2CPP_LINK_AS_SHARED

// The private seam the Python module builds on (docs/tasks/0352): a toy object of a "host
// language" written in C++, with each operation of IHostObject switchable, read by templates
namespace
{
struct ToySpec
{
    std::string str = "Тоу";
    std::string repr = "<Toy>";
    std::map<std::string, InternalValue> attrs;
    std::map<std::string, InternalValue> items;
    std::map<int64_t, InternalValue> intItems;
    bool callable = false;
    std::optional<std::vector<InternalValue>> iteration;
    std::optional<std::vector<std::string>> contains;
    std::optional<size_t> len;
    std::optional<bool> truth;
    std::optional<int64_t> equalsInt;
    bool hashable = true;
    std::optional<std::string> html;
};

class ToyObject final : public IHostObject
{
public:
    explicit ToyObject(std::shared_ptr<const ToySpec> spec)
        : m_spec(std::move(spec))
    {
    }

    [[nodiscard]] const char* TypeName() const override { return "Toy"; }
    [[nodiscard]] std::string Str() const override { return m_spec->str; }
    [[nodiscard]] std::string Repr() const override { return m_spec->repr; }
    [[nodiscard]] std::optional<InternalValue> GetAttr(const std::string& name) const override { return Find(m_spec->attrs, name); }
    [[nodiscard]] std::optional<InternalValue> GetItem(const InternalValue& key) const override
    {
        if (IsStringValue(key))
        {
            return Find(m_spec->items, AsString(key));
        }
        if (const auto* index = GetIf<int64_t>(&key))
        {
            return Find(m_spec->intItems, *index);
        }
        return std::nullopt;
    }
    [[nodiscard]] bool IsCallable() const override { return m_spec->callable; }
    InternalValue Call(const CallParams& params, RenderContext& context) const override
    {
        if (!m_spec->callable)
        {
            return IHostObject::Call(params, context);
        }
        std::string result = "called(" + std::to_string(params.posParams.size());
        for (const auto& [name, value] : params.kwParams)
        {
            result += "," + name + "=" + (IsStringValue(value) ? AsString(value) : std::string("<callable>"));
        }
        return InternalValue(result + ")");
    }
    [[nodiscard]] bool IsIterable() const override { return m_spec->iteration.has_value(); }
    [[nodiscard]] std::optional<ListAdapter> Iterate() const override
    {
        if (!m_spec->iteration)
        {
            return std::nullopt;
        }
        // Single pass, as a Python iterator
        auto items = *m_spec->iteration;
        size_t next = 0;
        return ListAdapter::CreateAdapter([items = std::move(items), next]() mutable -> std::optional<InternalValue> {
            if (next == items.size())
            {
                return std::nullopt;
            }
            return items[next++];
        });
    }
    [[nodiscard]] std::optional<bool> Contains(const InternalValue& item) const override
    {
        if (!m_spec->contains)
        {
            return std::nullopt;
        }
        const auto& names = *m_spec->contains;
        return IsStringValue(item) && std::find(names.begin(), names.end(), AsString(item)) != names.end();
    }
    [[nodiscard]] std::optional<size_t> Len() const override { return m_spec->len; }
    [[nodiscard]] bool Truth() const override { return m_spec->truth ? *m_spec->truth : IHostObject::Truth(); }
    [[nodiscard]] std::optional<bool> Equals(const InternalValue& other) const override
    {
        const auto* number = GetIf<int64_t>(&other);
        if (!m_spec->equalsInt || !number)
        {
            return std::nullopt;
        }
        return *number == *m_spec->equalsInt;
    }
    [[nodiscard]] std::optional<size_t> Hash() const override { return m_spec->hashable ? IHostObject::Hash() : std::nullopt; }
    // Every view of one spec is one object, as two proxies of one PyObject are
    [[nodiscard]] const void* Identity() const override { return m_spec.get(); }
    [[nodiscard]] bool IsMarkupLike() const override { return m_spec->html.has_value(); }
    [[nodiscard]] std::string Html() const override { return m_spec->html.value_or(""); }

private:
    template<typename Map, typename Key>
    static std::optional<InternalValue> Find(const Map& map, const Key& key)
    {
        auto pos = map.find(key);
        return pos == map.end() ? std::nullopt : std::make_optional(pos->second);
    }

    std::shared_ptr<const ToySpec> m_spec;
};

InternalValue MakeToy(std::shared_ptr<const ToySpec> spec)
{
    return MakeHostValue<ToyObject>(std::move(spec));
}

InternalValue MakeToy(ToySpec spec = {})
{
    return MakeToy(std::make_shared<const ToySpec>(std::move(spec)));
}

template<typename CharT>
std::basic_string<CharT> Render(const std::basic_string<CharT>& source, const InternalValueMap& params)
{
    BasicTemplate<CharT> tpl;
    auto loaded = tpl.Load(source);
    if (!loaded)
    {
        return ConvertString<std::basic_string<CharT>>(std::string("load error: ")) + loaded.error().ToString();
    }
    std::basic_string<CharT> out;
    if (auto error = TemplateImpl<CharT>::RenderInternal(tpl, out, params))
    {
        return ConvertString<std::basic_string<CharT>>(std::string("error: ")) + error->ToString();
    }
    return out;
}

std::string Render(const std::string& source, const InternalValueMap& params)
{
    return Render<char>(source, params);
}

std::wstring RenderW(const std::wstring& source, const InternalValueMap& params)
{
    return Render<wchar_t>(source, params);
}

ToySpec Rich()
{
    ToySpec spec;
    spec.attrs = { { "name", InternalValue(std::string("attr-name")) }, { "both", InternalValue(std::string("attr-both")) } };
    spec.items = { { "key", InternalValue(std::string("item-key")) }, { "both", InternalValue(std::string("item-both")) } };
    spec.intItems = { { 1, InternalValue(std::string("item-1")) } };
    return spec;
}
} // namespace

TEST(HostObjectTest, StrPrintsAndReprInsideContainers)
{
    const InternalValueMap params{ { "x", MakeToy() } };
    EXPECT_EQ("Тоу [<Toy>] Тоу|Тоу {'k': <Toy>}", Render("{{ x }} {{ [x] }} {{ x ~ '|' ~ x|string }} {{ {'k': x} }}", params));
    EXPECT_EQ(L"Тоу [<Toy>] Тоу", RenderW(L"{{ x }} {{ [x] }} {{ x|string }}", params));
}

TEST(HostObjectTest, AttributeBeforeItemAndItemBeforeAttribute)
{
    const InternalValueMap params{ { "x", MakeToy(Rich()) } };
    // x.name is getattr, then getitem; x['name'] is getitem, then getattr, as in Jinja2
    EXPECT_EQ("attr-name item-key attr-both|item-key attr-name item-both|item-1|", Render("{{ x.name }} {{ x.key }} {{ x.both }}|{{ x['key'] }} {{ x['name'] }} {{ x['both'] }}|{{ x[1] }}|{{ x[2] }}", params));
    EXPECT_EQ("True False", Render("{{ x.missing is undefined }} {{ x.name is undefined }}", params));
    // attr() reads attributes only
    EXPECT_EQ("attr-both - attr-name", Render("{{ x|attr('both') }} {{ x|attr('key') or '-' }} {{ x|attr('name') }}", params));
    // No dict methods are added to a host object: a missing one is undefined
    EXPECT_EQ("True", Render("{{ x.items is undefined }}", params));
    EXPECT_EQ(L"attr-name item-key", RenderW(L"{{ x.name }} {{ x['key'] }}", params));
}

TEST(HostObjectTest, CallsWithPositionalAndKeywordArguments)
{
    ToySpec spec;
    spec.callable = true;
    spec.attrs = { { "method", MakeToy(spec) } };
    const InternalValueMap params{ { "f", MakeToy(spec) }, { "o", MakeToy() } };
    EXPECT_EQ("called(2,k=v) called(0) True False", Render("{{ f(1, 2, k='v') }} {{ f.method() }} {{ f is callable }} {{ o is callable }}", params));
    EXPECT_EQ(L"called(1)", RenderW(L"{{ f(1) }}", params));
    // A call block passes caller=, as to any callable
    EXPECT_EQ("called(1,caller=<callable>)", Render("{% call f(1) %}body{% endcall %}", params));
    EXPECT_NE(std::string::npos, Render("{{ o() }}", params).find("'Toy' object is not callable"));
}

TEST(HostObjectTest, IteratesOnceAsAnIterator)
{
    ToySpec spec;
    spec.iteration = std::vector<InternalValue>{ InternalValue(int64_t{ 1 }), InternalValue(std::string("b")), MakeToy() };
    const InternalValueMap params{ { "it", MakeToy(spec) }, { "o", MakeToy() } };
    EXPECT_EQ("1,b,Тоу,|[1, 'b', <Toy>]|1-b-Тоу|True False", Render("{% for i in it %}{{ i }},{% endfor %}|{{ it|list }}|{{ it|join('-') }}|{{ it is iterable }} {{ o is iterable }}", params));
    EXPECT_EQ(L"1,b,Тоу,", RenderW(L"{% for i in it %}{{ i }},{% endfor %}", params));
    EXPECT_NE(std::string::npos, Render("{% for i in o %}{% endfor %}", params).find("'Toy' object is not iterable"));
}

TEST(HostObjectTest, ContainsThenIteration)
{
    ToySpec withContains;
    withContains.contains = std::vector<std::string>{ "a" };
    ToySpec withIteration;
    withIteration.iteration = std::vector<InternalValue>{ InternalValue(int64_t{ 1 }), InternalValue(int64_t{ 2 }) };
    const InternalValueMap params{ { "c", MakeToy(withContains) }, { "i", MakeToy(withIteration) }, { "o", MakeToy() } };
    EXPECT_EQ("True False True False", Render("{{ 'a' in c }} {{ 'b' in c }} {{ 2 in i }} {{ 3 in i }}", params));
    EXPECT_NE(std::string::npos, Render("{{ 1 in o }}", params).find("argument of type 'Toy' is not iterable"));
    // A host object in a list is found by its __eq__
    ToySpec equalsTwo;
    equalsTwo.equalsInt = 2;
    EXPECT_EQ("True False", Render("{{ 2 in [x] }} {{ 3 in [x] }}", { { "x", MakeToy(equalsTwo) } }));
}

TEST(HostObjectTest, LengthAndTruth)
{
    ToySpec sized;
    sized.len = 3;
    ToySpec empty;
    empty.len = 0;
    ToySpec falsy;
    falsy.truth = false;
    const InternalValueMap params{ { "s", MakeToy(sized) }, { "e", MakeToy(empty) }, { "f", MakeToy(falsy) }, { "o", MakeToy() } };
    EXPECT_EQ("3 0|yes no no yes|False", Render("{{ s|length }} {{ e|count }}|{{ s and 'yes' or 'no' }} {{ e and 'yes' or 'no' }} {{ 'yes' if f else 'no' }} {{ 'yes' if o else 'no' }}|{{ not o }}", params));
    EXPECT_NE(std::string::npos, Render("{{ o|length }}", params).find("object of type 'Toy' has no len()"));
}

TEST(HostObjectTest, EqualityAsksEitherOperandThenIdentity)
{
    ToySpec equalsTwo;
    equalsTwo.equalsInt = 2;
    auto shared = std::make_shared<const ToySpec>();
    const InternalValueMap params{ { "two", MakeToy(equalsTwo) }, { "a", MakeToy(shared) }, { "a2", MakeToy(shared) }, { "b", MakeToy() } };
    // __eq__ of the left operand, then of the right one (2 == two), then identity
    EXPECT_EQ("True True False True|True True False True|True True", Render("{{ two == 2 }} {{ 2 == two }} {{ two == 3 }} {{ two != 3 }}|{{ a == a }} {{ a == a2 }} {{ a == b }} {{ a != b }}|{{ a is sameas a2 }} {{ a == a2 and not (a is sameas b) }}", params));
    EXPECT_EQ("False False", Render("{{ a == 'Тоу' }} {{ a == none }}", params));
    EXPECT_NE(std::string::npos, Render("{{ a < 1 }}", params).find("'<' not supported between instances of 'Toy' and 'int'"));
}

TEST(HostObjectTest, MarkupLikeObjectUnderAutoescape)
{
    ToySpec markupLike;
    markupLike.str = "<plain>";
    markupLike.html = "<b>Тоу</b>";
    ToySpec plain;
    plain.str = "<plain>";
    const InternalValueMap params{ { "m", MakeToy(markupLike) }, { "p", MakeToy(plain) } };
    // str() without autoescape, __html__() with it, as markupsafe.escape has it
    EXPECT_EQ("<plain> <b>Тоу</b> &lt;plain&gt;|True False", Render("{{ m }} {% autoescape true %}{{ m }} {{ p }}{% endautoescape %}|{{ m is escaped }} {{ p is escaped }}", params));
    EXPECT_EQ(L"<b>Тоу</b> &lt;plain&gt;", RenderW(L"{% autoescape true %}{{ m }} {{ p }}{% endautoescape %}", params));
    // safe, striptags and forceescape read __html__() too; string is str()
    EXPECT_EQ("<b>Тоу</b>|Тоу|&lt;b&gt;Тоу&lt;/b&gt;|<plain>", Render("{{ m|safe }}|{{ m|striptags }}|{{ m|forceescape }}|{{ m|string }}", params));
    EXPECT_EQ(L"<b>Тоу</b>|Тоу", RenderW(L"{{ m|safe }}|{{ m|striptags }}", params));
}

TEST(HostObjectTest, DictKeysByIdentity)
{
    auto shared = std::make_shared<const ToySpec>();
    ToySpec unhashable;
    unhashable.hashable = false;
    const InternalValueMap params{ { "a", MakeToy(shared) }, { "a2", MakeToy(shared) }, { "b", MakeToy() }, { "u", MakeToy(unhashable) } };
    // Two views of one object are one key; another object is another key
    EXPECT_EQ("2 one two|True False|[<Toy>, <Toy>]", Render("{% set d = {a: 'one', b: 'two'} %}{% set d2 = {a: 'x', a2: 'one'} %}{{ d|length }} {{ d2[a] }} {{ d[b] }}|{{ a in d }} {{ 1 in d }}|{{ [a, a2, b]|unique|list }}", params));
    EXPECT_NE(std::string::npos, Render("{{ {u: 1} }}", params).find("unhashable type: 'Toy'"));
}

TEST(HostObjectTest, KeyKeepsTheObjectAlive)
{
    // The dict's key outlives every other reference to the object: the token's address stays valid
    TemplateDict dict;
    {
        auto key = MakeToy();
        dict.Set(key, InternalValue(int64_t{ 1 }));
    }
    ASSERT_EQ(1u, dict.size());
    const auto key = dict.KeyOf(dict.begin()->first);
    ASSERT_NE(nullptr, GetHostObject(key));
    EXPECT_EQ("Тоу", GetHostObject(key)->Str());
    EXPECT_NE(dict.end(), dict.Find(key));
}

TEST(HostObjectTest, PublicViewReadsAttributes)
{
    // A host object handed to the host's public API (a user callable's argument) is a map of
    // its attributes
    const auto toy = MakeToy(Rich());
    const auto map = AsMap(toy);
    ASSERT_TRUE(map.has_value());
    const auto generic = map->CreateGenericMap();
    EXPECT_TRUE(generic.HasValue("name"));
    EXPECT_EQ("attr-name", generic.GetValueByName("name").As<std::string>());
    EXPECT_EQ("item-key", generic.GetValueByName("key").As<std::string>());
}

#endif // JINJA2CPP_LINK_AS_SHARED
