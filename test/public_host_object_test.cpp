#include "gtest/gtest.h"

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/generic_list_iterator.h> // IWYU pragma: keep (GenericList::iterator is only declared in generic_list.h)
#include <jinja2cpp/host_object.h>
#include <jinja2cpp/markup.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace jinja2;

// The public host object interface (docs/tasks/0357 B1, B2) through public headers only: a toy object of a "host
// language" written in C++, with each operation of IHostObject switchable, read by templates (test/host_object_test.cpp
// does the same against the engine's private interface)
namespace
{
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding): a test's spec, read for clarity, not size
struct ToySpec
{
    std::string str = "Тоу";
    std::string repr = "<Toy>";
    std::map<std::string, Value> attrs;
    std::map<std::string, Value> items;
    std::map<int64_t, Value> intItems;
    bool callable = false;
    std::optional<std::vector<Value>> iteration;
    // Whether iteration gives a list with a size and an indexer, rather than a single-pass one
    bool indexable = false;
    std::optional<std::vector<std::string>> contains;
    std::optional<size_t> len;
    std::optional<bool> truth;
    std::optional<int64_t> equalsInt;
    bool hashable = true;
    std::optional<std::string> html;
    // __getitem__ answers every other key with the type of the key
    bool typeKeys = false;
    // A sequence: __getitem__ takes a slice of it, and __reversed__ gives it last first
    std::optional<std::vector<int64_t>> sequence;
    // What a call returns instead of describing its arguments
    std::optional<Value> callResult;
    bool mapping = false;
    bool listOrTuple = false;
    bool tuple = false;
    // A call keeps the template callable it was handed as `caller`, for a later call to try
    mutable std::optional<UserCallable> kept;
    mutable std::string keptResult;
    // What the object saw of its last item key, operand and call arguments
    mutable std::vector<Value> seen;
};

class ListOf final : public IListItemAccessor
    , private IIndexBasedAccessor
{
public:
    ListOf(std::vector<Value> items, bool indexable)
        : m_items(std::move(items))
        , m_indexable(indexable)
    {
    }

    [[nodiscard]] const IIndexBasedAccessor* GetIndexer() const override { return m_indexable ? this : nullptr; }
    [[nodiscard]] std::optional<ListEnumeratorPtr> CreateEnumerator() const override { return MakeEnumerator<Enumerator>(m_items); }
    [[nodiscard]] std::optional<size_t> GetSize() const override { return m_indexable ? std::optional<size_t>(m_items.size()) : std::nullopt; }

private:
    class Enumerator final : public IListEnumerator
    {
    public:
        explicit Enumerator(const std::vector<Value>& items)
            : m_items(items)
        {
        }
        void Reset() override { m_next = 0; }
        bool MoveNext() override
        {
            if (m_next == m_items.size())
            {
                return false;
            }
            m_current = m_next++;
            return true;
        }
        [[nodiscard]] Value GetCurrent() const override { return m_items[m_current]; }
        [[nodiscard]] ListEnumeratorPtr Clone() const override { return MakeEnumerator<Enumerator>(m_items); }
        ListEnumeratorPtr Move() override { return MakeEnumerator<Enumerator>(m_items); }

    private:
        const std::vector<Value>& m_items;
        size_t m_next = 0;
        size_t m_current = 0;
    };

    [[nodiscard]] Value GetItemByIndex(int64_t idx) const override { return m_items[static_cast<size_t>(idx)]; }

    std::vector<Value> m_items;
    bool m_indexable;
};

GenericList MakeList(std::vector<Value> items, bool indexable = true)
{
    return GenericList(std::make_shared<const ListOf>(std::move(items), indexable));
}

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
    [[nodiscard]] std::optional<Value> GetAttr(std::string_view name) const override { return Find(m_spec->attrs, std::string(name)); }
    [[nodiscard]] std::optional<Value> GetItem(const Value& key) const override
    {
        m_spec->seen = { key };
        if (key.IsString())
        {
            return Find(m_spec->items, key.AsString());
        }
        if (key.IsInt())
        {
            return Find(m_spec->intItems, key.AsInt());
        }
        if (m_spec->typeKeys)
        {
            const char* kind = key.IsList() ? "list" : "other";
            return Value(std::string(key.IsMap() ? "map" : kind) + "-key");
        }
        return std::nullopt;
    }
    [[nodiscard]] std::optional<Value> GetSlice(const Value& start, const Value& stop, const Value& step) const override
    {
        m_spec->seen = { start, stop, step };
        if (!m_spec->sequence)
        {
            return std::nullopt;
        }
        // Python's slice holds None for an omitted bound
        const auto& seq = *m_spec->sequence;
        const auto size = static_cast<int64_t>(seq.size());
        const int64_t stride = step.IsInt() ? step.AsInt() : 1;
        int64_t from = start.IsInt() ? start.AsInt() : (stride > 0 ? 0 : size - 1);
        int64_t to = stop.IsInt() ? stop.AsInt() : (stride > 0 ? size : -1);
        if (start.IsInt() && from < 0)
        {
            from += size;
        }
        if (stop.IsInt() && to < 0)
        {
            to += size;
        }
        ValuesList result;
        for (int64_t i = from; stride > 0 ? i < std::min(to, size) : i > to && i < size; i += stride)
        {
            result.emplace_back(seq[static_cast<size_t>(i)]);
        }
        return Value(std::move(result));
    }
    [[nodiscard]] std::optional<GenericList> Reversed() const override
    {
        if (!m_spec->sequence)
        {
            return std::nullopt;
        }
        std::vector<Value> items;
        for (auto it = m_spec->sequence->rbegin(); it != m_spec->sequence->rend(); ++it)
        {
            items.emplace_back(*it);
        }
        return MakeList(std::move(items));
    }
    [[nodiscard]] bool IsCallable() const override { return m_spec->callable; }
    [[nodiscard]] Value Call(const UserCallableParams& params) const override
    {
        if (!m_spec->callable)
        {
            return IHostObject::Call(params);
        }
        const auto* positional = params.extraPosArgs.GetIf<ValuesList>();
        const auto* keywords = params.extraKwArgs.GetIf<ValuesMap>();
        m_spec->seen = positional ? *positional : ValuesList();
        if (keywords)
        {
            for (const auto& [name, value] : *keywords)
            {
                m_spec->seen.push_back(value);
            }
        }
        if (keywords)
        {
            if (const auto caller = keywords->find("caller"); caller != keywords->end() && caller->second.IsCallable())
            {
                m_spec->kept = *caller->second.GetIf<UserCallable>();
                UserCallableParams none;
                none.paramsParsed = true;
                m_spec->keptResult = ToString(m_spec->kept->callable(none));
            }
        }
        if (m_spec->callResult)
        {
            return *m_spec->callResult;
        }
        std::string result = "called(" + std::to_string(positional ? positional->size() : 0);
        if (keywords)
        {
            for (const auto& [name, value] : *keywords)
            {
                result += "," + name + "=" + (value.IsString() ? value.AsString() : std::string("<callable>"));
            }
        }
        return Value(result + ")");
    }
    [[nodiscard]] bool IsIterable() const override { return m_spec->iteration.has_value(); }
    [[nodiscard]] std::optional<GenericList> Iterate() const override
    {
        if (!m_spec->iteration)
        {
            return std::nullopt;
        }
        // Single pass, as a Python iterator, unless it is a sequence
        return MakeList(*m_spec->iteration, m_spec->indexable);
    }
    [[nodiscard]] std::optional<bool> Contains(const Value& item) const override
    {
        if (!m_spec->contains)
        {
            return std::nullopt;
        }
        const auto& names = *m_spec->contains;
        return item.IsString() && std::find(names.begin(), names.end(), item.AsString()) != names.end();
    }
    [[nodiscard]] std::optional<size_t> Len() const override { return m_spec->len; }
    [[nodiscard]] bool Truth() const override { return m_spec->truth ? *m_spec->truth : IHostObject::Truth(); }
    [[nodiscard]] std::optional<bool> Equals(const Value& other) const override
    {
        m_spec->seen = { other };
        if (!m_spec->equalsInt || !other.IsInt())
        {
            return std::nullopt;
        }
        return other.AsInt() == *m_spec->equalsInt;
    }
    [[nodiscard]] std::optional<size_t> Hash() const override { return m_spec->hashable ? IHostObject::Hash() : std::nullopt; }
    // Every view of one spec is one object, as two proxies of one PyObject are
    [[nodiscard]] const void* Identity() const override { return m_spec.get(); }
    [[nodiscard]] bool IsMapping() const override { return m_spec->mapping; }
    [[nodiscard]] bool IsSequence() const override { return m_spec->listOrTuple; }
    [[nodiscard]] bool IsTuple() const override { return m_spec->tuple; }
    [[nodiscard]] bool IsMarkupLike() const override { return m_spec->html.has_value(); }
    [[nodiscard]] std::string Html() const override { return m_spec->html.value_or(""); }

private:
    template<typename Map, typename Key>
    static std::optional<Value> Find(const Map& map, const Key& key)
    {
        auto pos = map.find(key);
        return pos == map.end() ? std::nullopt : std::make_optional(pos->second);
    }

    std::shared_ptr<const ToySpec> m_spec;
};

Value MakeToy(const std::shared_ptr<const ToySpec>& spec)
{
    return MakeHostValue<ToyObject>(spec);
}

Value MakeToy(ToySpec spec = {})
{
    return MakeToy(std::make_shared<const ToySpec>(std::move(spec)));
}

struct Options
{
    UndefinedPolicy policy = UndefinedPolicy::Default;
    bool autoescape = false;
};

