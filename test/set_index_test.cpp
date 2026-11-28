// The hash index beside the engine's set and its walk's size check (docs/tasks/0561). Every row's expectation is
// what Python Jinja2 renders (3.1.6); a row with an error names the message Python raises
#include "test_tools.h"

#include "../src/engine_errors.h"
#include "../src/internal_value.h"
#include "../src/set_index.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

using namespace jinja2;

namespace
{
struct SetCase
{
    const char* name;
    const char* source;
    // The output; unused when `error` is set
    const char* output;
    // A part of the error message; empty when the template renders
    const char* error;
};

// A narrow string as wide characters (the sources are ASCII)
std::wstring Widen(const std::string& text)
{
    return std::wstring(text.begin(), text.end());
}

std::string ToString(const std::string& text)
{
    return text;
}

// The expectations are ASCII: each wide character narrows to one char (MSVC /W4 warns of an implicit narrowing, C4244)
std::string ToString(const std::wstring& text)
{
    std::string result;
    result.reserve(text.size());
    for (const wchar_t ch : text)
    {
        result.push_back(static_cast<char>(ch));
    }
    return result;
}

template<typename TemplateType, typename StringType>
void Check(const SetCase& row, const StringType& source, const StringType& expected, const StringType& errorText, const Settings& settings)
{
    TemplateEnv env;
    env.SetSettings(settings);
    TemplateType tpl(&env);
    const auto loaded = tpl.Load(source);
    ASSERT_TRUE(!!loaded) << row.name;
    const auto rendered = tpl.RenderAsString(ValuesMap{});
    if (!errorText.empty())
    {
        ASSERT_FALSE(!!rendered) << row.name;
        EXPECT_NE(std::string::npos, ErrorToString(rendered.error()).find(errorText)) << row.name << ": " << ToString(ErrorToString(rendered.error()));
        return;
    }
    ASSERT_TRUE(!!rendered) << row.name << ": " << ToString(ErrorToString(rendered.error()));
    EXPECT_EQ(expected, rendered.value()) << row.name;
}

const std::vector<SetCase>& Rows()
{
    static const std::vector<SetCase> rows = {
        { "RemoveThenAdd", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.remove(3) %}{% set _ = s.add(6) %}{{ s|list }}|{{ 3 in s }}|{{ 6 in s }}|{{ 4 in s }}|{{ s|length }})", R"([1, 2, 4, 5, 6]|False|True|True|5)", "" },
        { "RemoveFirstAndLast", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.remove(1) %}{% set _ = s.remove(5) %}{{ s|list }}|{{ 1 in s }}|{{ 2 in s }}|{{ 4 in s }}|{{ 5 in s }})", R"([2, 3, 4]|False|True|True|False)", "" },
        { "DiscardPresentAndMissing", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.discard(2) %}{% set _ = s.discard(77) %}{% set _ = s.discard(2) %}{{ s|list }}|{{ 2 in s }}|{{ 3 in s }})", R"([1, 3, 4, 5]|False|True)", "" },
        { "RemoveMissingIsKeyError", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.remove(77) %})", "", R"(77)" },
        { "PopThenMembership", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set p = s.pop() %}{{ p }}|{{ s|list }}|{{ p in s }}|{{ 2 in s }}|{{ s|length }})", R"(1|[2, 3, 4, 5]|False|True|4)", "" },
        { "PopAllThenAdd", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% for i in range(5) %}{% set _ = s.pop() %}{% endfor %}{% set _ = s.add(3) %}{{ s|list }}|{{ 3 in s }}|{{ 1 in s }})", R"([3]|True|False)", "" },
        { "ClearThenAdd", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.clear() %}{{ 3 in s }}|{{ s|length }}|{% set _ = s.add(4) %}{{ s|list }}|{{ 4 in s }}|{{ 3 in s }})", R"(False|0|[4]|True|False)", "" },
        { "UpdateThenMembership", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.update([6, 7, 1]) %}{{ s|list }}|{{ 7 in s }}|{{ 8 in s }}{% set _ = s.add(7) %}{% set _ = s.add(8) %}{{ s|list }})", R"([1, 2, 3, 4, 5, 6, 7]|True|False[1, 2, 3, 4, 5, 6, 7, 8])", "" },
        { "IntersectionUpdate", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.intersection_update([2, 4, 9]) %}{{ s|list }}|{{ 2 in s }}|{{ 3 in s }}|{{ 9 in s }}{% set _ = s.add(5) %}{{ s|list }})", R"([2, 4]|True|False|False[2, 4, 5])", "" },
        { "DifferenceUpdate", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.difference_update([2, 4, 9]) %}{{ s|list }}|{{ 2 in s }}|{{ 3 in s }}{% set _ = s.add(2) %}{{ s|length }})", R"([1, 3, 5]|False|True4)", "" },
        { "SymmetricDifferenceUpdate", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.symmetric_difference_update([4, 5, 6]) %}{{ s|list }}|{{ 4 in s }}|{{ 6 in s }}|{{ 1 in s }}{% set _ = s.discard(6) %}{{ s|list }})", R"([1, 2, 3, 6]|False|True|True[1, 2, 3])", "" },
        { "UnhashableAdd", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.add([1]) %})", "", R"(unhashable type: 'list')" },
        { "UnhashableIn", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{{ [1] in s }})", "", R"(unhashable type: 'list')" },
        { "UnhashableDiscard", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.discard({}) %})", "", R"(unhashable type: 'dict')" },
        { "OneIntFloatBool", R"({% set s = {1:0}.keys() - [] %}{% set _ = s.add(1.0) %}{% set _ = s.add(true) %}{{ s|length }}|{{ 1.0 in s }}|{{ true in s }}|{{ 2 in s }}{% set _ = s.discard(1.0) %}{{ s|length }})", R"(1|True|True|False0)", "" },
        { "TupleAndString", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.add((1, 2)) %}{% set _ = s.add('a') %}{% set _ = s.add((1, 2)) %}{{ s|length }}|{{ (1, 2) in s }}|{{ 'a' in s }}|{{ 'b' in s }}|{{ (2, 1) in s }})", R"(7|True|True|False|False)", "" },
        { "AddDuringLoopRaises", R"({% set s = {1: 0, 2: 0}.keys() - [] %}{% for x in s %}{% set _ = s.add(3) %}{{ x }}{% endfor %})", "", R"(Set changed size during iteration)" },
        { "DiscardDuringLoopRaises", R"({% set s = {1: 0, 2: 0, 3: 0}.keys() - [] %}{% for x in s %}{% set _ = s.discard(3) %}{{ x }}{% endfor %})", "", R"(Set changed size during iteration)" },
        { "AddExistingDuringLoop", R"({% set s = {1: 0, 2: 0}.keys() - [] %}{% for x in s %}{% set _ = s.add(x) %}{{ x }}{% endfor %})", R"(12)", "" },
        { "AddAndDiscardDuringLoopKeepsSize", R"({% set s = {1: 0, 2: 0, 3: 0}.keys() - [] %}{% for x in s %}{% set _ = s.add(x + 10) %}{% set _ = s.discard(x + 10) %}{{ x }}{% endfor %})", R"(123)", "" },
        { "SingleItemAddDuringLoopRaises", R"({% set s = {1: 0}.keys() - [] %}{% for x in s %}{% set _ = s.add(2) %}{{ x }}{% endfor %})", "", R"(Set changed size during iteration)" },
        { "ListGrowsUnderLoop", R"({% set l = [1, 2] %}{% for x in l %}{% if l|length < 4 %}{% set _ = l.append(x + 2) %}{% endif %}{{ x }}{% endfor %})", R"(1234)", "" },
        { "CopyHasItsOwnIndex", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set c = s.copy() %}{% set _ = c.remove(1) %}{% set _ = s.add(9) %}{{ 1 in c }}|{{ 1 in s }}|{{ 9 in c }}|{{ 9 in s }}|{{ c|length }}|{{ s|length }})", R"(False|True|False|True|4|6)", "" },
        // Erased slots stay until a read closes them up (docs/tasks/0587)
        { "DiscardMiddleThenAdd", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.discard(2) %}{% set _ = s.discard(4) %}{% set _ = s.add(6) %}{{ s|sort }}|{{ s|length }}|{{ 2 in s }}|{{ 6 in s }})", R"([1, 3, 5, 6]|4|False|True)", "" },
        { "PopTwiceThenIterate", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.pop() %}{% set _ = s.pop() %}{% for x in s %}{{ x }}{% endfor %}|{{ s|length }}|{{ s.pop() }})", R"(345|3|3)", "" },
        { "PopUntilCompactThenAdd", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% for i in range(3) %}{% set _ = s.pop() %}{% endfor %}{{ s|length }}|{% set _ = s.add(1) %}{{ s|sort }}|{{ 1 in s }}|{{ 4 in s }})", R"(2|[1, 4, 5]|True|True)", "" },
        { "DiscardThenLength", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.discard(1) %}{{ s|length }}|{{ s }}|{{ s|sum }})", R"(4|{2, 3, 4, 5}|14)", "" },
        { "DiscardAllThenPopEmpty", R"({% set s = {1:0}.keys() - [] %}{% set _ = s.discard(1) %}{{ s|length }}{{ s.pop() }})", "", R"(pop from an empty set)" },
        { "AliasSeesErase", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set t = s %}{% set _ = s.pop() %}{% set _ = s.discard(3) %}{{ t|list }}|{{ t|length }}|{{ 2 in t }})", R"([2, 4, 5]|3|True)", "" },
        { "CopyAfterErase", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.discard(2) %}{% set c = s.copy() %}{% set _ = s.discard(3) %}{{ c|list }}|{{ s|list }}|{{ s == c }})", R"([1, 3, 4, 5]|[1, 4, 5]|False)", "" },
        { "UpdateManyArgs", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.update([6, 1], (7, 6), 'ab') %}{{ s|length }}|{{ 'a' in s }}|{{ 7 in s }})", R"(9|True|True)", "" },
        { "UpdateNotIterable", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.update([6], 5) %})", "", R"('int' object is not iterable)" },
        { "UpdateUnhashable", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.update([6, [1]]) %})", "", R"(unhashable type: 'list')" },
        { "DifferenceUpdateMany", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.difference_update([1], (2, 9)) %}{{ s|list }}|{{ 1 in s }}|{{ s|length }})", R"([3, 4, 5]|False|3)", "" },
        { "SymmetricDifferenceUpdateDup", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.symmetric_difference_update([1, 1, 6, 6]) %}{{ s|sort }}|{{ 1 in s }}|{{ 6 in s }})", R"([2, 3, 4, 5, 6]|False|True)", "" },
        { "SameNanOnce", R"({% set w = 'nan' %}{% set s = {}.keys() - [] %}{% set n = w|float %}{% set _ = s.add(n) %}{% set _ = s.add(n) %}{% set _ = s.add((n, 1)) %}{% set _ = s.add((n, 1)) %}{{ s|length }}|{{ n in s }})", R"(2|True)", "" },
        // intersection_update walks the argument and drops the rest through the index (docs/tasks/0593)
        { "IntersectionUpdateTakesArgumentItems", R"({% set s = {1:0,2:0,3:0}.keys() - [] %}{% set _ = s.intersection_update([2.0, 3]) %}{{ s|list }}|{{ s|length }}|{{ 2 in s }}|{{ 1 in s }})", R"([2.0, 3]|2|True|False)", "" },
        { "IntersectionUpdateManyArgs", R"({% set s = {1:0,2:0,3:0,4:0}.keys() - [] %}{% set _ = s.intersection_update([1, 2, 3], [3, 4, 1]) %}{{ s|list }}|{{ 2 in s }})", R"([1, 3]|False)", "" },
        { "IntersectionUpdateNoArgs", R"({% set s = {1:0,2:0,3:0,4:0}.keys() - [] %}{% set _ = s.intersection_update() %}{{ s|list }}|{{ 4 in s }})", R"([1, 2, 3, 4]|True)", "" },
        { "IntersectionUpdateLargerSetKeepsReceiverItems", R"({% set s = {1:0,2:0}.keys() - [] %}{% set b = {1.0:0,2.0:0,3:0}.keys() - [] %}{% set _ = s.intersection_update(b) %}{{ s|list }})", R"([1, 2])", "" },
        { "IntersectionUpdateSmallerSetGivesItems", R"({% set s = {1:0,2:0,3:0}.keys() - [] %}{% set b = {1.0:0,2.0:0}.keys() - [] %}{% set _ = s.intersection_update(b) %}{{ s|list }})", R"([1.0, 2.0])", "" },
        { "IntersectionUpdateDuplicatesThenAdd", R"({% set s = {1:0,2:0,3:0}.keys() - [] %}{% set _ = s.intersection_update([3, 3]) %}{{ s|list }}|{{ 3 in s }}|{{ 1 in s }}{% set _ = s.add(1) %}{{ s|sort }})", R"([3]|True|False[1, 3])", "" },
        { "IntersectionUpdateWithItself", R"({% set s = {1:0,2:0,3:0}.keys() - [] %}{% set _ = s.intersection_update(s, [1]) %}{{ s|list }}|{{ 2 in s }})", R"([1]|False)", "" },
        { "IntersectionUpdateAfterDiscard", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set _ = s.discard(2) %}{% set _ = s.intersection_update([5, 1, 2]) %}{{ s|list }}|{{ 2 in s }}|{{ 5 in s }}|{{ s|length }})", R"([1, 5]|False|True|2)", "" },
        { "IntersectionUpdateNotIterableKeepsSet", R"({% set s = {1:0,2:0,3:0}.keys() - [] %}{% set _ = s.intersection_update([1], 5) %})", "", R"('int' object is not iterable)" },
        { "AliasSharesIndex", R"({% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set t = s %}{% set _ = t.remove(2) %}{% set _ = s.add(6) %}{{ 2 in s }}|{{ 2 in t }}|{{ 6 in t }}|{{ s|list }})", R"(False|False|True|[1, 3, 4, 5, 6])", "" },
    };
    return rows;
}
} // namespace

