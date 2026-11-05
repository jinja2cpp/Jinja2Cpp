// neutralize and fence (docs/tasks/0358, docs/cpp-only-filters.md, "Prompt filters")
#include "engine_errors.h"
#include "filters.h"
#include "function_base.h"
#include "internal_value.h"
#include "markup.h"
#include "prompt_escape.h"
#include "render_context.h"

#include <jinja2cpp/prompt.h>
#include <jinja2cpp/template_env.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace filters
{
namespace
{
using namespace std::string_literals;

// lang of fence: CommonMark forbids a backtick in the info string of a backtick fence and a newline would end it
bool IsFenceLanguage(std::string_view lang)
{
    return std::all_of(lang.begin(), lang.end(), [](char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '+' || ch == '.' || ch == '#' || ch == '-';
    });
}

std::vector<std::string> StringList(const InternalValue& value, const char* argument)
{
    bool isList = false;
    const auto list = ConvertToList(value, isList);
    if (!isList)
    {
        throw TemplateRuntimeError("neutralize(): "s + argument + " must be a list of strings");
    }
    std::vector<std::string> result;
    list.ForEach([&result, argument](InternalValue&& moved) {
        const InternalValue item = std::move(moved);
        if (!IsStringValue(item))
        {
            throw TemplateRuntimeError("neutralize(): "s + argument + " must be a list of strings");
        }
        result.push_back(AsString(item));
        return true;
    });
    return result;
}

// The text of a string value of the template's character type, neutralized, or the value itself when nothing changes
template<typename CharT>
InternalValue NeutralizeText(const detail::PromptEscaperImpl* escaper, const InternalValue& text)
{
    if (!escaper)
    {
        return text;
    }
    const auto view = AsStringView<CharT>(text);
    std::basic_string<CharT> out;
    if (!view || !escaper->Neutralize(*view, out, true))
    {
        return text;
    }
    return InternalValue(out);
}

InternalValue NeutralizeValue(const detail::PromptEscaperImpl* escaper, const InternalValue& text, IRendererCallback* callback)
{
    return callback->IsWideTarget() ? NeutralizeText<wchar_t>(escaper, text) : NeutralizeText<char>(escaper, text);
}

// A fence of n backticks, n being 3 or one more than the longest run of backticks in the content
template<typename CharT>
InternalValue MakeFence(std::basic_string_view<CharT> content, std::string_view lang, bool markup)
{
    std::size_t run = 0;
    std::size_t longest = 0;
    for (auto ch : content)
    {
        run = ch == CharT('`') ? run + 1 : 0;
        longest = std::max(longest, run);
    }
    const std::size_t ticks = std::max<std::size_t>(3, longest + 1);
    const std::uint64_t total = static_cast<std::uint64_t>(content.size()) + lang.size() + (2 * static_cast<std::uint64_t>(ticks)) + 2;
    CheckSequenceSize(total, "a fenced string");
    std::basic_string<CharT> out;
    out.reserve(static_cast<std::size_t>(total));
    out.append(ticks, CharT('`'));
    for (char ch : lang)
    {
        out.push_back(static_cast<CharT>(ch));
    }
    out.push_back(CharT('\n'));
    out.append(content);
    if (content.empty() || content.back() != CharT('\n'))
    {
        out.push_back(CharT('\n'));
    }
    out.append(ticks, CharT('`'));
    InternalValue result(out);
    result.SetMarkup(markup);
    return result;
}
} // namespace

struct PromptFilter::Cache
{
    std::mutex mutex;
    PromptEscapeOptions options;
    std::shared_ptr<const PromptEscaper> escaper;
};

PromptFilter::PromptFilter(const FilterParams& params, Mode mode)
    : m_mode(mode)
{
    if (m_mode == NeutralizeMode)
    {
        static const auto args = MakeArgumentsTable({ { "specials", false }, { "tags", false }, { "separator", false } });
        ParseParams(args, params);
        m_cache = std::make_unique<Cache>();
    }
    else
    {
        static const auto args = MakeArgumentsTable({ { "lang", false, ""s } });
        ParseParams(args, params);
    }
}

PromptFilter::PromptFilter(PromptFilter&&) noexcept = default;
PromptFilter::~PromptFilter() = default;

InternalValue PromptFilter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return m_mode == NeutralizeMode ? Neutralize(baseVal, context) : Fence(baseVal, context);
}