template<typename CharT>
std::basic_string<CharT> Render(const std::basic_string<CharT>& source, const ValuesMap& params, const Options& options = {})
{
    TemplateEnv env;
    env.GetSettings().undefinedPolicy = options.policy;
    env.GetSettings().autoescape = options.autoescape;
    env.GetSettings().extensions.doStatement = true;
    BasicTemplate<CharT> tpl(&env);
    auto loaded = tpl.Load(source);
    if (!loaded)
    {
        return ConvertString<std::basic_string<CharT>>(std::string("load error: ")) + loaded.error().ToString();
    }
    auto rendered = tpl.RenderAsString(params);
    if (!rendered)
    {
        return ConvertString<std::basic_string<CharT>>(std::string("error: ")) + rendered.error().ToString();
    }
    return *rendered;
}

std::string Render(const std::string& source, const ValuesMap& params, const Options& options = {})
{
    return Render<char>(source, params, options);
}

std::wstring RenderW(const std::wstring& source, const ValuesMap& params)
{
    return Render<wchar_t>(source, params);
}

ToySpec Rich()
{
    ToySpec spec;
    spec.attrs = { { "name", Value("attr-name") }, { "both", Value("attr-both") } };
    spec.items = { { "key", Value("item-key") }, { "both", Value("item-both") } };
    spec.intItems = { { 1, Value("item-1") } };
    return spec;
}

// A callable of the host that gives its first argument back
UserCallable Ident()
{
    return UserCallable([](const UserCallableParams& params) { return params["x"]; }, { ArgInfo("x", true) });
}

// A callable that hands every argument to `fn`
UserCallable Probe(std::function<Value(const UserCallableParams&)> fn)
{
    return UserCallable(std::move(fn), { ArgInfo("x", true) });
}

template<typename Fn>
bool Throws(const Fn& fn, ErrorCode code)
{
    try
    {
        fn();
    }
    catch (const ErrorInfo& error)
    {
        return error.GetCode() == code;
    }
    return false;
}
} // namespace

TEST(PublicHostObjectTest, StrPrintsAndReprInsideContainers)
{
    const ValuesMap params{ { "x", MakeToy() } };
    EXPECT_EQ("Тоу [<Toy>] Тоу|Тоу {'k': <Toy>} (<Toy>, 1)", Render("{{ x }} {{ [x] }} {{ x ~ '|' ~ x|string }} {{ {'k': x} }} {{ (x, 1) }}", params));
    EXPECT_EQ(L"Тоу [<Toy>] Тоу", RenderW(L"{{ x }} {{ [x] }} {{ x|string }}", params));
}

TEST(PublicHostObjectTest, AttributeBeforeItemAndItemBeforeAttribute)
{
    const ValuesMap params{ { "x", MakeToy(Rich()) } };
    // x.name is getattr, then getitem; x['name'] is getitem, then getattr, as in Jinja2
    EXPECT_EQ("attr-name item-key attr-both|item-key attr-name item-both|item-1|", Render("{{ x.name }} {{ x.key }} {{ x.both }}|{{ x['key'] }} {{ x['name'] }} {{ x['both'] }}|{{ x[1] }}|{{ x[2] }}", params));
    EXPECT_EQ("True False", Render("{{ x.missing is undefined }} {{ x.name is undefined }}", params));
    EXPECT_EQ("attr-both - attr-name", Render("{{ x|attr('both') }} {{ x|attr('key') or '-' }} {{ x|attr('name') }}", params));
    // No dict methods are added to a host object: a missing one is undefined
    EXPECT_EQ("True", Render("{{ x.items is undefined }}", params));
    EXPECT_EQ(L"attr-name item-key", RenderW(L"{{ x.name }} {{ x['key'] }}", params));

    // __getitem__ takes any key, hashable or not
    ToySpec typed;
    typed.typeKeys = true;
    const ValuesMap typedParams{ { "k", MakeToy(typed) } };
    EXPECT_EQ("list-key map-key list-key", Render("{{ k[[1]] }} {{ k[{}] }} {{ k[(1,)] }}", typedParams));
}

TEST(PublicHostObjectTest, SliceAsksTheObject)
{
    ToySpec sequence;
    sequence.sequence = std::vector<int64_t>{ 1, 2, 3 };
    auto spec = std::make_shared<const ToySpec>(sequence);
    const ValuesMap params{ { "x", MakeToy(spec) }, { "o", MakeToy(Rich()) } };
    EXPECT_EQ("[1, 2]|[2, 3]|[3, 2, 1]|[2]", Render("{{ x[0:2:1] }}|{{ x[1:] }}|{{ x[::-1] }}|{{ x[1:2] }}", params));
    // An omitted bound is None, not undefined
    Render("{{ x[1:] }}", params);
    ASSERT_EQ(3u, spec->seen.size());
    EXPECT_TRUE(spec->seen[0].IsInt());
    EXPECT_TRUE(spec->seen[1].IsNone());
    EXPECT_TRUE(spec->seen[2].IsNone());
    EXPECT_EQ(L"[2, 3]", RenderW(L"{{ x[1:] }}", params));
    // An object without items: Jinja2 passes the slice to __getitem__ without getitem's fallback
    EXPECT_NE(std::string::npos, Render("{{ o[1:2] }}", params).find("'Toy' object is not subscriptable"));
}

TEST(PublicHostObjectTest, TupleAndListPrintAndOperate)
{
    auto items = [](std::vector<Value> values, bool tuple) {
        ToySpec spec;
        spec.listOrTuple = true;
        spec.tuple = tuple;
        spec.len = values.size();
        spec.iteration = std::move(values);
        spec.indexable = true;
        return MakeToy(spec);
    };
    const ValuesMap params{ { "l", items({ Value(1) }, false) }, { "m", items({ Value(2) }, false) }, { "t", items({ Value(1), Value(2) }, true) }, { "u", items({ Value(1) }, true) } };
    EXPECT_EQ("[1, 2] [1, 3] (1, 2, 1, 2) (1, 1) True", Render("{{ l + m }} {{ l + [3] }} {{ t * 2 }} {{ 2 * u }} {{ l < m }}", params));
    EXPECT_EQ("[1, \"b\"]", Render("{{ [1, 'b']|tojson }}", {}));
    EXPECT_EQ("[1, 2]", Render("{{ t|tojson }}", params));
    EXPECT_EQ(L"(1, 2, 1, 2)", RenderW(L"{{ t * 2 }}", params));
}

TEST(PublicHostObjectTest, MarkupLikeObjectUnderAutoescape)
{
    ToySpec markupLike;
    markupLike.str = "<plain>";
    markupLike.html = "<b>Тоу</b>";
    ToySpec plain;
    plain.str = "<plain>";
    const ValuesMap params{ { "m", MakeToy(markupLike) }, { "p", MakeToy(plain) } };
    // str() without autoescape, __html__() with it, as markupsafe.escape has it
    EXPECT_EQ("<plain> <b>Тоу</b> &lt;plain&gt;|True False", Render("{{ m }} {% autoescape true %}{{ m }} {{ p }}{% endautoescape %}|{{ m is escaped }} {{ p is escaped }}", params));
    EXPECT_EQ("<b>Тоу</b> &lt;plain&gt;", Render("{{ m }} {{ p }}", params, { UndefinedPolicy::Default, true }));
    EXPECT_EQ(L"<b>Тоу</b> &lt;plain&gt;", RenderW(L"{% autoescape true %}{{ m }} {{ p }}{% endautoescape %}", params));
    EXPECT_EQ("<b>Тоу</b>|Тоу|&lt;b&gt;Тоу&lt;/b&gt;|<plain>", Render("{{ m|safe }}|{{ m|striptags }}|{{ m|forceescape }}|{{ m|string }}", params));
}

TEST(PublicHostObjectTest, IdentityHoldsAcrossCalls)
{
    auto spec = std::make_shared<const ToySpec>();
    const auto object = MakeToy(spec);
    std::vector<Value> kept;
    auto keep = Probe([&kept](const UserCallableParams& params) {
        kept.emplace_back(params["x"]);
        return params["x"];
    });
    const ValuesMap params{ { "ident", Ident() }, { "keep", std::move(keep) }, { "o", object }, { "o2", MakeToy(spec) }, { "other", MakeToy() } };
    // Two calls, each handing the object back: one key, one object
    EXPECT_EQ("True|1|True|True", Render("{% set k = ident(o) %}{% set k2 = ident(o) %}{{ k is sameas k2 }}|{{ {ident(o): 1}[o] }}|{{ ident(o) is sameas o }}|{{ ident(o) == o2 }}", params));
    EXPECT_EQ("False|2", Render("{{ ident(o) is sameas other }}|{{ {o: 1, other: 2, ident(other): 2}|length }}", params));
    // The value the host sees is the host's own object, and so is what it gives back
    Render("{{ keep(o) }}{{ keep(ident(o)) }}", params);
    ASSERT_EQ(2u, kept.size());
    const auto* map = kept[0].GetIf<GenericMap>();
    ASSERT_NE(nullptr, map);
    const auto original = object.GetIf<GenericMap>()->AsHostObject();
    ASSERT_NE(nullptr, original);
    EXPECT_EQ(original.get(), map->AsHostObject().get());
    EXPECT_EQ(original.get(), kept[1].GetIf<GenericMap>()->AsHostObject().get());
    EXPECT_EQ(*object.GetIf<GenericMap>(), *map);
    // An object with an identity of its own is the key
    EXPECT_EQ("2 one two|True False", Render("{% set d = {o: 'one', other: 'two'} %}{{ d|length }} {{ d[o2] }} {{ d[other] }}|{{ o in d }} {{ 1 in d }}", params));
    ToySpec unhashable;
    unhashable.hashable = false;
    EXPECT_NE(std::string::npos, Render("{{ {u: 1} }}", { { "u", MakeToy(unhashable) } }).find("unhashable type: 'Toy'"));
    // What an object saw can hold the object itself: a cycle the test breaks
    spec->seen.clear();
}