class SetIndexTest : public testing::TestWithParam<SetCase>
{
};

TEST_P(SetIndexTest, Narrow)
{
    const auto& row = GetParam();
    Check<Template>(row, std::string(row.source), std::string(row.output), std::string(row.error), Settings{});
}

TEST_P(SetIndexTest, Wide)
{
    const auto& row = GetParam();
    Check<TemplateW>(row, Widen(row.source), Widen(row.output), Widen(row.error), Settings{});
}

INSTANTIATE_TEST_SUITE_P(Rows, SetIndexTest, testing::ValuesIn(Rows()), [](const testing::TestParamInfo<SetCase>& entry) { return std::string(entry.param.name); });

// An add in a loop does a constant amount of work: 20000 adds and as many lookups. A scan per add is 2*10^8
// comparisons, minutes in a debug build, which the render's time limit stops (docs/tasks/0561). The size is
// moderate because MSVC debug builds run set code far slower (docs/tasks/0565)
TEST(SetIndexLinearTest, AddInALoopIsLinear)
{
    Settings settings;
    settings.limits.maxRenderTime = std::chrono::seconds(30);
    TemplateEnv env;
    env.SetSettings(settings);
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{% set s = {}.keys() - [] %}{% for i in range(20000) %}{% set _ = s.add(i) %}{% set _ = s.add(i) %}{% endfor %}"
                           "{{ s|length }}|{{ 0 in s }}|{{ 19999 in s }}|{{ 20000 in s }}"));
    const auto rendered = tpl.RenderAsString(ValuesMap{});
    ASSERT_TRUE(!!rendered) << ErrorToString(rendered.error());
    EXPECT_EQ("20000|True|True|False", rendered.value());
}


