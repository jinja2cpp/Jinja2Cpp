// count_tokens, truncate_tokens and pack (docs/tasks/0359, docs/cpp-only-filters.md, "Prompt filters")
#include "token_budget.h"

#include "engine_errors.h"
#include "expression_evaluator.h"
#include "filters.h"
#include "function_base.h"
#include "internal_value.h"
#include "markup.h"
#include "render_context.h"
#include "undefined.h"
#include "value_visitors.h"

#include <jinja2cpp/prompt.h>
#include <jinja2cpp/string_helpers.h>

#include <algorithm>
#include <limits>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace
{
using namespace std::string_literals;

bool IsContinuation(char ch) noexcept
{
    return (static_cast<unsigned char>(ch) & 0xC0) == 0x80;
}

// The nearest start of a UTF-8 sequence at or before offset (the end of the text is one)
std::size_t RoundDown(std::string_view text, std::size_t offset) noexcept
{
    offset = std::min(offset, text.size());
    while (offset > 0 && offset < text.size() && IsContinuation(text[offset]))
    {
        --offset;
    }
    return offset;
}

std::size_t RoundUp(std::string_view text, std::size_t offset) noexcept
{
    while (offset < text.size() && IsContinuation(text[offset]))
    {
        ++offset;
    }
    return offset;
}

// ceil(bytes / 4), the "about four bytes per token" estimate
constexpr std::size_t EstimateTokens(std::uint64_t bytes) noexcept
{
    return static_cast<std::size_t>((bytes / 4) + (bytes % 4 != 0 ? 1 : 0));
}

std::uint64_t Utf8Size(std::wstring_view text) noexcept
{
    std::uint64_t size = 0;
    while (!text.empty())
    {
        char32_t cp = 0;
        text.remove_prefix(detail::DecodeWide(text, cp));
        size += cp < 0x80 ? 1 : cp < 0x800 ? 2
                            : cp < 0x10000 ? 3
                                           : 4;
    }
    return size;
}

// What a host callback raised leaves as the render's UserCallableException
template<typename Call>
auto CallHost(const Call& call) -> decltype(call())
{
    try
    {
        return call();
    }
    catch (const std::bad_alloc&)
    {
        throw;
    }
    catch (const detail::HostCallableFailure&)
    {
        throw;
    }
    catch (...)
    {
        throw detail::HostCallableFailure(std::current_exception());
    }
}
} // namespace

Tokenizer::Tokenizer(CountFunction count, PrefixFunction prefix)
{
    if (!count)
    {
        if (prefix)
        {
            throw std::invalid_argument("a Tokenizer needs a count function for its prefix function");
        }
        return;
    }
    m_impl = std::make_shared<const detail::TokenizerImpl>(std::move(count), std::move(prefix));
}

std::size_t Tokenizer::Count(std::string_view text) const
{
    if (text.empty())
    {
        return 0;
    }
    return m_impl ? m_impl->CountFunction()(text) : EstimateTokens(text.size());
}

namespace detail
{
std::size_t TokenMeter::CountUtf8(std::string_view text) const
{
    if (text.empty())
    {
        return 0;
    }
    if (!m_impl)
    {
        return EstimateTokens(text.size());
    }
    return CallHost([&] { return m_impl->CountFunction()(text); });
}

std::size_t TokenMeter::PrefixUtf8(std::string_view text, std::size_t maxTokens) const
{
    if (text.empty() || maxTokens == 0)
    {
        return 0;
    }
    if (!m_impl)
    {
        // count(prefix) <= maxTokens as 4 * maxTokens bytes are that many tokens at most
        return RoundDown(text, maxTokens > text.size() / 4 ? text.size() : maxTokens * 4);
    }
    if (const auto& prefix = m_impl->PrefixFunction())
    {
        // The host's result is not trusted: past the text it is the text, inside a sequence it is the sequence before
        return RoundDown(text, CallHost([&] { return prefix(text, maxTokens); }));
    }
    // The longest prefix that fits by a search on its length. Counts of BPE tokens do not grow with the text for sure,
    // so this is a prefix that fits and not always the longest. lo fits, hi does not; both start a code point
    std::size_t lo = 0;
    std::size_t hi = text.size();
    while (hi - lo > 1)
    {
        const std::size_t half = lo + ((hi - lo) / 2);
        std::size_t mid = RoundDown(text, half);
        if (mid <= lo)
        {
            mid = RoundUp(text, half);
            if (mid >= hi)
            {
                break;
            }
        }
        if (CountUtf8(text.substr(0, mid)) <= maxTokens)
        {
            lo = mid;
        }
        else
        {
            hi = mid;
        }
    }
    return lo;
}

template<>
std::size_t TokenMeter::Count<char>(std::string_view text) const
{
    return CountUtf8(text);
}

template<>
std::size_t TokenMeter::Count<wchar_t>(std::wstring_view text) const
{
    if (text.empty())
    {
        return 0;
    }
    if (!m_impl)
    {
        return EstimateTokens(Utf8Size(text));
    }
    return CountUtf8(ConvertString<std::string>(text));
}

template<>
std::size_t TokenMeter::PrefixSize<char>(std::string_view text, std::size_t maxTokens) const
{
    return PrefixUtf8(text, maxTokens);
}

template<>
std::size_t TokenMeter::PrefixSize<wchar_t>(std::wstring_view text, std::size_t maxTokens) const
{
    const auto utf8 = ConvertString<std::string>(text);
    const std::size_t bytes = PrefixUtf8(utf8, maxTokens);
    // The same number of code points of the wide text: the conversion keeps one per code point
    auto codePoints = static_cast<std::size_t>(std::count_if(utf8.begin(), utf8.begin() + static_cast<std::ptrdiff_t>(bytes), [](char ch) { return !IsContinuation(ch); }));
    std::size_t units = 0;
    while (codePoints > 0 && units < text.size())
    {
        char32_t cp = 0;
        units += DecodeWide(text.substr(units), cp);
        --codePoints;
    }
    return units;
}
} // namespace detail