TEST(PublicHostObjectTest, MappingReadsAsAMap)
{
    ToySpec spec;
    spec.mapping = true;
    spec.iteration = std::vector<Value>{ Value("b"), Value("a") };
    spec.indexable = true;
    spec.items = { { "a", Value(1) }, { "b", Value("x") } };
    spec.attrs = { { "keys", Value("attr-keys") } };
    spec.contains = std::vector<std::string>{ "a", "b" };
    spec.len = 2;
    const ValuesMap params{ { "m", MakeToy(spec) }, { "o", MakeToy(Rich()) } };
    EXPECT_EQ("True False|[('a', 1), ('b', 'x')]|[('b', 'x'), ('a', 1)]|{\"a\": 1, \"b\": \"x\"}", Render("{{ m is mapping }} {{ o is mapping }}|{{ m|dictsort }}|{{ m|items|list }}|{{ m|tojson }}", params));
    EXPECT_EQ("True False attr-keys 1 ['b', 'a'] 2 Тоу", Render("{{ 'a' in m }} {{ 'keys' in m }} {{ m.keys }} {{ m.a }} {{ m|list }} {{ m|length }} {{ m }}", params));
    EXPECT_EQ(L"[('a', 1), ('b', 'x')]", RenderW(L"{{ m|dictsort }}", params));
    EXPECT_NE(std::string::npos, Render("{{ o|dictsort }}", params).find("'Toy' object has no attribute 'items'"));
    // A C++ reader of the value gets the keys and items as the template does
    const auto* map = params.at("m").GetIf<GenericMap>();
    ASSERT_NE(nullptr, map);
    const auto items = map->GetItems();
    ASSERT_EQ(2u, items.size());
    EXPECT_EQ("b", items[0].first.AsString());
    EXPECT_EQ("x", items[0].second.AsString());
    EXPECT_EQ("a", items[1].first.AsString());
}

TEST(PublicHostObjectTest, LengthTruthAndIteration)
{
    ToySpec sized;
    sized.len = 3;
    ToySpec empty;
    empty.len = 0;
    ToySpec falsy;
    falsy.truth = false;
    ToySpec iterator;
    iterator.iteration = std::vector<Value>{ Value(1), Value("b"), MakeToy() };
    const ValuesMap params{ { "s", MakeToy(sized) }, { "e", MakeToy(empty) }, { "f", MakeToy(falsy) }, { "o", MakeToy() }, { "it", MakeToy(iterator) } };
    EXPECT_EQ("3 0|yes no no yes|False", Render("{{ s|length }} {{ e|count }}|{{ s and 'yes' or 'no' }} {{ e and 'yes' or 'no' }} {{ 'yes' if f else 'no' }} {{ 'yes' if o else 'no' }}|{{ not o }}", params));
    EXPECT_NE(std::string::npos, Render("{{ o|length }}", params).find("object of type 'Toy' has no len()"));
    // A single-pass list stays single-pass: no length, one trip
    EXPECT_EQ("1,b,Тоу,|[1, 'b', <Toy>]|1-b-Тоу|True False", Render("{% for i in it %}{{ i }},{% endfor %}|{{ it|list }}|{{ it|join('-') }}|{{ it is iterable }} {{ o is iterable }}", params));
    EXPECT_EQ(L"1,b,Тоу,", RenderW(L"{% for i in it %}{{ i }},{% endfor %}", params));
    EXPECT_NE(std::string::npos, Render("{% for i in o %}{% endfor %}", params).find("'Toy' object is not iterable"));
    // A sequence has a length of its own and is read twice
    ToySpec sequence;
    sequence.iteration = std::vector<Value>{ Value(1), Value(2) };
    sequence.indexable = true;
    sequence.len = 2;
    EXPECT_EQ("2|1,2,|1,2,|2", Render("{{ q|length }}|{% for i in q %}{{ i }},{% endfor %}|{% for i in q %}{{ i }},{% endfor %}|{{ q|list|length }}", { { "q", MakeToy(sequence) } }));
}

namespace
{
// A list that is walked once: no size, no indexer, and its enumerator can neither be copied nor rewound
class OnePass final : public IListItemAccessor
{
public:
    OnePass(std::vector<Value> items, std::shared_ptr<int> moves)
        : m_items(std::move(items))
        , m_moves(std::move(moves))
    {
    }

    [[nodiscard]] const IIndexBasedAccessor* GetIndexer() const override { return nullptr; }
    [[nodiscard]] std::optional<ListEnumeratorPtr> CreateEnumerator() const override { return MakeEnumerator<Enumerator>(m_items, m_moves); }
    [[nodiscard]] std::optional<size_t> GetSize() const override { return std::nullopt; }

private:
    class Enumerator final : public IListEnumerator
    {
    public:
        Enumerator(std::vector<Value> items, std::shared_ptr<int> moves)
            : m_items(std::move(items))
            , m_moves(std::move(moves))
        {
        }
        void Reset() override { throw std::logic_error("a single-pass list was rewound"); }
        bool MoveNext() override
        {
            if (m_next == m_items.size())
            {
                return false;
            }
            ++*m_moves;
            ++m_next;
            return true;
        }
        [[nodiscard]] Value GetCurrent() const override { return m_items[m_next - 1]; }
        [[nodiscard]] ListEnumeratorPtr Clone() const override { throw std::logic_error("a single-pass list was copied"); }
        ListEnumeratorPtr Move() override { return MakeEnumerator<Enumerator>(std::move(m_items), m_moves); }

    private:
        std::vector<Value> m_items;
        std::shared_ptr<int> m_moves;
        size_t m_next = 0;
    };

    std::vector<Value> m_items;
    std::shared_ptr<int> m_moves;
};

class OnePassObject final : public IHostObject
{
public:
    explicit OnePassObject(std::shared_ptr<int> moves)
        : m_moves(std::move(moves))
    {
    }

    [[nodiscard]] const char* TypeName() const override { return "Gen"; }
    [[nodiscard]] std::string Str() const override { return "gen"; }
    [[nodiscard]] std::string Repr() const override { return "<gen>"; }
    [[nodiscard]] std::optional<Value> GetAttr(std::string_view /*name*/) const override { return std::nullopt; }
    [[nodiscard]] bool IsIterable() const override { return true; }
    [[nodiscard]] std::optional<GenericList> Iterate() const override
    {
        return GenericList(std::make_shared<const OnePass>(std::vector<Value>{ Value(1), Value(2), Value(3) }, m_moves));
    }

private:
    std::shared_ptr<int> m_moves;
};
} // namespace

TEST(PublicHostObjectTest, SinglePassIterationIsLazyAndNeverRewound)
{
    auto moves = std::make_shared<int>(0);
    const ValuesMap params{ { "it", MakeHostValue<OnePassObject>(moves) } };
    // Asking whether it iterates, and making the list, take no step
    EXPECT_EQ("True", Render("{{ it is iterable }}", params));
    EXPECT_EQ(0, *moves);
    EXPECT_EQ("1,2,3,", Render("{% for i in it %}{{ i }},{% endfor %}", params));
    EXPECT_EQ(3, *moves);
    *moves = 0;
    EXPECT_EQ("[1, 2, 3]|1-2-3|1|True", Render("{{ it|list }}|{{ it|join('-') }}|{{ it|first }}|{{ 2 in it }}", params));
    EXPECT_EQ(L"1,2,3,", RenderW(L"{% for i in it %}{{ i }},{% endfor %}", params));
}

TEST(PublicHostObjectTest, ContainsThenIterationAndReversed)
{
    ToySpec withContains;
    withContains.contains = std::vector<std::string>{ "a" };
    ToySpec withIteration;
    withIteration.iteration = std::vector<Value>{ Value(1), Value(2) };
    ToySpec sequence;
    sequence.sequence = std::vector<int64_t>{ 1, 2, 3 };
    const ValuesMap params{ { "c", MakeToy(withContains) }, { "i", MakeToy(withIteration) }, { "o", MakeToy() }, { "x", MakeToy(sequence) } };
    EXPECT_EQ("True False True False", Render("{{ 'a' in c }} {{ 'b' in c }} {{ 2 in i }} {{ 3 in i }}", params));
    EXPECT_NE(std::string::npos, Render("{{ 1 in o }}", params).find("argument of type 'Toy' is not iterable"));
    EXPECT_EQ("3|[3, 2, 1]", Render("{{ x|last }}|{{ x|reverse|list }}", params));
    EXPECT_NE(std::string::npos, Render("{{ o|last }}", params).find("'Toy' object is not reversible"));
}