// The set's other methods in a loop cost what their arguments hold (docs/tasks/0587): an erase leaves its slot until the
// erased ones pass half, `update` goes through the index, and one NaN is found again as itself. Each was quadratic
// before, seconds to minutes in a debug build
TEST(SetIndexLinearTest, ChangesInALoopAreLinear)
{
    const char* const prefix = "{% set w = 'nan' %}{% set s = {}.keys() - [] %}{% for i in range(20000) %}{% set _ = s.add(i) %}{% endfor %}";
    const struct
    {
        const char* body;
        const char* expected;
    } rows[] = {
        { "{% for i in range(20000) %}{% set _ = s.pop() %}{% endfor %}{{ s|length }}", "0" },
        { "{% for i in range(20000) %}{% set _ = s.discard(i) %}{{ s|length if i == 9999 }}{% endfor %}|{{ s|length }}", "10000|0" },
        { "{% for i in range(19999, -1, -1) %}{% set _ = s.remove(i) %}{% endfor %}{{ s|length }}", "0" },
        { "{% for i in range(20000) %}{% if i in s %}{% set _ = s.discard(i) %}{% endif %}{% endfor %}{{ s|length }}", "0" },
        { "{% for i in range(20000, 40000) %}{% set _ = s.update([i]) %}{% set _ = s.difference_update([i - 20000]) %}{% endfor %}{{ s|length }}|{{ 39999 in s }}|{{ 0 in s }}",
          "20000|True|False" },
        { "{% for i in range(20000) %}{% set _ = s.symmetric_difference_update([i, i + 20000]) %}{% endfor %}{{ s|length }}|{{ 20000 in s }}", "20000|True" },
        { "{% set _ = s.discard(0) %}{% for i in range(20000, 40000) %}{% set _ = s.add(i) %}{% endfor %}{{ s|length }}|{{ 39999 in s }}", "39999|True" },
        { "{% set n = w|float %}{% for i in range(20000) %}{% set _ = s.add(n) %}{% endfor %}{{ s|length }}", "20001" },
    };
    for (const auto& row : rows)
    {
        Settings settings;
        settings.limits.maxRenderTime = std::chrono::seconds(30);
        TemplateEnv env;
        env.SetSettings(settings);
        Template tpl(&env);
        ASSERT_TRUE(!!tpl.Load(std::string(prefix) + row.body)) << row.body;
        const auto rendered = tpl.RenderAsString(ValuesMap{});
        ASSERT_TRUE(!!rendered) << row.body << ": " << ErrorToString(rendered.error());
        EXPECT_EQ(row.expected, rendered.value()) << row.body;
    }
}