namespace filters
{
namespace
{
using detail::TokenMeter;

int64_t IntegerArgument(const InternalValue& val, const char* filter, const char* argument)
{
    CheckUndefinedUse(val, UndefinedUse::Arithmetic);
    const auto* number = GetIf<int64_t>(&val);
    if (!number)
    {
        throw TemplateRuntimeError(std::string(filter) + "(): " + argument + " must be an integer");
    }
    return *number;
}

// The tokens of str(val), in the template's character type
std::size_t CountValue(const TokenMeter& meter, const InternalValue& val, IRendererCallback* callback)
{
    // str() of an undefined value is empty, which StrictUndefined refuses
    CheckUndefinedUse(val, UndefinedUse::Operator);
    const InternalValue text = TextValue(val, callback);
    if (callback->IsWideTarget())
    {
        const auto view = AsStringView<wchar_t>(text);
        return view ? meter.Count<wchar_t>(*view) : 0;
    }
    const auto view = AsStringView<char>(text);
    return view ? meter.Count<char>(*view) : 0;
}

template<typename CharT>
InternalValue TruncateText(const TokenMeter& meter, const InternalValue& value, const InternalValue& endVal, int64_t length, IRendererCallback* callback)
{
    using String = std::basic_string<CharT>;
    InternalValue text = TextValue(value, callback);
    const auto view = AsStringView<CharT>(text);
    if (!view)
    {
        return text;
    }
    if (length >= 0 && meter.Count<CharT>(*view) <= static_cast<std::uint64_t>(length))
    {
        // The string itself, sharing its text
        text.SetMarkup(value.IsMarkup());
        return text;
    }
    const String end = GetAsSameString(String(), endVal).value_or(String());
    const std::uint64_t endTokens = meter.Count<CharT>(end);
    if (length < 0 || static_cast<std::uint64_t>(length) < endTokens)
    {
        throw TemplateRuntimeError("truncate_tokens(): expected n >= "s + std::to_string(endTokens) + " (the tokens of end), got " + std::to_string(length));
    }

    // The prefix of m tokens and end make at most length tokens, unless the tokens merge across the join: then m drops
    auto budget = static_cast<std::size_t>(static_cast<std::uint64_t>(length) - endTokens);
    for (;;)
    {
        const std::size_t prefixSize = meter.PrefixSize<CharT>(*view, budget);
        // Host text may exceed the cap, and a token budget does not bound characters
        CheckSequenceSize(static_cast<std::uint64_t>(prefixSize) + end.size(), "a truncated string");
        String result(view->substr(0, prefixSize));
        result.append(end);
        const std::size_t tokens = budget == 0 ? 0 : meter.Count<CharT>(result);
        if (budget == 0 || tokens <= static_cast<std::uint64_t>(length))
        {
            return InternalValue(std::move(result));
        }
        const std::size_t excess = tokens - static_cast<std::size_t>(length);
        budget = budget > excess ? budget - excess : 0;
    }
}
// The order pack considers the items in: that of sort(attribute=by, reverse=reverse), or their own, the last first with
// reverse
std::vector<std::size_t> ConsiderationOrder(const InternalValueList& items, bool byGiven, const InternalValue& by, bool reverse, RenderContext& context)
{
    if (byGiven)
    {
        return SortedOrderByPaths(items, AttributeSortPaths(by, true), reverse ? BinaryExpression::LogicalGt : BinaryExpression::LogicalLt, BinaryExpression::CaseInsensitive, context);
    }
    std::vector<std::size_t> order(items.size());
    for (std::size_t n = 0; n != order.size(); ++n)
    {
        order[n] = reverse ? order.size() - 1 - n : n;
    }
    return order;
}
} // namespace

TokenBudget::TokenBudget(const FilterParams& params, Mode mode)
    : m_mode(mode)
{
    switch (m_mode)
    {
    case CountMode:
    {
        static const auto args = MakeArgumentsTable({});
        ParseParams(args, params);
        break;
    }
    case TruncateMode:
    {
        static const auto args = MakeArgumentsTable({ { "n", true }, { "end", false, "..."s } });
        ParseParams(args, params);
        break;
    }
    case PackMode:
    {
        static const auto args = MakeArgumentsTable({ { "budget", true },
                                                      { "attribute", false },
                                                      { "by", false },
                                                      { "reverse", false, InternalValue(false) },
                                                      { "reserve", false, InternalValue(int64_t{ 0 }) },
                                                      { "stop", false, InternalValue(false) } });
        ParseParams(args, params);
        break;
    }
    }
}

InternalValue TokenBudget::Filter(const InternalValue& baseVal, RenderContext& context)
{
    switch (m_mode)
    {
    case CountMode:
    {
        auto* callback = context.GetRendererCallback();
        const TokenMeter meter(callback->GetSettings().tokenizer);
        // A host count above the int range saturates rather than wrapping negative
        const std::size_t count = CountValue(meter, baseVal, callback);
        return InternalValue(static_cast<int64_t>(std::min<std::size_t>(count, static_cast<std::size_t>(std::numeric_limits<int64_t>::max()))));
    }
    case TruncateMode:
        return Truncate(baseVal, context);
    case PackMode:
        return Pack(baseVal, context);
    }
    return InternalValue();
}

InternalValue TokenBudget::Truncate(const InternalValue& baseVal, RenderContext& context)
{
    auto* callback = context.GetRendererCallback();
    const auto length = IntegerArgument(GetArgumentValue("n", context), "truncate_tokens", "n");
    const auto endVal = GetArgumentValue("end", context);
    // end is joined to the text, which an undefined one refuses under StrictUndefined
    CheckUndefinedUse(endVal, UndefinedUse::Operator);
    if (!IsStringValue(endVal) && !endVal.IsUndefined())
    {
        throw TemplateRuntimeError("truncate_tokens(): end must be a string");
    }
    CheckUndefinedUse(baseVal, UndefinedUse::Operator);
    const TokenMeter meter(callback->GetSettings().tokenizer);
    return callback->IsWideTarget() ? TruncateText<wchar_t>(meter, baseVal, endVal, length, callback)
                                    : TruncateText<char>(meter, baseVal, endVal, length, callback);
}

InternalValue TokenBudget::Pack(const InternalValue& baseVal, RenderContext& context)
{
    auto* callback = context.GetRendererCallback();
    bool attributeGiven = false;
    bool byGiven = false;
    const auto budgetVal = GetArgumentValue("budget", context);
    const auto attribute = GetArgumentValue("attribute", context, attributeGiven);
    const auto by = GetArgumentValue("by", context, byGiven);
    const auto reverse = ConvertToBool(GetArgumentValue("reverse", context));
    const auto reserveVal = GetArgumentValue("reserve", context);
    const auto stop = ConvertToBool(GetArgumentValue("stop", context));
    attributeGiven = IsArgumentSet(attribute, attributeGiven);
    byGiven = IsArgumentSet(by, byGiven);

    const auto budget = IntegerArgument(budgetVal, "pack", "budget");
    const auto reserve = IntegerArgument(reserveVal, "pack", "reserve");
    if (budget < 0 || reserve < 0)
    {
        throw TemplateRuntimeError("pack(): budget and reserve must not be negative");
    }

    bool isConverted = false;
    const ListAdapter source = ConvertToList(baseVal, isConverted, false);
    if (!isConverted)
    {
        if (IsNotIterableValue(baseVal))
        {
            ThrowNotIterableValue(baseVal);
        }
        // An undefined value is an empty sequence, which StrictUndefined refuses
        CheckUndefinedUse(baseVal, UndefinedUse::Operator);
        return ListAdapter::CreateAdapter(InternalValueList());
    }
    // Iterated once and kept as owned values, as `list` and `sort` copy them: a generator is consumed and a lent item
    // does not outlive its statement. A generator is held to the sequence cap as it grows
    InternalValueList items = source.ToValueList();
    // Nothing is left of the budget to put an item in
    if (reserve > budget)
    {
        return ListAdapter::CreateAdapter(InternalValueList());
    }

    const auto order = ConsiderationOrder(items, byGiven, by, reverse, context);

    const InternalValueList path = attributeGiven ? AttributePathOf(attribute) : InternalValueList();
    const TokenMeter meter(callback->GetSettings().tokenizer);
    auto remaining = static_cast<std::uint64_t>(budget) - static_cast<std::uint64_t>(reserve);
    std::vector<bool> kept(items.size(), false);
    // An item is measured when it is considered, at most once, and none once the budget is spent
    for (const auto index : order)
    {
        if (remaining == 0)
        {
            break;
        }
        const InternalValue& item = items[index];
        const std::size_t tokens = attributeGiven ? CountValue(meter, ItemAttribute(item, path, context), callback) : CountValue(meter, item, callback);
        if (tokens <= remaining)
        {
            kept[index] = true;
            remaining -= tokens;
        }
        else if (stop)
        {
            break;
        }
    }

    InternalValueList result;
    for (std::size_t index = 0; index != items.size(); ++index)
    {
        if (kept[index])
        {
            result.push_back(std::move(items[index]));
        }
    }
    return ListAdapter::CreateAdapter(std::move(result));
}

} // namespace filters
} // namespace v2
} // namespace jinja2