TEST(PublicHostObjectTest, EqualityAsksEitherOperandThenIdentity)
{
    ToySpec equalsTwo;
    equalsTwo.equalsInt = 2;
    auto shared = std::make_shared<const ToySpec>();
    const ValuesMap params{ { "two", MakeToy(equalsTwo) }, { "a", MakeToy(shared) }, { "a2", MakeToy(shared) }, { "b", MakeToy() } };
    EXPECT_EQ("True True False True|True True False True|True True", Render("{{ two == 2 }} {{ 2 == two }} {{ two == 3 }} {{ two != 3 }}|{{ a == a }} {{ a == a2 }} {{ a == b }} {{ a != b }}|{{ a is sameas a2 }} {{ a == a2 and not (a is sameas b) }}", params));
    EXPECT_EQ("False False", Render("{{ a == 'Тоу' }} {{ a == none }}", params));
    EXPECT_NE(std::string::npos, Render("{{ a < 1 }}", params).find("'<' not supported between instances of 'Toy' and 'int'"));
    // The other operand reaches the object as the template's own value
    auto spec = std::make_shared<const ToySpec>();
    Render("{{ x == m }}", { { "x", MakeToy(spec) }, { "m", MakeToy(shared) } });
    ASSERT_EQ(1u, spec->seen.size());
    EXPECT_EQ(shared.get(), spec->seen[0].GetIf<GenericMap>()->AsHostObject()->Identity());
    // What an object saw can hold the object itself: a cycle the test breaks
    spec->seen.clear();
    shared->seen.clear();
}

TEST(PublicHostObjectTest, CallsWithPositionalAndKeywordArguments)
{
    ToySpec spec;
    spec.callable = true;
    spec.attrs = { { "method", MakeToy(spec) } };
    const ValuesMap params{ { "f", MakeToy(spec) }, { "o", MakeToy() } };
    EXPECT_EQ("called(2,k=v) called(0) True False", Render("{{ f(1, 2, k='v') }} {{ f.method() }} {{ f is callable }} {{ o is callable }}", params));
    EXPECT_EQ(L"called(1)", RenderW(L"{{ f(1) }}", params));
    EXPECT_NE(std::string::npos, Render("{{ o() }}", params).find("'Toy' object is not callable"));
}

TEST(PublicHostObjectTest, TemplateCallableIsLentForTheCall)
{
    auto spec = std::make_shared<ToySpec>();
    spec->callable = true;
    const ValuesMap params{ { "f", MakeToy(std::shared_ptr<const ToySpec>(spec)) } };
    EXPECT_EQ("called(1,k=<callable>)", Render("{% macro m() %}M{% endmacro %}{{ f(1, k=m) }}", params));
    // A call block passes caller=; the object called it during the call
    EXPECT_EQ("called(0,caller=<callable>)", Render("{% call f() %}body{% endcall %}", params));
    EXPECT_EQ("body", spec->keptResult);
    // Called after the call it was lent for, the macro has expired
    ASSERT_TRUE(spec->kept.has_value());
    UserCallableParams none;
    none.paramsParsed = true;
    EXPECT_TRUE(Throws([&] { spec->kept->callable(none); }, ErrorCode::TemplateExpired));
}

TEST(PublicHostObjectTest, UndefinedReachesTheObjectDescribed)
{
    auto spec = std::make_shared<ToySpec>();
    spec->callable = true;
    spec->attrs = { { "u", MakeUndefined({ UndefinedPolicy::Strict, "u", "Toy object", true, "" }) } };
    const ValuesMap params{ { "f", MakeToy(std::shared_ptr<const ToySpec>(spec)) } };

    for (const auto policy : { UndefinedPolicy::Default, UndefinedPolicy::Strict, UndefinedPolicy::Chainable, UndefinedPolicy::Debug })
    {
        Render("{{ f(nope) }}", params, { policy });
        ASSERT_EQ(1u, spec->seen.size());
        const auto described = UndefinedOf(spec->seen[0]);
        ASSERT_TRUE(described.has_value());
        EXPECT_EQ(policy, described.value().policy);
        EXPECT_EQ("nope", described.value().name);
        EXPECT_EQ("", described.value().objType);
        EXPECT_FALSE(UndefinedOf(Value(1)).has_value());
    }
    // A missing attribute and an item key
    Render("{{ f(f.missing) }}", params);
    auto described = UndefinedOf(spec->seen[0]);
    ASSERT_TRUE(described.has_value());
    EXPECT_EQ("missing", described.value().name);
    EXPECT_EQ("Toy object", described.value().objType);
    EXPECT_TRUE(described.value().isAttr);
    // An undefined key reaches GetItem described as well
    auto keyed = std::make_shared<const ToySpec>(Rich());
    Render("{{ k[nope] }}", { { "k", MakeToy(keyed) } });
    ASSERT_EQ(1u, keyed->seen.size());
    ASSERT_TRUE(UndefinedOf(keyed->seen[0]).has_value());
    EXPECT_EQ("nope", UndefinedOf(keyed->seen[0]).value().name);
}

TEST(PublicHostObjectTest, UndefinedComesBackAsUndefined)
{
    ToySpec spec;
    spec.attrs = { { "strict", MakeUndefined({ UndefinedPolicy::Strict, "strict", "", true, "" }) },
                   { "plain", MakeUndefined({ UndefinedPolicy::Default, "plain", "", true, "" }) },
                   { "hinted", MakeUndefined({ UndefinedPolicy::Strict, "hinted", "", true, "no such thing here" }) },
                   { "debug", MakeUndefined({ UndefinedPolicy::Debug, "debug", "", true, "" }) } };
    const ValuesMap params{ { "o", MakeToy(spec) } };
    EXPECT_EQ("True|True|[]|", Render("{{ o.plain is undefined }}|{{ o.strict is undefined }}|{{ [o.plain]|reject('undefined')|list }}|{{ o.plain }}", params));
    // The policy and the message are the description's
    EXPECT_NE(std::string::npos, Render("{{ o.strict }}", params).find("'strict' is undefined"));
    EXPECT_NE(std::string::npos, Render("{{ o.hinted }}", params).find("no such thing here"));
    EXPECT_EQ("{{ debug }}", Render("{{ o.debug }}", params));
    EXPECT_EQ("", Render("{{ o.plain }}", params));
    EXPECT_NE(std::string::npos, Render("{{ o.plain.x }}", params).find("'plain' is undefined"));

    // A host callable that returns it
    auto make = [](const UndefinedDescription& description) {
        return UserCallable([description](const UserCallableParams&) { return MakeUndefined(description); }, {});
    };
    const ValuesMap callables{ { "plain", make({ UndefinedPolicy::Default, "p", "", true, "" }) }, { "strict", make({ UndefinedPolicy::Strict, "s", "", true, "strict hint" }) } };
    EXPECT_EQ("True|True|", Render("{{ plain() is undefined }}|{{ strict() is undefined }}|{{ plain() }}", callables));
    const auto printed = Render("{{ strict() }}", callables);
    EXPECT_NE(std::string::npos, printed.find("strict hint"));
    EXPECT_EQ(0u, printed.find("error:"));
    EXPECT_NE(std::string::npos, Render("{{ strict() }}", callables, { UndefinedPolicy::Default }).find("strict hint"));
    // A description with no name or hint keeps its policy: strict still fails to print, debug prints
    const ValuesMap unnamed{ { "strict", make({ UndefinedPolicy::Strict, "", "", true, "" }) }, { "debug", make({ UndefinedPolicy::Debug, "", "", true, "" }) } };
    EXPECT_EQ(0u, Render("{{ strict() }}", unnamed).find("error:"));
    EXPECT_EQ("True", Render("{{ strict() is undefined }}", unnamed));
}

TEST(PublicHostObjectTest, KeepUndefinedGivesCallablesUndefined)
{
    std::vector<Value> seen;
    auto record = [&seen](const UserCallableParams& params) {
        seen.clear();
        seen.emplace_back(params["x"]);
        if (const auto* positional = params.extraPosArgs.GetIf<ValuesList>())
        {
            seen.insert(seen.end(), positional->begin(), positional->end());
        }
        if (const auto* keywords = params.extraKwArgs.GetIf<ValuesMap>())
        {
            for (const auto& [name, value] : *keywords)
            {
                seen.push_back(value);
            }
        }
        return Value(1);
    };
    UserCallable plain({ record }, { ArgInfo("x"), ArgInfo(ArgInfo::VarArgs), ArgInfo(ArgInfo::VarKwArgs) });
    UserCallable keeping = plain;
    keeping.keepUndefined = true;
    // A copy and a move carry the flag
    UserCallable moved{ UserCallable(keeping) };
    UserCallable assigned;
    assigned = keeping;
    EXPECT_TRUE(moved.keepUndefined);
    EXPECT_TRUE(assigned.keepUndefined);
    const ValuesMap params{ { "plain", plain }, { "keeping", keeping }, { "moved", moved }, { "assigned", assigned } };

    // Without the flag an undefined argument is None, as before
    Render("{{ plain(a, b, k=c) }}", params);
    ASSERT_EQ(3u, seen.size());
    for (const auto& value : seen)
    {
        EXPECT_TRUE(value.IsNone());
        EXPECT_FALSE(UndefinedOf(value).has_value());
    }
    for (const char* name : { "keeping", "moved", "assigned" })
    {
        Render(std::string("{{ ") + name + "(a, b, k=c) }}", params);
        ASSERT_EQ(3u, seen.size()) << name;
        const std::vector<std::string> expected{ "a", "b", "c" };
        for (size_t i = 0; i != seen.size(); ++i)
        {
            const auto described = UndefinedOf(seen[i]);
            ASSERT_TRUE(described.has_value()) << name << i;
            EXPECT_EQ(expected[i], described.value().name);
        }
    }
    // A defined value is untouched, a missing attribute is described as one
    Render("{{ keeping(1, {}.nope, k=[]) }}", params);
    ASSERT_EQ(3u, seen.size());
    EXPECT_TRUE(seen[0].IsInt());
    const auto described = UndefinedOf(seen[1]);
    ASSERT_TRUE(described.has_value());
    EXPECT_EQ("nope", described.value().name);
    EXPECT_EQ("dict object", described.value().objType);
    EXPECT_TRUE(seen[2].IsList());
    // And it goes back: the template sees undefined, with the policy it had
    // Without the flag it comes back as None
    EXPECT_EQ("False", Render("{{ echo(a) is undefined }}", { { "echo", UserCallable([](const UserCallableParams& p) { return p["x"]; }, { ArgInfo("x") }) } }));
    UserCallable echo([](const UserCallableParams& p) { return p["x"]; }, { ArgInfo("x") });
    echo.keepUndefined = true;
    const auto text = Render("{{ echo(nope) is undefined }} {{ echo(nope) }}", { { "echo", echo } }, { UndefinedPolicy::Strict });
    EXPECT_EQ(0u, text.find("error:"));
    EXPECT_NE(std::string::npos, text.find("'nope' is undefined"));
    EXPECT_EQ("True ", Render("{{ echo(nope) is undefined }} {{ echo(nope) }}", { { "echo", echo } }));
}