// A set nothing looks up has no index (docs/tasks/0593): the temporaries a - b and s|list make, a loop over a set and its
// size cost none; the first lookup makes one that the copies sharing the items share
TEST(SetIndexLazyTest, TemporariesMakeNoIndex)
{
    TemplateEnv env;
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{% set s = {1:0,2:0,3:0,4:0,5:0}.keys() - [] %}{% set b = {1:0}.keys() - [] %}{% for i in range(20) %}{% set t = s - b %}{% set l = s|list %}{{ t|length }}{{ l|length }}{% for x in t %}{% endfor %}{% endfor %}{{ s|length }}"));
    const auto before = SetIndexesMade();
    const auto rendered = tpl.RenderAsString(ValuesMap{});
    ASSERT_TRUE(!!rendered) << ErrorToString(rendered.error());
    EXPECT_EQ(before, SetIndexesMade());

    // A lookup makes the index of that set only
    Template lookup(&env);
    ASSERT_TRUE(!!lookup.Load("{% set s = {1:0,2:0,3:0}.keys() - [] %}{% set t = s %}{% for i in range(20) %}{{ 1 in s }}{{ 2 in t }}{% endfor %}"));
    const auto beforeLookup = SetIndexesMade();
    const auto looked = lookup.RenderAsString(ValuesMap{});
    ASSERT_TRUE(!!looked) << ErrorToString(looked.error());
    EXPECT_EQ(beforeLookup + 1, SetIndexesMade());
}

