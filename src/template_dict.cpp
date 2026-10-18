#include "template_dict.h"

#include "internal_value.h"
#include "recursion_guard.h"
#include "value_visitors.h"

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/value.h>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace jinja2
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

// The type of a value Python's hash() rejects, also inside a tuple
struct UnhashableTypeGetter : visitors::BaseVisitor<const char*>
{
    using visitors::BaseVisitor<const char*>::operator();

    const char* operator()(const ListRef& list) const
    {
        // A tuple can nest deeper than the stack: Python raises RecursionError
        CheckStack();
        if (list.GetRangeInfo())
        {
            return nullptr;
        }
        if (!list.IsTuple())
        {
            return "list";
        }
        const char* result = nullptr;
        list.ForEach([&result](const InternalValue& item) {
            result = Apply<UnhashableTypeGetter>(item);
            return result == nullptr;
        });
        return result;
    }
    const char* operator()(const MapRef& map) const
    {
        return map.GetObjectClass() || map.GetAttrPolicy() == MapAttrPolicy::KeysFirst ? nullptr : "dict";
    }
    const char* operator()(const KeyValuePair& pair) const { return Apply<UnhashableTypeGetter>(pair.value); }
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
    KeyTokenBuilder(std::string& out, bool& hasObject)
        : m_out(out)
        , m_hasObject(hasObject)
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
        if (!list.IsTuple())
        {
            return "list";
        }
        m_out.push_back(DictKeyMark);
        m_out.push_back(KindTuple);
        AppendBytes(m_out, static_cast<uint32_t>(list.GetSize().value_or(0)));
        const char* result = nullptr;
        list.ForEach([this, &result](const InternalValue& item) {
            result = AddElement(item);
            return result == nullptr;
        });
        return result;
    }
    const char* operator()(const MapRef& map) const
    {
        if (map.GetObjectClass() || map.GetAttrPolicy() == MapAttrPolicy::KeysFirst)
        {
            AddObject();
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
    // Undefined values, callables and renderers
    template<typename T>
    const char* operator()(const T& /*val*/) const
    {
        AddObject();
        return nullptr;
    }

private:
    // A tuple item: the length of its token, then the token
    [[nodiscard]] const char* AddElement(const InternalValue& item) const
    {
        const auto lengthAt = m_out.size();
        AppendBytes(m_out, uint32_t{ 0 });
        const auto start = m_out.size();
        if (const char* type = Apply<KeyTokenBuilder>(item, m_out, m_hasObject))
        {
            return type;
        }
        const auto length = static_cast<uint32_t>(m_out.size() - start);
        std::memcpy(&m_out[lengthAt], &length, sizeof(length));
        return nullptr;
    }

    // Objects, undefined values and callables have no key of their own yet (docs/tasks/0212):
    // they add nothing, so as a key each stands for the empty string, as dict keys did before,
    // and a lookup by one finds nothing (MakeDictKey's hasObject)
    void AddObject() const { m_hasObject = true; }

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
    bool& m_hasObject;
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
    return Apply<UnhashableTypeGetter>(value);
}

std::optional<std::string> MakeDictKey(const InternalValue& key, const char** unhashableType, bool* hasObject)
{
    std::string token;
    bool objectFound = false;
    const char* type = Apply<KeyTokenBuilder>(key, token, objectFound);
    if (hasObject)
    {
        *hasObject = objectFound;
    }
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

void ThrowUnhashable(const char* type)
{
    throw std::runtime_error(std::string("unhashable type: '") + (type ? type : "object") + "'");
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
#ifndef NDEBUG
    for (const auto& item : m_items)
    {
        assert(!NeedsEscape(item.first) && "a name never starts with the mark of a dict key token");
    }
#endif
}

TemplateDict::TemplateDict(const TemplateDict& other)
    : m_items(other.m_items)
    , m_keys(other.m_keys ? std::make_unique<KeyTable>(*other.m_keys) : nullptr)
{
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
    bool hasObject = false;
    auto token = MakeDictKey(key, nullptr, &hasObject);
    return token && !hasObject ? m_items.find(*token) : m_items.end();
}

TemplateDict::iterator TemplateDict::Find(const InternalValue& key)
{
    bool hasObject = false;
    auto token = MakeDictKey(key, nullptr, &hasObject);
    return token && !hasObject ? m_items.find(*token) : m_items.end();
}

TemplateDict::iterator TemplateDict::Set(const InternalValue& key, InternalValue value)
{
    const char* unhashable = nullptr;
    auto token = MakeDictKey(key, &unhashable);
    if (!token)
    {
        ThrowUnhashable(unhashable);
    }
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

TemplateDict::iterator TemplateDict::SetString(std::string name, InternalValue value)
{
    if (NeedsEscape(name))
    {
        return m_items.insert_or_assign(EscapeString(name), std::move(value)).first;
    }
    return m_items.insert_or_assign(std::move(name), std::move(value)).first;
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
    if (m_keys && IsSpelledToken(pos->first))
    {
        m_keys->erase(pos->first);
    }
    return m_items.erase(pos);
}

void TemplateDict::Clear()
{
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

} // namespace jinja2