TEST(PublicHostObjectTest, ExportedEngineValuesKeepTupleIdentityAndKeys)
{
    std::vector<Value> seen;
    auto probe = Probe([&seen](const UserCallableParams& params) {
        seen.emplace_back(params["x"]);
        return Value(1);
    });
    const ValuesMap params{ { "probe", std::move(probe) } };

    // 7a: a tuple is a tuple, nested too, and a list is not
    Render("{{ probe((1, [2, (3,)])) }}{{ probe([1, 2]) }}{{ probe(1, 2) }}", params);
    ASSERT_GE(seen.size(), 2u);
    const auto* tuple = seen[0].GetIf<GenericList>();
    ASSERT_NE(nullptr, tuple);
    EXPECT_TRUE(tuple->IsTuple());
    ASSERT_EQ(2u, tuple->GetSize().value_or(0));
    std::vector<Value> inner;
    for (auto it = tuple->begin(); it != tuple->end(); ++it)
    {
        inner.push_back(*it);
    }
    ASSERT_EQ(2u, inner.size());
    const auto* nestedList = inner[1].GetIf<GenericList>();
    ASSERT_NE(nullptr, nestedList);
    EXPECT_FALSE(nestedList->IsTuple());
    std::vector<Value> nested;
    for (auto it = nestedList->begin(); it != nestedList->end(); ++it)
    {
        nested.push_back(*it);
    }
    ASSERT_EQ(2u, nested.size());
    ASSERT_NE(nullptr, nested[1].GetIf<GenericList>());
    EXPECT_TRUE(nested[1].GetIf<GenericList>()->IsTuple());
    EXPECT_FALSE(seen[1].GetIf<GenericList>()->IsTuple());

    // 7c: the keys of a dict are its own, not strings that spell alike
    seen.clear();
    Render("{{ probe({1: 'a', '1': 'b', (1, 2): 'c'}) }}", params);
    ASSERT_EQ(1u, seen.size());
    const auto* dict = seen[0].GetIf<GenericMap>();
    ASSERT_NE(nullptr, dict);
    const auto items = dict->GetItems();
    ASSERT_EQ(3u, items.size());
    EXPECT_TRUE(items[0].first.IsInt());
    EXPECT_EQ(1, items[0].first.AsInt());
    EXPECT_EQ("a", items[0].second.AsString());
    EXPECT_TRUE(items[1].first.IsString());
    EXPECT_EQ("1", items[1].first.AsString());
    EXPECT_EQ("b", items[1].second.AsString());
    ASSERT_NE(nullptr, items[2].first.GetIf<GenericList>());
    EXPECT_TRUE(items[2].first.GetIf<GenericList>()->IsTuple());
    // A dict with string keys is unchanged
    seen.clear();
    Render("{{ probe({'a': 1, 'b': 2}) }}", params);
    const auto plain = seen[0].GetIf<GenericMap>()->GetItems();
    ASSERT_EQ(2u, plain.size());
    EXPECT_EQ("a", plain[0].first.AsString());
    EXPECT_EQ(2, plain[1].second.AsInt());
}

namespace
{
// Walks a value the way a binding converts one, stopping at a list or dict it has seen: the memo is the identity
struct Walker
{
    std::set<const void*> seen;
    int visited = 0;
    int repeats = 0;

    void Walk(const Value& value)
    {
        if (const auto* list = value.GetIf<GenericList>())
        {
            if (!seen.insert(list->GetIdentity()).second)
            {
                ++repeats;
                return;
            }
            for (auto it = list->begin(); it != list->end(); ++it)
            {
                Walk(*it);
            }
        }
        else if (const auto* map = value.GetIf<GenericMap>())
        {
            if (!seen.insert(map->GetIdentity()).second)
            {
                ++repeats;
                return;
            }
            for (const auto& [key, item] : map->GetItems())
            {
                Walk(item);
            }
        }
        ++visited;
    }
};
} // namespace

TEST(PublicHostObjectTest, ExportedEngineContainersHaveAStableIdentity)
{
    Walker walker;
    auto walk = Probe([&walker](const UserCallableParams& params) {
        walker.Walk(params["x"]);
        return Value(walker.repeats);
    });
    std::vector<Value> seen;
    auto probe = Probe([&seen](const UserCallableParams& params) {
        seen.emplace_back(params["x"]);
        return Value(1);
    });
    const ValuesMap params{ { "walk", std::move(walk) }, { "probe", std::move(probe) } };
    // A list that contains itself ends the walk on the repeated identity
    EXPECT_EQ("1", Render("{% set xs = [] %}{% do xs.append(xs) %}{{ walk(xs) }}", params));
    EXPECT_EQ(1, walker.repeats);
    EXPECT_EQ(0, walker.visited - 1);
    walker = {};
    EXPECT_EQ("1", Render("{% set d = {} %}{% do d.update(self=d) %}{{ walk(d) }}", params));
    walker = {};
    EXPECT_EQ("1", Render("{% set d = {'k': []} %}{% do d['k'].append(d) %}{{ walk(d) }}", params));
    // A dict with a key that is not a string goes out as a copy (CreateSnapshot), so one that
    // holds itself is RecursionError, never a stack overflow
    for (const std::string source : { "{% set d = {1: 'a'} %}{% do d.update(self=d) %}{{ walk(d) }}", "{% set d = {1: 'a'} %}{% do d.update({(1, 2): d}) %}{{ walk(d) }}" })
    {
        walker = {};
        const auto result = Render(source, params);
        EXPECT_NE(std::string::npos, result.find("Maximum recursion depth exceeded")) << result;
    }

    // Two exports of one list, one dict, one tuple: one identity; another list another
    Render("{% set xs = [1] %}{% set d = {'a': 1} %}{% set t = (1,) %}{{ probe(xs) }}{{ probe(xs) }}{{ probe(d) }}{{ probe(d) }}{{ probe(t) }}{{ probe(t) }}{{ probe([1]) }}", params);
    ASSERT_EQ(7u, seen.size());
    EXPECT_EQ(seen[0].GetIf<GenericList>()->GetIdentity(), seen[1].GetIf<GenericList>()->GetIdentity());
    EXPECT_EQ(seen[2].GetIf<GenericMap>()->GetIdentity(), seen[3].GetIf<GenericMap>()->GetIdentity());
    EXPECT_EQ(seen[4].GetIf<GenericList>()->GetIdentity(), seen[5].GetIf<GenericList>()->GetIdentity());
    EXPECT_NE(seen[0].GetIf<GenericList>()->GetIdentity(), seen[6].GetIf<GenericList>()->GetIdentity());
    EXPECT_NE(nullptr, seen[0].GetIf<GenericList>()->GetIdentity());

    // A dict with a key that is not a string goes out as a copy; the copy keeps the dict's storage, so a later dict
    // never shares the identity of one a host still holds
    seen.clear();
    Render("{{ probe({1: 'a'}) }}", params);
    Render("{{ probe({2: 'b'}) }}", params);
    ASSERT_EQ(2u, seen.size());
    EXPECT_NE(seen[0].GetIf<GenericMap>()->GetIdentity(), seen[1].GetIf<GenericMap>()->GetIdentity());
}