// Internal functions are not exported from a shared library
#ifndef JINJA2CPP_LINK_AS_SHARED
TEST(SetIndexLazyTest, AdapterCopiesShareTheIndexMadeLater)
{
    const auto set = ListAdapter::CreateIterator(InternalValueList{ InternalValue(std::int64_t{ 1 }), InternalValue(std::int64_t{ 2 }) }, IteratorKind::Set);
    const ListAdapter copy(set); // NOLINT(performance-unnecessary-copy-initialization): the copy is what the test is about
    const auto before = SetIndexesMade();
    auto* index = copy.GetSetIndex();
    ASSERT_NE(nullptr, index);
    EXPECT_EQ(before + 1, SetIndexesMade());
    // The copy that was taken before the index existed sees the one the other made
    EXPECT_EQ(index, set.GetSetIndex());
    EXPECT_EQ(before + 1, SetIndexesMade());
    // A list has none
    const auto list = ListAdapter::CreateAdapter(InternalValueList{ InternalValue(std::int64_t{ 1 }) });
    EXPECT_EQ(nullptr, list.GetSetIndex());
}

// A cloned walk fails at the size the walk began with, not the size at the clone (docs/tasks/0593)
TEST(SetIndexLazyTest, ClonedWalkKeepsItsStartSize)
{
    const auto set = ListAdapter::CreateIterator(InternalValueList{ InternalValue(std::int64_t{ 1 }), InternalValue(std::int64_t{ 2 }), InternalValue(std::int64_t{ 3 }) }, IteratorKind::Set);
    auto walk = set.GetEnumerator();
    ASSERT_TRUE(walk.has_value());
    auto& walker = **walk; // NOLINT(bugprone-unchecked-optional-access): checked above
    ASSERT_TRUE(walker.MoveNext());
    // The set grows under the walk, then the walk is cloned
    set.GetMutableItems()->emplace_back(std::int64_t{ 4 });
    auto clone = walker.Clone();
    ASSERT_TRUE(clone.has_value());
    auto& cloned = **clone; // NOLINT(bugprone-unchecked-optional-access): checked above
    EXPECT_THROW(walker.MoveNext(), TemplateRuntimeError);
    EXPECT_THROW(cloned.MoveNext(), TemplateRuntimeError);
}
#endif