InternalValue PromptFilter::Neutralize(const InternalValue& baseVal, RenderContext& context)
{
    auto* callback = context.GetRendererCallback();
    bool specialsGiven = false;
    bool tagsGiven = false;
    bool separatorGiven = false;
    const auto specials = GetArgumentValue("specials", context, specialsGiven);
    const auto tags = GetArgumentValue("tags", context, tagsGiven);
    const auto separator = GetArgumentValue("separator", context, separatorGiven);
    specialsGiven = IsArgumentSet(specials, specialsGiven);
    tagsGiven = IsArgumentSet(tags, tagsGiven);
    separatorGiven = IsArgumentSet(separator, separatorGiven);

    // The environment's escaper, or one made for the arguments that replace its options
    std::shared_ptr<const PromptEscaper> custom;
    const PromptEscaper* escaper = &callback->GetSettings().promptEscaper;
    if (specialsGiven || tagsGiven || separatorGiven)
    {
        PromptEscapeOptions options = escaper->Options();
        if (specialsGiven)
        {
            options.specialTokens = StringList(specials, "specials");
        }
        if (tagsGiven)
        {
            options.tags = StringList(tags, "tags");
        }
        if (separatorGiven)
        {
            if (!IsStringValue(separator))
            {
                throw TemplateRuntimeError("neutralize(): separator must be a string");
            }
            options.separator = AsString(separator);
        }
        if (!PromptEscaper::IsValid(options))
        {
            throw TemplateRuntimeError("neutralize(): the separator must be non-empty and share no character with a special token or tag pattern, and no pattern may be longer than 256 bytes");
        }
        {
            const std::scoped_lock lock(m_cache->mutex);
            if (!m_cache->escaper || m_cache->options.specialTokens != options.specialTokens || m_cache->options.tags != options.tags
                || m_cache->options.separator != options.separator)
            {
                m_cache->escaper = std::make_shared<const PromptEscaper>(options);
                m_cache->options = std::move(options);
            }
            custom = m_cache->escaper;
        }
        escaper = custom.get();
    }

    const auto* impl = detail::PromptEscaperAccess::Impl(*escaper);
    InternalValue text = TextValue(baseVal, callback);
    InternalValue result = NeutralizeValue(impl, text, callback);
    // Markup in, Markup out: it is an explicit request, but the value stays what it was
    result.SetMarkup(baseVal.IsMarkup());
    return result;
}

InternalValue PromptFilter::Fence(const InternalValue& baseVal, RenderContext& context)
{
    auto* callback = context.GetRendererCallback();
    const auto langVal = GetArgumentValue("lang", context);
    if (!IsStringValue(langVal))
    {
        throw TemplateRuntimeError("fence(): lang must be a string");
    }
    const std::string lang = AsString(langVal);
    if (!IsFenceLanguage(lang))
    {
        throw TemplateRuntimeError("fence(): lang may hold only letters, digits and _+.#-");
    }

    // The content is escaped first with the active escaper, so that a fence never skips it. Markup is not escaped again
    const bool autoescape = context.IsAutoescape();
    InternalValue content;
    if (autoescape || HasHtml(baseVal))
    {
        content = TextValue(MarkupEscape(baseVal, callback), callback);
    }
    else
    {
        content = NeutralizeValue(detail::PromptEscaperAccess::Impl(callback->GetSettings().promptEscaper), TextValue(baseVal, callback), callback);
    }
    if (callback->IsWideTarget())
    {
        const auto view = AsStringView<wchar_t>(content);
        return MakeFence<wchar_t>(view ? *view : std::wstring_view(), lang, autoescape);
    }
    const auto view = AsStringView<char>(content);
    return MakeFence<char>(view ? *view : std::string_view(), lang, autoescape);
}

} // namespace filters
} // namespace v2
} // namespace jinja2