TEST(PublicHostObjectTest, MembersOfTemplateObjectsAreLentForTheCall)
{
    std::vector<Value> kept;
    std::vector<std::string> results;
    auto call = [&](const Value& callable, ValuesList args) {
        const auto* fn = callable.GetIf<UserCallable>();
        if (!fn)
        {
            results.emplace_back("<none>");
            return;
        }
        UserCallableParams params;
        params.paramsParsed = true;
        params.extraPosArgs = Value(std::move(args));
        params.extraKwArgs = Value(ValuesMap());
        results.push_back(ToString(fn->callable(params)));
        kept.push_back(callable);
    };
    auto read = Probe([&](const UserCallableParams& params) {
        const auto& x = params["x"];
        if (const auto* map = x.GetIf<GenericMap>())
        {
            if (map->HasValue("cycle"))
            {
                call(map->GetValueByName("cycle"), { Value("a"), Value("b") });
            }
            for (const auto& [keyValue, value] : map->GetItems())
            {
                const auto key = keyValue.IsString() ? keyValue.AsString() : std::string();
                if (value.IsCallable() && (key == "m" || key == "fn" || key == "next"))
                {
                    call(value, {});
                }
            }
        }
        else if (const auto* list = x.GetIf<GenericList>())
        {
            for (auto it = list->begin(); it != list->end(); ++it)
            {
                call(*it, {});
            }
        }
        return Value("ok");
    });
    const ValuesMap params{ { "read", std::move(read) } };
    const std::string macro = "{% macro m() %}M{% endmacro %}";

    // A macro in a dict, in a list, in a tuple, in a namespace, in a nested container
    EXPECT_EQ("ok", Render(macro + "{{ read({'m': m}) }}", params));
    EXPECT_EQ(std::vector<std::string>({ "M" }), results);
    results.clear();
    EXPECT_EQ("ok", Render(macro + "{{ read([m, m]) }}", params));
    EXPECT_EQ(std::vector<std::string>({ "M", "M" }), results);
    results.clear();
    EXPECT_EQ("ok", Render(macro + "{{ read((m, 1)) }}", params));
    EXPECT_EQ(std::vector<std::string>({ "M", "<none>" }), results);
    results.clear();
    EXPECT_EQ("ok", Render(macro + "{% set ns = namespace(fn=m) %}{{ read(ns) }}", params));
    EXPECT_EQ(std::vector<std::string>({ "M" }), results);
    results.clear();
    EXPECT_EQ("ok", Render(macro + "{{ read([{'k': m}]) }}", params));
    EXPECT_EQ(std::vector<std::string>({ "<none>" }), results);
    results.clear();
    // loop.cycle and a cycler's next
    EXPECT_EQ("okok", Render("{% for i in [1, 2] %}{{ read(loop) }}{% endfor %}", params));
    EXPECT_EQ(std::vector<std::string>({ "a", "b" }), results);
    results.clear();
    EXPECT_EQ("ok", Render("{% set c = cycler('x', 'y') %}{{ read(c) }}", params));
    EXPECT_EQ(std::vector<std::string>({ "x" }), results);
    // Called after the call they were lent for, they have expired
    ASSERT_FALSE(kept.empty());
    for (const auto& callable : kept)
    {
        UserCallableParams none;
        none.paramsParsed = true;
        EXPECT_TRUE(Throws([&] { callable.GetIf<UserCallable>()->callable(none); }, ErrorCode::TemplateExpired));
    }
}

namespace
{
// An object that counts its constructions and destructions, to see when the value that carries it lets go of it
struct LifeCount
{
    int alive = 0;
    int made = 0;
};

class Counted final : public IHostObject
{
public:
    Counted(LifeCount& count, std::string name)
        : m_count(&count)
        , m_name(std::move(name))
    {
        ++m_count->alive;
        ++m_count->made;
    }
    Counted(const Counted&) = delete;
    Counted& operator=(const Counted&) = delete;
    ~Counted() override { --m_count->alive; }

    [[nodiscard]] const char* TypeName() const override { return "Counted"; }
    [[nodiscard]] std::string Str() const override { return m_name; }
    [[nodiscard]] std::string Repr() const override { return "<" + m_name + ">"; }
    [[nodiscard]] std::optional<Value> GetAttr(std::string_view name) const override { return name == "name" ? std::optional<Value>(Value(m_name)) : std::nullopt; }
    [[nodiscard]] bool IsCallable() const override { return true; }
    [[nodiscard]] Value Call(const UserCallableParams& params) const override { return Value(m_name + "!" + AsString(params.extraPosArgs.GetIf<ValuesList>()->size())); }

private:
    static std::string AsString(size_t n) { return std::to_string(n); }

    LifeCount* m_count;
    std::string m_name;
};

// More aligned than `new` guarantees: made in its own allocation, as the library cannot place it in a block of its own
class alignas(64) WideCounted final : public IHostObject
{
public:
    explicit WideCounted(LifeCount& count)
        : m_count(&count)
    {
        ++m_count->alive;
    }
    WideCounted(const WideCounted&) = delete;
    WideCounted& operator=(const WideCounted&) = delete;
    ~WideCounted() override { --m_count->alive; }

    [[nodiscard]] const char* TypeName() const override { return "WideCounted"; }
    [[nodiscard]] std::string Str() const override { return std::to_string(reinterpret_cast<uintptr_t>(this) % 64); }
    [[nodiscard]] std::string Repr() const override { return Str(); }
    [[nodiscard]] std::optional<Value> GetAttr(std::string_view) const override { return std::nullopt; }

private:
    LifeCount* m_count;
};

class ThrowsOnConstruction final : public IHostObject
{
public:
    explicit ThrowsOnConstruction(LifeCount& count)
    {
        ++count.made;
        throw std::runtime_error("no object");
    }
    [[nodiscard]] const char* TypeName() const override { return "ThrowsOnConstruction"; }
    [[nodiscard]] std::string Str() const override { return {}; }
    [[nodiscard]] std::string Repr() const override { return {}; }
    [[nodiscard]] std::optional<Value> GetAttr(std::string_view) const override { return std::nullopt; }
};
} // namespace

TEST(PublicHostObjectTest, ValueMadeInPlaceOwnsItsObjectUntilTheLastReference)
{
    LifeCount count;
    {
        Value first = MakeHostValue<Counted>(count, "obj");
        EXPECT_EQ(1, count.alive);
        {
            // A copy shares the object; a render's own reference to it ends with the render
            Value second = first;
            const ValuesMap params{ { "o", second } };
            EXPECT_EQ("obj|obj|[<obj>]|obj!2", Render("{{ o }}|{{ o.name }}|{{ [o] }}|{{ o(1, 2) }}", params));
            // The map hands out the object, which keeps the value alive
            const auto object = first.GetIf<GenericMap>()->AsHostObject();
            ASSERT_NE(nullptr, object);
            second = Value();
            first = Value();
            EXPECT_EQ(1, count.alive);
            EXPECT_STREQ("Counted", object->TypeName());
        }
        EXPECT_EQ(0, count.alive);
    }
    EXPECT_EQ(1, count.made);
}

TEST(PublicHostObjectTest, ValueMadeInPlaceSurvivesTheRenderAndComesBackAsItself)
{
    LifeCount count;
    Value kept;
    auto keep = Probe([&kept](const UserCallableParams& params) {
        kept = params["x"];
        return params["x"];
    });
    {
        const ValuesMap params{ { "keep", std::move(keep) }, { "o", MakeHostValue<Counted>(count, "obj") } };
        EXPECT_EQ("obj|True", Render("{{ keep(o) }}|{{ keep(o) is sameas o }}", params));
        EXPECT_EQ(1, count.alive);
    }
    // The params are gone; the host still holds what the call was handed
    ASSERT_NE(nullptr, kept.GetIf<GenericMap>());
    EXPECT_EQ(1, count.alive);
    EXPECT_EQ("obj", Render("{{ o }}", { { "o", kept } }));
    kept = Value();
    EXPECT_EQ(0, count.alive);
}

TEST(PublicHostObjectTest, ValueMadeInPlaceHandlesOverAlignedAndFailingObjects)
{
    LifeCount count;
    {
        const Value wide = MakeHostValue<WideCounted>(count);
        EXPECT_EQ(1, count.alive);
        EXPECT_EQ("0", Render("{{ o }}", { { "o", wide } }));
    }
    EXPECT_EQ(0, count.alive);
    EXPECT_THROW(MakeHostValue<ThrowsOnConstruction>(count), std::runtime_error);
    EXPECT_EQ(1, count.made);
}