// A one-item argument in a loop (docs/tasks/0593): the work of a call is what its argument holds, and the receiver is
// walked only for what leaves it
TEST(SetIndexLinearTest, IntersectionUpdateWithOneItemInALoopIsLinear)
{
    const char* const prefix = "{% set s = {}.keys() - [] %}{% for i in range(20000) %}{% set _ = s.add(i) %}{% endfor %}";
    const struct
    {
        const char* body;
        const char* expected;
    } rows[] = {
        { "{% set _ = s.intersection_update([5]) %}{{ s|list }}", "[5]" },
        { "{% for i in range(20000) %}{% set _ = s.add(i + 20000) %}{% set _ = s.intersection_update([i + 20000]) %}{% endfor %}{{ s|list }}", "[39999]" },
        { "{% for i in range(20000) %}{% set _ = s.intersection_update([i]) %}{% set _ = s.add(i + 1) %}{% endfor %}{{ s|list }}", "[19999, 20000]" },
        { "{% for i in range(20000) %}{% set _ = s.intersection_update(s) %}{% set _ = s.intersection_update([i]) %}{% set _ = s.update([i, i + 1]) %}{% endfor %}{{ s|length }}", "2" },
    };
    for (const auto& row : rows)
    {
        Settings settings;
        settings.limits.maxRenderTime = std::chrono::seconds(30);
        TemplateEnv env;
        env.SetSettings(settings);
        Template tpl(&env);
        ASSERT_TRUE(!!tpl.Load(std::string(prefix) + row.body)) << row.body;
        const auto rendered = tpl.RenderAsString(ValuesMap{});
        ASSERT_TRUE(!!rendered) << row.body << ": " << ErrorToString(rendered.error());
        EXPECT_EQ(row.expected, rendered.value()) << row.body;
    }
}
