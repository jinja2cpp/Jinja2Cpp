#include "template_dict.h"

#include "contracts.h"
#include "engine_errors.h"
#include "host_object.h"
#include "internal_value.h"
#include "recursion_guard.h"
#include "undefined.h"
#include "value_visitors.h"

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/value.h>

#include <fmt/format.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace jinja2
{
inline namespace v2
{
namespace
{
// The kind byte after the mark. Built from char constants, never from string literals: a
// token holds NUL bytes, and "\xFFf" would read as the single character \xFFF
constexpr char KindInt = 'n';
constexpr char KindFloat = 'g';
constexpr char KindNone = 'N';
constexpr char KindTuple = 't';
constexpr char KindRange = 'r';
constexpr char KindString = 's';
constexpr char KindBigInt = 'b';
constexpr char KindObject = 'o';
constexpr char KindBoundMethod = 'm';
constexpr char KindMethodCall = 'f';
constexpr char KindUndefined = 'u';
constexpr char KindUserCallable = 'c';

// The type of a value Python's hash() rejects, also inside a tuple
struct UnhashableTypeGetter : visitors::BaseVisitor<const char*>
{
    using visitors::BaseVisitor<const char*>::operator();

    explicit UnhashableTypeGetter(uint64_t& items)
        : m_items(items)
    {
    }

    const char* operator()(const ListRef& list) const
    {
        // A tuple can nest deeper than the stack: Python raises RecursionError
        CheckStack();
        // A generator hashes by identity, as a Python object does (docs/tasks/0401)
        if (list.GetRangeInfo() || list.GetIteratorKind() != IteratorKind::None)
        {
            return nullptr;
        }
        if (!list.IsTuple())
        {
            return "list";
        }
        // A lazy tuple (('x',) * 2**40) reports items it never stores: the walk is held to
        // MaxSequenceSize items over all its tuples, like any sequence the template builds
        m_items += list.GetSize().value_or(0);
        CheckSequenceSize(m_items, "a dict key");
        const char* result = nullptr;
        list.ForEach([this, &result](const InternalValue& item) {
            result = Apply<UnhashableTypeGetter>(item, m_items);
            return result == nullptr;
        });
        return result;
    }
    const char* operator()(const MapRef& map) const
    {
        // An object of the host language says itself, as Python asks its __hash__
        if (const auto* host = map.GetHostObject())
        {
            return host->Hash() ? nullptr : host->TypeName();
        }
        return map.GetObjectClass() || map.GetAttrPolicy() == MapAttrPolicy::KeysFirst ? nullptr : "dict";
    }
    const char* operator()(const KeyValuePair& pair) const { return Apply<UnhashableTypeGetter>(pair.value, m_items); }
    // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): replaces BaseVisitor's deleted overload
    const char* operator()(const BigIntView& /*val*/) const { return nullptr; }

private:
    uint64_t& m_items;
};

template<typename T>
void AppendBytes(std::string& out, T value)
{
    char bytes[sizeof(T)];
    std::memcpy(bytes, &value, sizeof(T));
    out.append(bytes, sizeof(T));
}

void AppendInt(std::string& out, int64_t value)
{
    out.push_back(DictKeyMark);
    out.push_back(KindInt);
    AppendBytes(out, value);
}

// An integer outside int64 by its decimal spelling (docs/tasks/0270)
void AppendBigInt(std::string& out, std::string_view decimal)
{
    out.push_back(DictKeyMark);
    out.push_back(KindBigInt);
    AppendBytes(out, static_cast<uint64_t>(decimal.size()));
    out.append(decimal);
}

// ObjectKey: the key of an object by its identity, as Python's default __hash__ and __eq__
// have it. The dict's table of first spellings (TemplateDict::KeyOf) holds the key, which keeps
// the object, and so its address, alive while the entry is there. An object of the host
// language is keyed by IEngineHostObject::Identity (docs/tasks/0352), so that two views of one host
// object are one key; the engine's objects by the identity their copies share (docs/tasks/0212).
// A token that holds an address is never kept past the dict nor compared across renders
void AppendObjectKey(std::string& out, const void* identity)
{
    out.push_back(DictKeyMark);
    out.push_back(KindObject);
    AppendBytes(out, reinterpret_cast<uintptr_t>(identity)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast): the address is the key
}

// The number of items of range(start, stop, step), for a step that is not zero
uint64_t RangeLength(const RangeInfo& range)
{
    if (range.step > 0 ? range.stop <= range.start : range.stop >= range.start)
    {
        return 0;
    }
    // Unsigned, so that the distance of two int64 values cannot overflow
    if (range.step > 0)
    {
        const auto distance = static_cast<uint64_t>(range.stop) - static_cast<uint64_t>(range.start);
        return ((distance - 1) / static_cast<uint64_t>(range.step)) + 1;
    }
    const auto distance = static_cast<uint64_t>(range.start) - static_cast<uint64_t>(range.stop);
    const auto stride = uint64_t{ 0 } - static_cast<uint64_t>(range.step);
    return ((distance - 1) / stride) + 1;
}

// Appends the token of a value, and returns the type that made it unhashable instead
struct KeyTokenBuilder
{
    explicit KeyTokenBuilder(std::string& out)
        : m_out(out)
    {
    }

    template<typename CharT>
    const char* operator()(const std::basic_string<CharT>& str) const
    {
        AddString(std::basic_string_view<CharT>(str));
        return nullptr;
    }
    template<typename CharT>
    const char* operator()(const std::basic_string_view<CharT>& str) const
    {
        AddString(str);
        return nullptr;
    }
    const char* operator()(bool val) const
    {
        AppendInt(m_out, val ? 1 : 0);
        return nullptr;
    }
    const char* operator()(int64_t val) const
    {
        AppendInt(m_out, val);
        return nullptr;
    }
    const char* operator()(const BigIntView& val) const
    {
        AppendBigInt(m_out, val.decimal);
        return nullptr;
    }
    const char* operator()(double val) const
    {
        // 2^63: the first double that is no int64
        constexpr double limit = 9223372036854775808.0;
        if (val == std::floor(val) && val >= -limit && val < limit)
        {
            // 1.0 is the key 1, and -0.0 the key 0
            AppendInt(m_out, static_cast<int64_t>(val));
            return nullptr;
        }
        // A whole double beyond int64 is the key of the equal big integer, which fmt spells exactly
        if (std::isfinite(val) && val == std::floor(val))
        {
            AppendBigInt(m_out, fmt::format("{:.0f}", val));
            return nullptr;
        }
        // Python compares NaN by identity; one NaN key here, a deliberate divergence
        const auto number = std::isnan(val) ? std::numeric_limits<double>::quiet_NaN() : val;
        m_out.push_back(DictKeyMark);
        m_out.push_back(KindFloat);
        AppendBytes(m_out, number);
        return nullptr;
    }
    const char* operator()(NoneValue /*val*/) const
    {
        m_out.push_back(DictKeyMark);
        m_out.push_back(KindNone);
        return nullptr;
    }
    const char* operator()(const ListRef& list) const
    {
        CheckStack();
        if (const auto* range = list.GetRangeInfo())
        {
            // Equal ranges are equal keys: an empty range is one, and a single item has no step
            auto length = RangeLength(*range);
            auto start = range->start;
            auto step = range->step;
            if (length == 0)
            {
                start = 0;
                step = 0;
            }
            else if (length == 1)
            {
                step = 1;
            }
            m_out.push_back(DictKeyMark);
            m_out.push_back(KindRange);
            AppendBytes(m_out, length);
            AppendBytes(m_out, start);
            AppendBytes(m_out, step);
            return nullptr;
        }
        // A generator or iterator hashes by identity, which its copies share (docs/tasks/0401)
        if (list.GetIteratorKind() != IteratorKind::None)
        {
            AppendObjectKey(m_out, list.GetIdentity());
            return nullptr;
        }
        if (!list.IsTuple())
        {
            return "list";
        }
        // A lazy tuple (('x',) * 2**40) reports items it never stores; its token would, so it
        // is held to MaxSequenceSize like any sequence the template builds
        const auto size = list.GetSize().value_or(0);
        CheckSequenceSize(size, "a dict key");
        m_out.push_back(DictKeyMark);
        m_out.push_back(KindTuple);
        AppendBytes(m_out, static_cast<uint32_t>(size));
        const char* result = nullptr;
        list.ForEach([this, &result](const InternalValue& item) {
            CheckSequenceSize(m_out.size(), "a dict key");
            result = AddElement(item);
            return result == nullptr;
        });
        return result;
    }
    const char* operator()(const MapRef& map) const
    {
        if (const auto* host = map.GetHostObject())
        {
            if (!host->Hash())
            {
                return host->TypeName();
            }
            AppendObjectKey(m_out, host->Identity());
            return nullptr;
        }
        // namespace(), cycler(), joiner() and reflected structs: the copies of a namespace share
        // its storage, which is its identity
        if (map.GetObjectClass() || map.GetAttrPolicy() == MapAttrPolicy::KeysFirst)
        {
            AppendObjectKey(m_out, map.GetIdentity());
            return nullptr;
        }
        return "dict";
    }
    // A dictsort pair is the tuple (key, value)
    const char* operator()(const KeyValuePair& pair) const
    {
        CheckStack();
        m_out.push_back(DictKeyMark);
        m_out.push_back(KindTuple);
        AppendBytes(m_out, uint32_t{ 2 });
        if (const char* type = AddElement(pair.key))
        {
            return type;
        }
        return AddElement(pair.value);
    }
    // Python hashes Undefined by its class, and the policy is one per render: every undefined value
    // is one key. StrictUndefined refuses to be hashed
    const char* operator()(const UndefinedValue& val) const
    {
        CheckStrictUndefined(val);
        m_out.push_back(DictKeyMark);
        m_out.push_back(KindUndefined);
        return nullptr;
    }
    const char* operator()(const Callable& callable) const
    {
        AddCallable(m_out, callable);
        return nullptr;
    }
    const char* operator()(const detail::RendererPtr& renderer) const
    {
        AppendObjectKey(m_out, renderer.get());
        return nullptr;
    }
    // A reference to a host value (a lent default) by the value it refers to
    const char* operator()(const ValueRef& ref) const
    {
        AppendObjectKey(m_out, &ref.get());
        return nullptr;
    }

    // A function, a macro or a method by Callable::Identity, which == compares too
    static void AddCallable(std::string& out, const Callable& callable)
    {
        const auto identity = callable.GetIdentity();
        if (identity.method)
        {
            out.push_back(DictKeyMark);
            out.push_back(KindBoundMethod);
            AppendBytes(out, reinterpret_cast<uintptr_t>(identity.object)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast): the address is the key
            AppendBytes(out, reinterpret_cast<uintptr_t>(identity.method)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast): the address is the key
            return;
        }
        if (identity.call)
        {
            // Python's builtin method: the function, then its __self__
            out.push_back(DictKeyMark);
            out.push_back(KindMethodCall);
            AppendBytes(out, identity.call->invoke);
            AddReceiver(out, identity.call->self);
            return;
        }
        if (identity.userCallable != 0)
        {
            out.push_back(DictKeyMark);
            out.push_back(KindUserCallable);
            AppendBytes(out, identity.userCallable);
            return;
        }
        AppendObjectKey(out, identity.object);
    }

    // The __self__ of a method: a container by its identity, as Python compares it (a list's
    // methods hash although the list does not); a string or a number by its value, which
    // stands in for an identity it does not have here
    static void AddReceiver(std::string& out, const InternalValue& self)
    {
        if (const auto list = AsList(self))
        {
            AppendObjectKey(out, list->GetIdentity());
            return;
        }
        if (const auto map = AsMap(self))
        {
            AppendObjectKey(out, map->GetIdentity());
            return;
        }
        if (Apply<KeyTokenBuilder>(self, out))
        {
            // A value that hashes by neither (none is made today)
            AppendObjectKey(out, detail::InternalValueAccess::Object(self));
        }
    }

private:
    // A tuple item: the length of its token, then the token
    [[nodiscard]] const char* AddElement(const InternalValue& item) const
    {
        const auto lengthAt = m_out.size();
        AppendBytes(m_out, uint32_t{ 0 });
        const auto start = m_out.size();
        if (const char* type = Apply<KeyTokenBuilder>(item, m_out))
        {
            return type;
        }
        const auto length = static_cast<uint32_t>(m_out.size() - start);
        std::memcpy(&m_out[lengthAt], &length, sizeof(length));
        return nullptr;
    }

    template<typename CharT>
    void AddString(std::basic_string_view<CharT> str) const
    {
        if constexpr (std::is_same_v<CharT, char>)
        {
            if (!str.empty() && str.front() == DictKeyMark)
            {
                m_out.push_back(DictKeyMark);
                m_out.push_back(KindString);
            }
            m_out.append(str);
        }
        else
        {
            // Keys are narrow UTF-8 whatever the template's character type
            const auto text = ConvertString<std::string>(str);
            AddString(std::string_view(text));
        }
    }

    std::string& m_out;
};

std::string EscapeString(const std::string& name)
{
    std::string token;
    token.reserve(name.size() + 2);
    token.push_back(DictKeyMark);
    token.push_back(KindString);
    token += name;
    return token;
}

bool NeedsEscape(const std::string& name)
{
    return !name.empty() && name.front() == DictKeyMark;
}

// Whether a stored name is a token that has a spelling of its own, not an escaped string
bool IsSpelledToken(const std::string& stored)
{
    return stored.size() >= 2 && stored.front() == DictKeyMark && stored[1] != KindString;
}
} // namespace

const char* UnhashableType(const InternalValue& value)
{
    uint64_t items = 0;
    return Apply<UnhashableTypeGetter>(value, items);
}

std::optional<std::string> MakeDictKey(const InternalValue& key, const char** unhashableType)
{
    std::string token;
    const char* type = Apply<KeyTokenBuilder>(key, token);
    if (type)
    {
        if (unhashableType)
        {
            *unhashableType = type;
        }
        return std::nullopt;
    }
    return token;
}

bool SameMethodReceiver(const InternalValue& left, const InternalValue& right)
{
    // Containers by identity, strings by their text, without copying either
    const auto leftList = AsList(left);
    const auto rightList = AsList(right);
    if (leftList || rightList)
    {
        return leftList && rightList && leftList->GetIdentity() == rightList->GetIdentity();
    }
    const auto leftMap = AsMap(left);
    const auto rightMap = AsMap(right);
    if (leftMap || rightMap)
    {
        return leftMap && rightMap && leftMap->GetIdentity() == rightMap->GetIdentity();
    }
    if (const auto leftText = AsStringView<char>(left))
    {
        const auto rightText = AsStringView<char>(right);
        return rightText && *leftText == *rightText;
    }
    if (const auto leftText = AsStringView<wchar_t>(left))
    {
        const auto rightText = AsStringView<wchar_t>(right);
        return rightText && *leftText == *rightText;
    }
    // A number or none: a token of a few bytes
    std::string leftKey;
    std::string rightKey;
    KeyTokenBuilder::AddReceiver(leftKey, left);
    KeyTokenBuilder::AddReceiver(rightKey, right);
    return leftKey == rightKey;
}

void ThrowUnhashable(const char* type)
{
    throw TemplateRuntimeError(std::string("unhashable type: '") + (type ? type : "object") + "'");
}

std::string DictKeyText(const InternalValue& key)
{
    if (IsStringValue(key))
    {
        return AsString(key);
    }
    std::string text;
    Apply<visitors::ValueRenderer<char>>(key, text, false);
    return text;
}

TemplateDict::TemplateDict(InternalDict&& items)
    : m_items(std::move(items))
{
    ChargeEntries();
    if constexpr (detail::ContractChecks >= 2)
    {
        for (const auto& item : m_items)
        {
            JINJA2CPP_ASSERT(!NeedsEscape(item.first) && "a name never starts with the mark of a dict key token");
        }
    }
}

TemplateDict::TemplateDict(const TemplateDict& other)
    : m_items(other.m_items)
    , m_keys(other.m_keys ? std::make_unique<KeyTable>(*other.m_keys) : nullptr)
{
    ChargeEntries();
}

TemplateDict& TemplateDict::operator=(const TemplateDict& other)
{
    if (this != &other)
    {
        TemplateDict copy(other);
        *this = std::move(copy);
    }
    return *this;
}

TemplateDict& TemplateDict::operator=(TemplateDict&& other) noexcept(std::is_nothrow_move_assignable_v<InternalDict>)
{
    if (this != &other)
    {
        ReleaseEntries();
        m_items = std::move(other.m_items);
        m_keys = std::move(other.m_keys);
    }
    return *this;
}

std::uint64_t TemplateDict::EntryBytes(const std::string& stored) noexcept
{
    // The entry and about two words of the index for it; a key past the short-string buffer has its own block, and
    // a key that is not a string has its spelling in m_keys too
    constexpr std::size_t shortKey = 15;
    const std::uint64_t key = stored.size() > shortKey ? stored.size() + 1 : 0;
    std::uint64_t bytes = sizeof(InternalDict::value_type) + (2 * sizeof(void*)) + key;
    if (IsSpelledToken(stored))
    {
        bytes += sizeof(KeyTable::value_type) + (2 * sizeof(void*)) + key;
    }
    return bytes;
}

void TemplateDict::ChargeEntriesSlow() const
{
    std::uint64_t bytes = 0;
    for (const auto& item : m_items)
    {
        bytes += EntryBytes(item.first);
    }
    ChargeRenderMemory(bytes);
}

void TemplateDict::ReleaseEntriesSlow() const noexcept
{
    std::uint64_t bytes = 0;
    for (const auto& item : m_items)
    {
        bytes += EntryBytes(item.first);
    }
    ReleaseRenderMemory(bytes);
}

TemplateDict::const_iterator TemplateDict::FindString(const std::string& name) const
{
    return NeedsEscape(name) ? m_items.find(EscapeString(name)) : m_items.find(name);
}

TemplateDict::iterator TemplateDict::FindString(const std::string& name)
{
    return NeedsEscape(name) ? m_items.find(EscapeString(name)) : m_items.find(name);
}

TemplateDict::const_iterator TemplateDict::Find(const InternalValue& key) const
{
    auto token = MakeDictKey(key);
    return token ? m_items.find(*token) : m_items.end();
}

TemplateDict::iterator TemplateDict::Find(const InternalValue& key)
{
    auto token = MakeDictKey(key);
    return token ? m_items.find(*token) : m_items.end();
}

TemplateDict::iterator TemplateDict::Set(const InternalValue& key, InternalValue value)
{
    const char* unhashable = nullptr;
    auto token = MakeDictKey(key, &unhashable);
    if (!token)
    {
        ThrowUnhashable(unhashable);
    }
    // A new entry is counted before it is added
    std::uint64_t bytes = 0;
    if (RenderMemoryCounted() && m_items.find(*token) == m_items.end())
    {
        bytes = EntryBytes(*token);
        ChargeRenderMemory(bytes);
    }
    try
    {
        auto [pos, inserted] = m_items.insert_or_assign(std::move(*token), std::move(value));
        if (inserted && IsSpelledToken(pos->first))
        {
            try
            {
                if (!m_keys)
                {
                    m_keys = std::make_unique<KeyTable>();
                }
                m_keys->emplace(pos->first, key);
            }
            catch (...)
            {
                m_items.erase(pos);
                throw;
            }
        }
        return pos;
    }
    catch (...)
    {
        ReleaseRenderMemory(bytes);
        throw;
    }
}

TemplateDict::iterator TemplateDict::SetString(std::string name, InternalValue value)
{
    if (NeedsEscape(name))
    {
        name = EscapeString(name);
    }
    // A new entry is counted before it is added
    std::uint64_t bytes = 0;
    if (RenderMemoryCounted() && m_items.find(name) == m_items.end())
    {
        bytes = EntryBytes(name);
        ChargeRenderMemory(bytes);
    }
    try
    {
        return m_items.insert_or_assign(std::move(name), std::move(value)).first;
    }
    catch (...)
    {
        ReleaseRenderMemory(bytes);
        throw;
    }
}

std::pair<TemplateDict::iterator, bool> TemplateDict::TryEmplace(const InternalValue& key, InternalValue value)
{
    const char* unhashable = nullptr;
    auto token = MakeDictKey(key, &unhashable);
    if (!token)
    {
        ThrowUnhashable(unhashable);
    }
    if (auto pos = m_items.find(*token); pos != m_items.end())
    {
        return { pos, false };
    }
    return { Set(key, std::move(value)), true };
}

TemplateDict::iterator TemplateDict::Erase(const_iterator pos)
{
    if (RenderMemoryCounted())
    {
        ReleaseRenderMemory(EntryBytes(pos->first));
    }
    if (m_keys && IsSpelledToken(pos->first))
    {
        m_keys->erase(pos->first);
    }
    return m_items.erase(pos);
}

void TemplateDict::Clear()
{
    ReleaseEntries();
    m_items.clear();
    m_keys.reset();
}

InternalValue TemplateDict::KeyOf(const std::string& stored) const
{
    if (!NeedsEscape(stored))
    {
        return InternalValue(stored);
    }
    if (m_keys)
    {
        if (auto spelling = m_keys->find(stored); spelling != m_keys->end())
        {
            return spelling->second;
        }
    }
    // An escaped string; a token without a spelling is read as the string it is
    if (stored.size() >= 2 && stored[1] == KindString)
    {
        return InternalValue(stored.substr(2));
    }
    return InternalValue(stored);
}

} // namespace v2
} // namespace jinja2