TEST(PublicHostObjectTest, ValueMadeInPlaceLivesAcrossThreads)
{
    LifeCount count;
    Value value = MakeHostValue<Counted>(count, "obj");
    std::vector<std::thread> threads;
    threads.reserve(4);
    std::atomic<int> rendered{ 0 };
    for (int i = 0; i < 4; ++i)
    {
        threads.emplace_back([&value, &rendered] {
            for (int n = 0; n < 200; ++n)
            {
                Value copy = value;
                const ValuesMap params{ { "o", copy } };
                if (Render("{{ o.name }}", params) == "obj")
                {
                    ++rendered;
                }
            }
        });
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
    EXPECT_EQ(800, rendered.load());
    EXPECT_EQ(1, count.alive);
    value = Value();
    EXPECT_EQ(0, count.alive);
}

namespace
{
// A host list with attributes and string keys, which the template can shorten while it loops (`s.pop()`), for the fast
// reads (GetAttrTo, GetItemTo, ItemAtTo) against the D2a forms they default to
struct SeqState
{
    std::vector<Value> items;
    std::map<std::string, Value> attrs;
    std::map<std::string, Value> keyed;
    // The last key GetItemTo saw, bytes as given
    std::string lastKey;
    // How many reads the fast forms answered
    int fastReads = 0;
    // What Len adds to the number of items: a length that runs ahead of them
    size_t extraLen = 0;
};

// The items as the host has them now: the size is read at each step
class LiveList final : public IListItemAccessor
    , private IIndexBasedAccessor
{
public:
    explicit LiveList(std::shared_ptr<SeqState> state)
        : m_state(std::move(state))
    {
    }

    [[nodiscard]] const IIndexBasedAccessor* GetIndexer() const override { return this; }
    [[nodiscard]] std::optional<ListEnumeratorPtr> CreateEnumerator() const override { return MakeEnumerator<Enumerator>(m_state); }
    [[nodiscard]] std::optional<size_t> GetSize() const override { return m_state->items.size(); }

private:
    class Enumerator final : public IListEnumerator
    {
    public:
        explicit Enumerator(std::shared_ptr<SeqState> state)
            : m_state(std::move(state))
        {
        }
        void Reset() override { m_next = 0; }
        bool MoveNext() override
        {
            if (m_next >= m_state->items.size())
            {
                return false;
            }
            m_current = m_next++;
            return true;
        }
        [[nodiscard]] Value GetCurrent() const override { return m_current < m_state->items.size() ? m_state->items[m_current] : Value(); }
        [[nodiscard]] ListEnumeratorPtr Clone() const override { return MakeEnumerator<Enumerator>(*this); }
        ListEnumeratorPtr Move() override { return MakeEnumerator<Enumerator>(*this); }

    private:
        std::shared_ptr<SeqState> m_state;
        size_t m_next = 0;
        size_t m_current = 0;
    };

    [[nodiscard]] Value GetItemByIndex(int64_t idx) const override { return m_state->items[static_cast<size_t>(idx)]; }

    std::shared_ptr<SeqState> m_state;
};

// `Fast` overrides the three fast reads, writing scalars and text straight into the result; the other answers the D2a
// forms only
template<bool Fast>
class SeqObject final : public IHostObject
{
public:
    explicit SeqObject(std::shared_ptr<SeqState> state)
        : m_state(std::move(state))
    {
    }

    [[nodiscard]] const char* TypeName() const override { return "Seq"; }
    [[nodiscard]] std::string Str() const override { return "seq"; }
    [[nodiscard]] std::string Repr() const override { return "<Seq>"; }
    [[nodiscard]] std::optional<Value> GetAttr(std::string_view name) const override
    {
        if (name == "pop")
        {
            auto state = m_state;
            return Value(UserCallable(
                [state](const UserCallableParams& /*params*/) {
                    Value last = state->items.back();
                    state->items.pop_back();
                    return last;
                },
                {}));
        }
        return Find(m_state->attrs, std::string(name));
    }
    [[nodiscard]] std::optional<Value> GetItem(const Value& key) const override
    {
        if (key.IsString())
        {
            m_state->lastKey = key.AsString();
            return Find(m_state->keyed, key.AsString());
        }
        if (key.IsInt() && key.AsInt() >= 0 && static_cast<size_t>(key.AsInt()) < m_state->items.size())
        {
            return m_state->items[static_cast<size_t>(key.AsInt())];
        }
        return std::nullopt;
    }
    [[nodiscard]] bool GetAttrTo(std::string_view name, HostResult& out) const override
    {
        if constexpr (!Fast)
        {
            return IHostObject::GetAttrTo(name, out);
        }
        if (name == "pop")
        {
            return IHostObject::GetAttrTo(name, out);
        }
        const auto pos = m_state->attrs.find(std::string(name));
        return pos != m_state->attrs.end() && Write(pos->second, out);
    }
    [[nodiscard]] bool GetItemTo(std::string_view key, HostResult& out) const override
    {
        if constexpr (!Fast)
        {
            return IHostObject::GetItemTo(key, out);
        }
        m_state->lastKey = std::string(key);
        const auto pos = m_state->keyed.find(m_state->lastKey);
        return pos != m_state->keyed.end() && Write(pos->second, out);
    }
    [[nodiscard]] bool ItemAtTo(size_t index, HostResult& out) const override
    {
        if constexpr (!Fast)
        {
            return IHostObject::ItemAtTo(index, out);
        }
        return index < m_state->items.size() && Write(m_state->items[index], out);
    }
    [[nodiscard]] bool IsIterable() const override { return true; }
    [[nodiscard]] std::optional<GenericList> Iterate() const override { return GenericList(std::make_shared<const LiveList>(m_state)); }
    [[nodiscard]] std::optional<size_t> Len() const override { return m_state->items.size() + m_state->extraLen; }
    [[nodiscard]] bool IsSequence() const override { return true; }
    [[nodiscard]] const void* Identity() const override { return m_state.get(); }

private:
    bool Write(const Value& value, HostResult& out) const
    {
        ++m_state->fastReads;
        if (value.IsNone())
        {
            out.SetNone();
        }
        else if (const auto* flag = value.GetIf<bool>())
        {
            out.Set(*flag);
        }
        else if (const auto* number = value.GetIf<int64_t>())
        {
            out.Set(*number);
        }
        else if (const auto* real = value.GetIf<double>())
        {
            out.Set(*real);
        }
        else if (value.IsMarkup())
        {
            out.SetMarkup(value.AsString());
        }
        else if (const auto* text = value.GetIf<std::string>())
        {
            out.SetString(*text);
        }
        else
        {
            out.Set(value);
        }
        return true;
    }
    static std::optional<Value> Find(const std::map<std::string, Value>& map, const std::string& key)
    {
        auto pos = map.find(key);
        return pos == map.end() ? std::nullopt : std::make_optional(pos->second);
    }

    std::shared_ptr<SeqState> m_state;
};

std::shared_ptr<SeqState> MakeSeqState()
{
    auto state = std::make_shared<SeqState>();
    state->items = { Value(1), Value("b"), Value(2.5), Value(true), Value(), Value(Markup("<i>")), MakeToy(Rich()) };
    state->attrs = { { "name", Value("attr-name") }, { "both", Value("attr-both") }, { "n", Value(int64_t{ 7 }) }, { "f", Value(0.5) }, { "flag", Value(false) }, { "none", Value() }, { "html", Value(Markup("<b>")) }, { "toy", MakeToy(Rich()) } };
    state->keyed = { { "key", Value("item-key") }, { "both", Value("item-both") }, { std::string("a\0b", 3), Value("nul-key") } };
    return state;
}
} // namespace

TEST(PublicHostObjectTest, FastReadsRenderAsTheValueForms)
{
    const std::string source = "{{ s.name }}|{{ s.both }}|{{ s['both'] }}|{{ s.key }}|{{ s['name'] }}|{{ s.n + 1 }}|{{ s.f }}|{{ s.flag }}|{{ s.none }}|{{ s.none is none }}"
                               "|{{ s.missing is defined }}|{{ s['missing'] is defined }}|{{ s.toy.name }}|{{ s.html }}"
                               "|{% for i in s %}{{ i }},{% endfor %}|{{ s|list }}|{{ s|join('-') }}|{{ s|length }}|{{ s|first }}|{{ s[1] }}"
                               "|{% for i in s %}{{ loop.index }}/{{ loop.length }}{{ '' if loop.last else ' ' }}{% endfor %}|{{ s|select('number')|list }}";
    auto slow = MakeSeqState();
    auto fast = MakeSeqState();
    const auto slowText = Render(source, { { "s", MakeHostValue<SeqObject<false>>(slow) } }, { UndefinedPolicy::Default, true });
    const auto fastText = Render(source, { { "s", MakeHostValue<SeqObject<true>>(fast) } }, { UndefinedPolicy::Default, true });
    EXPECT_EQ(slowText, fastText);
    EXPECT_EQ("attr-name|attr-both|item-both|item-key|attr-name|8|0.5|False|None|True|False|False|attr-name|<b>"
              "|1,b,2.5,True,None,<i>,Тоу,|[1, &#39;b&#39;, 2.5, True, None, Markup(&#39;&lt;i&gt;&#39;), &lt;Toy&gt;]|1-b-2.5-True-None-<i>-Тоу|7|1|b"
              "|1/7 2/7 3/7 4/7 5/7 6/7 7/7|[1, 2.5, True]",
              fastText);
    EXPECT_EQ(0, slow->fastReads);
    EXPECT_LT(0, fast->fastReads);
    const std::wstring wide = L"{{ s.name }}|{{ s['key'] }}|{% for i in s %}{{ i }},{% endfor %}";
    EXPECT_EQ(Render<wchar_t>(wide, { { "s", MakeHostValue<SeqObject<false>>(MakeSeqState()) } }), Render<wchar_t>(wide, { { "s", MakeHostValue<SeqObject<true>>(MakeSeqState()) } }));
}

TEST(PublicHostObjectTest, FastItemReadKeepsAnEmbeddedNul)
{
    for (const bool fast : { false, true })
    {
        auto state = MakeSeqState();
        const Value s = fast ? MakeHostValue<SeqObject<true>>(state) : MakeHostValue<SeqObject<false>>(state);
        const ValuesMap params{ { "s", s }, { "k", Value(std::string("a\0b", 3)) } };
        EXPECT_EQ("nul-key|False", Render("{{ s[k] }}|{{ s['a'] is defined }}", params)) << fast;
        EXPECT_EQ(std::string("a", 1), state->lastKey) << fast;
        EXPECT_EQ("nul-key", Render("{{ s[k] }}", params)) << fast;
        EXPECT_EQ(std::string("a\0b", 3), state->lastKey) << fast;
    }
}

TEST(PublicHostObjectTest, FastSequenceReadFollowsTheLiveLength)
{
    for (const bool fast : { false, true })
    {
        auto state = MakeSeqState();
        state->items = { Value(1), Value(2), Value(3), Value(4) };
        const Value s = fast ? MakeHostValue<SeqObject<true>>(state) : MakeHostValue<SeqObject<false>>(state);
        // Python: the list iterator stops at the live end, [1, 2]
        EXPECT_EQ("1,2,|[1, 2]", Render("{% for i in s %}{{ i }},{% do s.pop() %}{% endfor %}|{{ s|list }}", { { "s", s } })) << fast;
        // Emptied in the loop's first step
        state->items = { Value(1), Value(2) };
        EXPECT_EQ("1;|0", Render("{% for i in s %}{{ i }};{% do s.pop() %}{% do s.pop() %}{% endfor %}|{{ s|length }}", { { "s", s } })) << fast;
        EXPECT_EQ("|", Render("{% for i in s %}{{ i }}{% endfor %}|{{ s|join }}", { { "s", s } })) << fast;
    }
    // An object whose length is ahead of its items: the read past the end is false, and the step is undefined
    for (const bool fast : { false, true })
    {
        auto state = MakeSeqState();
        state->items = { Value(1) };
        state->extraLen = 1;
        const Value s = fast ? MakeHostValue<SeqObject<true>>(state) : MakeHostValue<SeqObject<false>>(state);
        EXPECT_EQ("1,False,|1", Render("{% for i in s %}{{ i if i is defined else (i is defined) }},{% endfor %}|{{ s[0] }}{{ s[5] }}", { { "s", s } })) << fast;
    }
}

namespace
{
// A tree of the host whose members are host objects of their own, handed to the engine through
// HostResult::SetHostObject: `first` and each item are a new view made in place per read, whose Identity is the node it
// shows; `shared` is one object the root keeps (the shared_ptr form); `wide` is over-aligned, `gone` undefined
struct Node
{
    std::string name;
    std::vector<std::shared_ptr<const Node>> children;
};

class NodeView final : public IHostObject
{
public:
    NodeView(LifeCount& count, std::shared_ptr<const Node> node, std::shared_ptr<const IHostObject> shared = nullptr)
        : m_count(&count)
        , m_node(std::move(node))
        , m_shared(std::move(shared))
    {
        ++m_count->alive;
        ++m_count->made;
    }
    NodeView(const NodeView&) = delete;
    NodeView& operator=(const NodeView&) = delete;
    ~NodeView() override { --m_count->alive; }

    [[nodiscard]] const char* TypeName() const override { return "Node"; }
    [[nodiscard]] std::string Str() const override { return m_node->name; }
    [[nodiscard]] std::string Repr() const override { return "<" + m_node->name + ">"; }
    [[nodiscard]] std::optional<Value> GetAttr(std::string_view /*name*/) const override { return std::nullopt; }
    [[nodiscard]] bool GetAttrTo(std::string_view name, HostResult& out) const override
    {
        if (name == "name")
        {
            out.SetString(m_node->name);
            return true;
        }
        if (name == "first" && !m_node->children.empty())
        {
            out.SetHostObject<NodeView>(*m_count, m_node->children.front());
            return true;
        }
        if (name == "shared" && m_shared)
        {
            out.SetHostObject(m_shared);
            return true;
        }
        if (name == "wide")
        {
            out.SetHostObject<WideCounted>(*m_count);
            return true;
        }
        if (name == "gone")
        {
            out.SetHostObject(MakeUndefined({ UndefinedPolicy::Default, "gone", "Node", true, {} }).GetIf<GenericMap>()->AsHostObject());
            return true;
        }
        if (name == "null")
        {
            out.SetHostObject(std::shared_ptr<const IHostObject>());
            return true;
        }
        return false;
    }
    [[nodiscard]] bool ItemAtTo(size_t index, HostResult& out) const override
    {
        if (index >= m_node->children.size())
        {
            return false;
        }
        out.SetHostObject<NodeView>(*m_count, m_node->children[index]);
        return true;
    }
    [[nodiscard]] std::optional<size_t> Len() const override { return m_node->children.size(); }
    [[nodiscard]] bool IsIterable() const override { return true; }
    [[nodiscard]] bool IsSequence() const override { return true; }
    [[nodiscard]] const void* Identity() const override { return m_node.get(); }

private:
    LifeCount* m_count;
    std::shared_ptr<const Node> m_node;
    std::shared_ptr<const IHostObject> m_shared;
};

std::shared_ptr<const Node> MakeTree()
{
    auto leaf = std::make_shared<Node>(Node{ "leaf", {} });
    auto a = std::make_shared<Node>(Node{ "a", { leaf } });
    auto b = std::make_shared<Node>(Node{ "b", {} });
    return std::make_shared<Node>(Node{ "root", { a, b } });
}

Value MakeTreeValue(LifeCount& count)
{
    auto shared = std::make_shared<const NodeView>(count, std::make_shared<Node>(Node{ "shared", {} }));
    return MakeHostValue<NodeView>(count, MakeTree(), std::move(shared));
}
} // namespace

TEST(PublicHostObjectTest, NestedObjectWrittenToTheResultRenders)
{
    LifeCount count;
    {
        const ValuesMap params{ { "t", MakeTreeValue(count) } };
        EXPECT_EQ("a|a|leaf|leaf|a,b,|[<a>, <leaf>]|shared|0|False|2", Render("{{ t.first }}|{{ t.first.name }}|{{ t.first.first }}|{{ t.first.first.name }}"
                                                                              "|{% for c in t %}{{ c }},{% endfor %}|{{ [t|first, t.first.first] }}|{{ t.shared.name }}|{{ t.wide }}"
                                                                              "|{{ t.gone is defined }}|{{ t|length }}",
                                                                              params));
        EXPECT_EQ(L"a|leaf", RenderW(L"{{ t.first.name }}|{% for c in t.first %}{{ c }}{% endfor %}", params));
        EXPECT_EQ(0, Render("{{ t.null }}", params).rfind("error: ", 0));
        // The views made for the reads went with the render; the root and its shared object stay
        EXPECT_EQ(2, count.alive);
    }
    EXPECT_EQ(0, count.alive);
}

TEST(PublicHostObjectTest, NestedObjectWrittenToTheResultKeepsItsIdentity)
{
    LifeCount count;
    const ValuesMap params{ { "t", MakeTreeValue(count) }, { "ident", Ident() } };
    EXPECT_EQ("True|True|True,False,|True|False|True", Render("{{ t.first is sameas t.first }}|{{ t.shared is sameas t.shared }}"
                                                              "|{% for c in t %}{{ c is sameas t.first }},{% endfor %}|{{ ident(t.first) is sameas t.first }}"
                                                              "|{{ t.first is sameas t.shared }}|{{ {t.first: 1}[t.first] == 1 }}",
                                                              params));
}

TEST(PublicHostObjectTest, NestedObjectWrittenToTheResultSurvivesTheRender)
{
    LifeCount count;
    std::vector<Value> kept;
    auto keep = Probe([&kept](const UserCallableParams& params) {
        kept.push_back(params["x"]);
        return Value();
    });
    {
        const ValuesMap params{ { "keep", std::move(keep) }, { "t", MakeTreeValue(count) } };
        EXPECT_EQ("", Render("{% do keep(t.first) %}{% do keep(t.shared) %}{% for c in t %}{% do keep(c) %}{% endfor %}", params));
        // The root, its shared object and the three views the host keeps
        EXPECT_EQ(5, count.alive);
    }
    // The root is gone; what the host was handed holds its own objects
    ASSERT_EQ(4U, kept.size());
    EXPECT_EQ(4, count.alive);
    for (const auto& value : kept)
    {
        ASSERT_NE(nullptr, value.GetIf<GenericMap>());
        ASSERT_NE(nullptr, value.GetIf<GenericMap>()->AsHostObject());
    }
    EXPECT_EQ("a|leaf|shared|a|b|True", Render("{{ k[0] }}|{{ k[0].first }}|{{ k[1].name }}|{{ k[2] }}|{{ k[3] }}|{{ k[0] is sameas k[2] }}", { { "k", ValuesList(kept.begin(), kept.end()) } }));
    kept.clear();
    EXPECT_EQ(0, count.alive);
}

namespace
{
// A list whose indexer steps to an item, as a reflected std::set's does: it counts the reads by index
class SteppingList final : public IListItemAccessor
    , public IIndexBasedAccessor
{
public:
    explicit SteppingList(std::vector<int64_t> items, std::shared_ptr<size_t> indexReads)
        : m_items(std::move(items))
        , m_indexReads(std::move(indexReads))
    {
    }

    [[nodiscard]] const IIndexBasedAccessor* GetIndexer() const override { return this; }
    [[nodiscard]] std::optional<ListEnumeratorPtr> CreateEnumerator() const override { return MakeEnumerator<Items>(&m_items); }
    [[nodiscard]] std::optional<size_t> GetSize() const override { return m_items.size(); }
    [[nodiscard]] Value GetItemByIndex(int64_t idx) const override
    {
        ++*m_indexReads;
        return m_items.at(static_cast<size_t>(idx));
    }

private:
    class Items final : public IListEnumerator
    {
    public:
        explicit Items(const std::vector<int64_t>* items)
            : m_items(items)
        {
        }
        void Reset() override { m_next = 0; }
        bool MoveNext() override
        {
            m_cur = m_next;
            return m_next++ < m_items->size();
        }
        [[nodiscard]] Value GetCurrent() const override { return (*m_items)[m_cur]; }
        [[nodiscard]] ListEnumeratorPtr Clone() const override { return MakeEnumerator<Items>(*this); }
        ListEnumeratorPtr Move() override { return MakeEnumerator<Items>(*this); }

    private:
        const std::vector<int64_t>* m_items;
        size_t m_cur = 0;
        size_t m_next = 0;
    };

    std::vector<int64_t> m_items;
    std::shared_ptr<size_t> m_indexReads;
};
} // namespace

// A loop reads a host's list through its enumerator unless the library knows its indexer costs no step: indexing a
// reflected std::set advances to the item, which made a loop over it quadratic
TEST(PublicHostObjectTest, LoopOverAListWithASteppingIndexerUsesItsEnumerator)
{
    auto indexReads = std::make_shared<size_t>(0);
    const ValuesMap params{ { "xs", GenericList(std::make_shared<const SteppingList>(std::vector<int64_t>{ 1, 2, 3 }, indexReads)) } };
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% for x in xs %}{{ x }}{% endfor %};{{ xs[1] }}").has_value());
    const auto result = tpl.RenderAsString(params);
    ASSERT_TRUE(result.has_value()) << result.error().ToString();
    EXPECT_EQ("123;2", result.value());
    // The subscript, never the loop
    EXPECT_EQ(1u, *indexReads);
}
