#ifndef JINJA2CPP_JSON_ADAPTER_H
#define JINJA2CPP_JSON_ADAPTER_H

// The contract between Jinja2C++ and a JSON library (docs/tasks/0311). It includes no JSON library: a binding
// (jinja2cpp/binding/<library>.h, or the user's own) specialises JsonTraits, and Reflect and ToValue then work for
// that library's value type.

#include <jinja2cpp/big_int.h>
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/value.h>
#include <jinja2cpp/value_ptr.h>

#include <nonstd/expected.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
//! The kinds of JSON value a binding reports; BigInt is an integer outside `int64_t` and `uint64_t`, given as text
enum class JsonKind : std::uint8_t
{
    Null,
    Bool,
    Int,  //!< fits `int64_t`
    UInt, //!< read with `AsUInt`; values above INT64_MAX become \ref BigInt
    Double,
    String,
    Array,
    Object,
    BigInt, //!< any other integer, read with `AsBigIntText`
};

//! What a function of forward-only traits returns when the walk can fail partway through the document
template<typename T>
using JsonResult = nonstd::expected<T, std::string>;

/*!
 * \brief Describes a JSON library to Jinja2C++; specialise it for the library's value type
 *
 * Every member is static. `Node` is a cheap, copyable handle of one value of the document.
 *
 * Required by both \ref Reflect (a view of the document) and \ref ToValue (a copy of it):
 * ```c++
 * using Node = ...;
 * static Node Root(const Json& doc);              // Json& for a parser that consumes the document
 * static JsonKind Kind(Node);
 * static bool AsBool(Node);
 * static std::int64_t AsInt(Node);
 * static double AsDouble(Node);
 * static S AsString(Node);                        // S: std::string_view, std::string, std::wstring(_view)
 * template<class F> static R ForEachMember(Node, F f);  // calls f(key, Node) -> bool for each member of an
 *                                                 // object in the library's order and stops when it returns false;
 *                                                 // key: std::string_view or std::string, valid during the call
 * ```
 * Optional:
 * ```c++
 * static std::uint64_t AsUInt(Node);              // when Kind can return UInt
 * static std::string_view AsBigIntText(Node);     // when Kind can return BigInt: [-]digits
 * template<class F> static R ForEachElement(Node, F f); // f(Node) -> bool per item of an array; built from
 *                                                 // Size and At when absent
 * static std::optional<std::uint64_t> Location(Node);     // byte offset of the value in the input, for errors
 * static bool SameNode(Node, Node);               // identity, for IComparable; operator== when absent
 * static constexpr bool IsHandle = true;          // Json does not own its storage: Reflect(Json) borrows it
 * ```
 * Random access, which \ref Reflect needs and \ref ToValue does not:
 * ```c++
 * static std::size_t Size(Node);                  // items of an array or members of an object
 * static Node At(Node, std::size_t i);            // item i < Size(node) of an array, O(1)
 * static std::optional<Node> Find(Node, std::string_view key);  // must not keep `key`
 * ```
 * Forward-only traits (a parser that reads the input as it goes) may return `JsonResult<T>` instead of `T` from any
 * function, and `JsonResult<void>` from `ForEach*`; \ref ToValue reports the message as `ErrorCode::DataReadError`.
 * Such traits support only \ref ToValue: the view has no way to report an error, so traits whose `Kind` or `At`
 * return `JsonResult` are not viewable. Strings and keys are copied before the next call. The functions may be
 * called from several threads at once for one document, as concurrent renders do.
 *
 * `Tag` allows partial specialisation with `std::enable_if_t`.
 */
template<typename Json, typename Tag = void>
struct JsonTraits
{
};

/*!
 * \brief A base for the traits of a DOM library, whose values have stable addresses
 *
 * `Node` refers to a value of the document; two nodes are the same when they refer to the same value.
 */
template<typename Json>
struct DomJsonTraits
{
    using Node = std::reference_wrapper<const Json>;
    static Node Root(const Json& doc) { return std::cref(doc); }
    static bool SameNode(Node a, Node b) { return &a.get() == &b.get(); }
};

//! The built-in cap on the items of one array or object and the characters of one string (the engine's own cap)
inline constexpr std::uint64_t JsonDefaultMaxSequenceSize = std::uint64_t{ 1 } << 26;

//! Bounds of \ref ToValue, for documents that may be untrusted
struct JsonCopyLimits
{
    //! Arrays and objects open at once; the copy recurses once per level
    std::size_t maxDepth = 128;
    //! Items of one array, members of one object, code units of one string; 0 means JsonDefaultMaxSequenceSize,
    //! and a larger value is lowered to it
    std::uint64_t maxSequenceSize = 0;
};

namespace detail::json
{
template<typename T>
struct IsResult : std::false_type
{
};
template<typename T>
struct IsResult<nonstd::expected<T, std::string>> : std::true_type
{
};
template<typename R>
struct ResultValue
{
    using type = R;
};
template<typename T>
struct ResultValue<nonstd::expected<T, std::string>>
{
    using type = T;
};

template<typename J>
using Traits = JsonTraits<std::remove_cv_t<std::remove_reference_t<J>>>;
template<typename J>
using NodeOf = typename Traits<J>::Node;

struct ElementProbe
{
    template<typename N>
    bool operator()(N&&) const
    {
        return true;
    }
};
struct MemberProbe
{
    template<typename K, typename N>
    bool operator()(K&&, N&&) const
    {
        return true;
    }
};

template<typename J, typename = void>
struct HasCore : std::false_type
{
};
template<typename J>
struct HasCore<J,
               std::void_t<NodeOf<J>,
                           decltype(Traits<J>::Kind(std::declval<NodeOf<J>>())),
                           decltype(Traits<J>::AsBool(std::declval<NodeOf<J>>())),
                           decltype(Traits<J>::AsInt(std::declval<NodeOf<J>>())),
                           decltype(Traits<J>::AsDouble(std::declval<NodeOf<J>>())),
                           decltype(Traits<J>::AsString(std::declval<NodeOf<J>>())),
                           decltype(Traits<J>::ForEachMember(std::declval<NodeOf<J>>(), std::declval<MemberProbe>()))>> : std::true_type
{
};

template<typename J, typename = void>
struct HasSizeAt : std::false_type
{
};
template<typename J>
struct HasSizeAt<J, std::void_t<decltype(Traits<J>::Size(std::declval<NodeOf<J>>())), decltype(Traits<J>::At(std::declval<NodeOf<J>>(), std::size_t{}))>>
    : std::bool_constant<!IsResult<decltype(Traits<J>::Size(std::declval<NodeOf<J>>()))>::value && !IsResult<decltype(Traits<J>::At(std::declval<NodeOf<J>>(), std::size_t{}))>::value>
{
};

template<typename J, typename = void>
struct HasFind : std::false_type
{
};
template<typename J>
struct HasFind<J, std::void_t<decltype(Traits<J>::Find(std::declval<NodeOf<J>>(), std::string_view{}))>>
    : std::is_same<decltype(Traits<J>::Find(std::declval<NodeOf<J>>(), std::string_view{})), std::optional<NodeOf<J>>>
{
};

// The view calls every function without an error channel
template<typename J, typename = void>
struct HasPlainScalars : std::false_type
{
};
template<typename J>
struct HasPlainScalars<J, std::enable_if_t<HasCore<J>::value>>
    : std::bool_constant<!IsResult<decltype(Traits<J>::Kind(std::declval<NodeOf<J>>()))>::value && !IsResult<decltype(Traits<J>::AsBool(std::declval<NodeOf<J>>()))>::value && !IsResult<decltype(Traits<J>::AsInt(std::declval<NodeOf<J>>()))>::value && !IsResult<decltype(Traits<J>::AsDouble(std::declval<NodeOf<J>>()))>::value && !IsResult<decltype(Traits<J>::AsString(std::declval<NodeOf<J>>()))>::value && !IsResult<decltype(Traits<J>::ForEachMember(std::declval<NodeOf<J>>(), std::declval<MemberProbe>()))>::value>
{
};

template<typename J, typename = void>
struct HasForEachElement : std::false_type
{
};
template<typename J>
struct HasForEachElement<J, std::void_t<decltype(Traits<J>::ForEachElement(std::declval<NodeOf<J>>(), std::declval<ElementProbe>()))>> : std::true_type
{
};

template<typename J, typename = void>
struct HasAsUInt : std::false_type
{
};
template<typename J>
struct HasAsUInt<J, std::void_t<decltype(Traits<J>::AsUInt(std::declval<NodeOf<J>>()))>> : std::true_type
{
};

template<typename J, typename = void>
struct HasBigIntText : std::false_type
{
};
template<typename J>
struct HasBigIntText<J, std::void_t<decltype(Traits<J>::AsBigIntText(std::declval<NodeOf<J>>()))>> : std::true_type
{
};

template<typename J, typename = void>
struct HasLocation : std::false_type
{
};
template<typename J>
struct HasLocation<J, std::void_t<decltype(Traits<J>::Location(std::declval<NodeOf<J>>()))>> : std::true_type
{
};

template<typename J, typename = void>
struct HasSameNode : std::false_type
{
};
template<typename J>
struct HasSameNode<J, std::void_t<decltype(Traits<J>::SameNode(std::declval<NodeOf<J>>(), std::declval<NodeOf<J>>()))>> : std::true_type
{
};

template<typename J, typename = void>
struct IsHandle : std::false_type
{
};
template<typename J>
struct IsHandle<J, std::enable_if_t<Traits<J>::IsHandle>> : std::true_type
{
};
} // namespace detail::json

//! JsonTraits<Json> support \ref Reflect, a view that reads the document in place
template<typename Json>
inline constexpr bool IsJsonViewable = detail::json::HasCore<Json>::value && detail::json::HasPlainScalars<Json>::value && detail::json::HasSizeAt<Json>::value && detail::json::HasFind<Json>::value;

//! JsonTraits<Json> support \ref ToValue, a copy of the document into \ref ValuesMap and \ref ValuesList
template<typename Json>
inline constexpr bool IsJsonCopyable = detail::json::HasCore<Json>::value && (detail::json::HasForEachElement<Json>::value || detail::json::HasSizeAt<Json>::value);

/*!
 * \brief Copies a JSON document into \ref ValuesMap, \ref ValuesList and scalars, in one pass
 *
 * The copy owns its data and outlives the document. Integers outside `int64_t` become \ref BigInt. Keys keep the
 * library's order, and a repeated key keeps its first position and its last value, as Python's `json.loads` does.
 * A JSON null becomes an empty \ref Value (`none`). Errors:
 *  - `ErrorCode::ResourceLimitExceeded` past `limits`: ExtraParams are the message, the limit's name (`"maxDepth"`
 *    or `"maxSequenceSize"`) and the JSON path of the value;
 *  - `ErrorCode::DataReadError` when forward-only traits report an error: ExtraParams are the traits' message and the
 *    JSON path.
 *
 * The path, such as `$.items[3]["a b"]`, names the value where the copy stopped; `GetLocationDescr()` is `JSON`
 * followed by the path, and by `(char N)` when the traits report byte offsets (`JsonTraits::Location`). Exceptions
 * from the traits propagate.
 *
 * @param doc    The document; forward-only traits may take it by non-const reference and consume it
 * @param limits Bounds of the copy
 */
template<typename Json>
nonstd::expected<Value, ErrorInfo> ToValue(Json&& doc, const JsonCopyLimits& limits = {});

/*!
 * \brief A view of `node` that keeps `owner` alive
 *
 * For a subtree of a document held by a shared pointer, or a handle type (`JsonTraits::IsHandle`) whose storage
 * another object owns. Every item the view hands out shares `owner`.
 */
template<typename Json>
Value ReflectJson(std::shared_ptr<const void> owner, const Json& node);

namespace detail::json
{
//! A node, and the owner that keeps its document alive (empty when the caller does)
template<typename J>
struct Ref
{
    NodeOf<J> node;
    std::shared_ptr<const void> owner;
};

template<typename J>
Value MakeValue(const Ref<J>& ref);

template<typename J>
bool SameNode(const NodeOf<J>& a, const NodeOf<J>& b)
{
    if constexpr (HasSameNode<J>::value)
    {
        return Traits<J>::SameNode(a, b);
    }
    else
    {
        return a == b;
    }
}

template<typename S>
Value StringValue(S&& str)
{
    using D = std::decay_t<S>;
    if constexpr (std::is_same_v<D, std::string> || std::is_same_v<D, std::wstring>)
    {
        return Value(std::forward<S>(str));
    }
    else if constexpr (std::is_convertible_v<const D&, std::wstring_view>)
    {
        return Value(std::wstring(std::wstring_view(str)));
    }
    else
    {
        return Value(std::string(std::string_view(str)));
    }
}

template<typename S>
std::size_t StringLength(const S& str)
{
    if constexpr (std::is_convertible_v<const S&, std::wstring_view>)
    {
        return std::wstring_view(str).size();
    }
    else
    {
        return std::string_view(str).size();
    }
}

//! A JsonKind::BigInt from its text; an integer that fits `int64_t` is stored as one
inline std::optional<Value> BigIntValue(std::string_view text)
{
    BigInt big;
    if (!BigInt::TryParse(text, big))
    {
        return std::nullopt;
    }
    int64_t small = 0;
    if (big.ToInt64(small))
    {
        return Value(small);
    }
    return Value(std::move(big));
}

template<typename J>
class ObjectAccessor final : public IMapItemAccessor
{
public:
    explicit ObjectAccessor(Ref<J> ref)
        : m_ref(std::move(ref))
    {
    }

    [[nodiscard]] size_t GetSize() const override { return Traits<J>::Size(m_ref.node); }
    [[nodiscard]] bool Contains(std::string_view name) const override { return Traits<J>::Find(m_ref.node, name).has_value(); }
    [[nodiscard]] std::optional<Value> Find(std::string_view name) const override
    {
        auto child = Traits<J>::Find(m_ref.node, name);
        if (!child)
        {
            return std::nullopt;
        }
        return MakeValue<J>(Ref<J>{ *child, m_ref.owner });
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> keys;
        keys.reserve(Traits<J>::Size(m_ref.node));
        Traits<J>::ForEachMember(m_ref.node, [&keys](auto&& key, auto&&) {
            keys.emplace_back(std::string_view(key));
            return true;
        });
        return keys;
    }
    // A null member is none and a missing key undefined, as in Python
    [[nodiscard]] bool EmptyIsNone() const override { return true; }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ObjectAccessor*>(&other);
        return val && SameNode<J>(m_ref.node, val->m_ref.node);
    }

private:
    Ref<J> m_ref;
};

template<typename J>
class ArrayEnumerator final : public IListEnumerator
{
public:
    explicit ArrayEnumerator(Ref<J> ref)
        : m_ref(std::move(ref))
        , m_size(Traits<J>::Size(m_ref.node))
    {
    }

    void Reset() override
    {
        m_next = 0;
        m_cur = m_size;
    }
    bool MoveNext() override
    {
        if (m_next >= m_size)
        {
            m_cur = m_size;
            return false;
        }
        m_cur = m_next++;
        return true;
    }
    [[nodiscard]] Value GetCurrent() const override
    {
        if (m_cur >= m_size)
        {
            return Value();
        }
        return MakeValue<J>(Ref<J>{ Traits<J>::At(m_ref.node, m_cur), m_ref.owner });
    }
    [[nodiscard]] ListEnumeratorPtr Clone() const override { return ListEnumeratorPtr(types::in_place_type_t<ArrayEnumerator>{}, *this); }
    ListEnumeratorPtr Move() override { return ListEnumeratorPtr(types::in_place_type_t<ArrayEnumerator>{}, std::move(*this)); }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ArrayEnumerator*>(&other);
        return val && SameNode<J>(m_ref.node, val->m_ref.node) && m_cur == val->m_cur && m_next == val->m_next;
    }

private:
    Ref<J> m_ref; // keeps an owned document alive after the list is gone
    std::size_t m_size;
    std::size_t m_next = 0;
    std::size_t m_cur = m_size;
};

template<typename J>
class ArrayAccessor final : public IListItemAccessor
    , public IIndexBasedAccessor
{
public:
    explicit ArrayAccessor(Ref<J> ref)
        : m_ref(std::move(ref))
    {
    }

    [[nodiscard]] std::optional<size_t> GetSize() const override { return Traits<J>::Size(m_ref.node); }
    [[nodiscard]] const IIndexBasedAccessor* GetIndexer() const override { return this; }
    [[nodiscard]] std::optional<ListEnumeratorPtr> CreateEnumerator() const override
    {
        return ListEnumeratorPtr(types::in_place_type_t<ArrayEnumerator<J>>{}, m_ref);
    }
    // Bounds-checked: C++ callers reach it directly, not only through the engine
    [[nodiscard]] Value GetItemByIndex(int64_t idx) const override
    {
        if (idx < 0 || static_cast<std::uint64_t>(idx) >= Traits<J>::Size(m_ref.node))
        {
            return Value();
        }
        return MakeValue<J>(Ref<J>{ Traits<J>::At(m_ref.node, static_cast<std::size_t>(idx)), m_ref.owner });
    }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ArrayAccessor*>(&other);
        return val && SameNode<J>(m_ref.node, val->m_ref.node);
    }

private:
    Ref<J> m_ref;
};

template<typename J>
Value MakeValue(const Ref<J>& ref)
{
    using T = Traits<J>;
    switch (T::Kind(ref.node))
    {
    case JsonKind::Null:
        return Value();
    case JsonKind::Bool:
        return Value(T::AsBool(ref.node));
    case JsonKind::Int:
        return Value(static_cast<int64_t>(T::AsInt(ref.node)));
    case JsonKind::UInt:
        if constexpr (HasAsUInt<J>::value)
        {
            return Value(static_cast<std::uint64_t>(T::AsUInt(ref.node))); // a BigInt above INT64_MAX
        }
        else
        {
            return Value();
        }
    case JsonKind::Double:
        return Value(static_cast<double>(T::AsDouble(ref.node)));
    case JsonKind::String:
        return StringValue(T::AsString(ref.node));
    case JsonKind::BigInt:
        if constexpr (HasBigIntText<J>::value)
        {
            return BigIntValue(T::AsBigIntText(ref.node)).value_or(Value());
        }
        else
        {
            return Value();
        }
    case JsonKind::Array:
        return Value(GenericList([accessor = ArrayAccessor<J>(ref)]() { return &accessor; }));
    case JsonKind::Object:
        return Value(GenericMap([accessor = ObjectAccessor<J>(ref)]() { return &accessor; }));
    }
    return Value();
}

//! A step of the path to the value where ToValue stopped
struct PathStep
{
    std::string key;
    std::size_t index = 0;
    bool isKey = false;
};

//! The path in JSONPath notation: `$`, `.name` for an identifier key, `["a b"]` for another key, `[3]` for an index
inline std::string FormatPath(const std::vector<PathStep>& innermostFirst)
{
    std::string result = "$";
    for (auto step = innermostFirst.rbegin(); step != innermostFirst.rend(); ++step)
    {
        if (!step->isKey)
        {
            result += '[';
            result += std::to_string(step->index);
            result += ']';
            continue;
        }
        auto isAlpha = [](char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_'; };
        bool identifier = !step->key.empty() && isAlpha(step->key.front()) && std::all_of(step->key.begin(), step->key.end(), [&](char ch) { return isAlpha(ch) || (ch >= '0' && ch <= '9'); });
        if (identifier)
        {
            result += '.';
            result += step->key;
            continue;
        }
        result += "[\"";
        for (char ch : step->key)
        {
            if (ch == '"' || ch == '\\')
            {
                result += '\\';
            }
            result += ch;
        }
        result += "\"]";
    }
    return result;
}

// One forward pass over the document. A failure is recorded once; the frames between it and the root add their step
// of the path while they return, so a successful copy never builds a path
template<typename J>
class Copier
{
    using T = Traits<J>;
    using Node = NodeOf<J>;

public:
    explicit Copier(const JsonCopyLimits& limits)
        : m_cap(limits.maxSequenceSize != 0 ? std::min(limits.maxSequenceSize, JsonDefaultMaxSequenceSize) : JsonDefaultMaxSequenceSize)
        , m_maxDepth(limits.maxDepth)
    {
    }

    nonstd::expected<Value, ErrorInfo> Run(Node root)
    {
        Value result = Copy(root, 0);
        if (!m_failed)
        {
            return result;
        }
        std::string path = FormatPath(m_path);
        ErrorInfo::Data data;
        data.code = m_limit ? ErrorCode::ResourceLimitExceeded : ErrorCode::DataReadError;
        data.extraParams.emplace_back(std::move(m_message));
        if (m_limit)
        {
            data.extraParams.emplace_back(std::string(m_limit));
        }
        data.locationDescr = "JSON " + path;
        if (m_location)
        {
            data.locationDescr += " (char " + std::to_string(*m_location) + ")";
        }
        data.extraParams.emplace_back(std::move(path));
        return nonstd::make_unexpected(ErrorInfo(std::move(data)));
    }

private:
    void Fail(const Node& node, std::string message, const char* limit)
    {
        if (m_failed)
        {
            return;
        }
        m_failed = true;
        m_message = std::move(message);
        m_limit = limit;
        if constexpr (HasLocation<J>::value)
        {
            m_location = T::Location(node);
        }
    }

    // Stores the value of a traits call returning `T` or `JsonResult<T>`; false after a failure
    template<typename R, typename V>
    bool Load(R&& result, V& out, const Node& node)
    {
        if constexpr (IsResult<std::decay_t<R>>::value)
        {
            if (!result)
            {
                Fail(node, std::move(result.error()), nullptr);
                return false;
            }
            out = std::move(*result);
        }
        else
        {
            out = std::forward<R>(result);
        }
        return true;
    }

    // Runs a ForEach* call returning void, bool or JsonResult<void>
    template<typename Fn>
    void Iterate(const Node& node, const Fn& fn)
    {
        using R = decltype(fn());
        if constexpr (IsResult<R>::value)
        {
            auto result = fn();
            if (!result)
            {
                Fail(node, std::move(result.error()), nullptr);
            }
        }
        else
        {
            fn();
        }
    }

    template<typename F>
    void ForEachElement(const Node& node, const F& onItem)
    {
        if constexpr (HasForEachElement<J>::value)
        {
            Iterate(node, [&] { return T::ForEachElement(node, onItem); });
        }
        else
        {
            const std::size_t size = T::Size(node);
            for (std::size_t idx = 0; idx < size; ++idx)
            {
                if (!onItem(T::At(node, idx)))
                {
                    break;
                }
            }
        }
    }

    Value CopyArray(const Node& node, std::size_t depth)
    {
        ValuesList list;
        ForEachElement(node, [&](auto&& child) {
            if (list.size() >= m_cap)
            {
                Fail(node, "a JSON array has more items than maxSequenceSize (" + std::to_string(m_cap) + ")", "maxSequenceSize");
                return false;
            }
            const std::size_t idx = list.size();
            list.push_back(Copy(child, depth + 1));
            if (m_failed)
            {
                m_path.push_back(PathStep{ {}, idx, false });
                return false;
            }
            return true;
        });
        return Value(std::move(list));
    }

    Value CopyObject(const Node& node, std::size_t depth)
    {
        ValuesMap map;
        std::uint64_t members = 0;
        Iterate(node, [&] {
            return T::ForEachMember(node, [&](auto&& key, auto&& child) {
                if (m_failed)
                {
                    return false; // the traits went on after a stop
                }
                if (++members > m_cap)
                {
                    Fail(node, "a JSON object has more members than maxSequenceSize (" + std::to_string(m_cap) + ")", "maxSequenceSize");
                    return false;
                }
                std::string name{ std::string_view(key) };
                Value item = Copy(child, depth + 1);
                if (m_failed)
                {
                    m_path.push_back(PathStep{ std::move(name), 0, true });
                    return false;
                }
                map[std::move(name)] = std::move(item); // a repeated key: first position, last value
                return true;
            });
        });
        return Value(std::move(map));
    }

    Value Copy(const Node& node, std::size_t depth)
    {
        typename ResultValue<decltype(T::Kind(node))>::type kind{};
        if (!Load(T::Kind(node), kind, node))
        {
            return Value();
        }
        switch (kind)
        {
        case JsonKind::Array:
        case JsonKind::Object:
            if (depth >= m_maxDepth)
            {
                Fail(node, "JSON nests deeper than maxDepth (" + std::to_string(m_maxDepth) + ")", "maxDepth");
                return Value();
            }
            return kind == JsonKind::Array ? CopyArray(node, depth) : CopyObject(node, depth);
        default:
            return CopyScalar(node, kind);
        }
    }

    Value CopyScalar(const Node& node, JsonKind kind)
    {
        switch (kind)
        {
        case JsonKind::Null:
            return Value();
        case JsonKind::Bool:
        {
            bool val = false;
            return Load(T::AsBool(node), val, node) ? Value(val) : Value();
        }
        case JsonKind::Int:
        {
            std::int64_t val = 0;
            return Load(T::AsInt(node), val, node) ? Value(static_cast<int64_t>(val)) : Value();
        }
        case JsonKind::UInt:
            if constexpr (HasAsUInt<J>::value)
            {
                std::uint64_t val = 0;
                return Load(T::AsUInt(node), val, node) ? Value(val) : Value();
            }
            else
            {
                Fail(node, "the JSON traits report an unsigned integer and have no AsUInt", nullptr);
                return Value();
            }
        case JsonKind::Double:
        {
            double val = 0;
            return Load(T::AsDouble(node), val, node) ? Value(val) : Value();
        }
        case JsonKind::String:
            return CopyString(node);
        case JsonKind::BigInt:
            return CopyBigInt(node);
        default:
            break;
        }
        Fail(node, "the JSON traits report an unknown kind of value", nullptr);
        return Value();
    }

    Value CopyString(const Node& node)
    {
        typename ResultValue<decltype(T::AsString(node))>::type str{};
        if (!Load(T::AsString(node), str, node))
        {
            return Value();
        }
        if (StringLength(str) > m_cap)
        {
            Fail(node, "a JSON string is longer than maxSequenceSize (" + std::to_string(m_cap) + ")", "maxSequenceSize");
            return Value();
        }
        return StringValue(std::move(str));
    }

    Value CopyBigInt(const Node& node)
    {
        if constexpr (HasBigIntText<J>::value)
        {
            std::string_view text;
            if (!Load(T::AsBigIntText(node), text, node))
            {
                return Value();
            }
            if (text.size() > m_cap)
            {
                Fail(node, "a JSON integer is longer than maxSequenceSize (" + std::to_string(m_cap) + ")", "maxSequenceSize");
                return Value();
            }
            auto val = BigIntValue(text);
            if (!val)
            {
                Fail(node, "the JSON traits gave an integer that is not [-]digits", nullptr);
                return Value();
            }
            return std::move(*val);
        }
        else
        {
            Fail(node, "the JSON traits report a big integer and have no AsBigIntText", nullptr);
            return Value();
        }
    }

    std::uint64_t m_cap;
    std::size_t m_maxDepth;
    bool m_failed = false;
    std::string m_message;
    const char* m_limit = nullptr; // the limit passed, or null for a traits error
    std::optional<std::uint64_t> m_location;
    std::vector<PathStep> m_path; // innermost step first
};

template<typename Json>
using EnableReflector = std::enable_if_t<!std::is_reference_v<Json> && !std::is_const_v<Json> && HasCore<Json>::value>;
} // namespace detail::json

namespace detail
{

/*!
 * \brief Reflect for every type with JsonTraits
 *
 * A document taken by value or as an rvalue is owned by the view and shared by every copy of the \ref Value and
 * every item it hands out (a non-const lvalue is copied first); a const reference or a pointer borrows it, so the
 * caller keeps it alive; a `std::shared_ptr` shares it. A scalar document is copied out at once. A handle type
 * (`JsonTraits::IsHandle`) is always borrowed: use \ref ReflectJson to give it an owner. References and const types
 * are excluded: `Reflector<T&>` and `Reflector<const T&>` route here through `Reflector<T>`.
 */
template<typename Json>
struct Reflector<Json, json::EnableReflector<Json>>
{
    static_assert(IsJsonViewable<Json>,
                  "jinja2::Reflect needs JsonTraits<Json>::Size, At and Find, and every function returning a plain value; "
                  "this JSON type supports only jinja2::ToValue (a copy)");

    static Value Create(Json doc)
    {
        using T = json::Traits<Json>;
        if constexpr (json::IsHandle<Json>::value)
        {
            return json::MakeValue<Json>(json::Ref<Json>{ T::Root(doc), nullptr });
        }
        else
        {
            const JsonKind kind = T::Kind(T::Root(doc));
            if (kind != JsonKind::Array && kind != JsonKind::Object)
            {
                return json::MakeValue<Json>(json::Ref<Json>{ T::Root(doc), nullptr });
            }
            auto owned = std::make_shared<const Json>(std::move(doc));
            const auto& root = *owned;
            return json::MakeValue<Json>(json::Ref<Json>{ T::Root(root), std::move(owned) });
        }
    }

    static Value CreateFromPtr(const Json* doc)
    {
        if (!doc)
        {
            return Value();
        }
        return json::MakeValue<Json>(json::Ref<Json>{ json::Traits<Json>::Root(*doc), nullptr });
    }

    template<typename U, typename = std::enable_if_t<std::is_same_v<std::remove_const_t<U>, Json>>>
    static Value CreateFromPtr(std::shared_ptr<U> doc)
    {
        if (!doc)
        {
            return Value();
        }
        const Json& root = *doc;
        return json::MakeValue<Json>(json::Ref<Json>{ json::Traits<Json>::Root(root), std::shared_ptr<const void>(std::move(doc)) });
    }
};
} // namespace detail

// Root takes the document by reference (non-const for forward-only traits), so it is not forwarded
template<typename Json>
nonstd::expected<Value, ErrorInfo> ToValue(Json&& doc, const JsonCopyLimits& limits) // NOLINT(cppcoreguidelines-missing-std-forward)
{
    static_assert(IsJsonCopyable<Json>, "jinja2::ToValue needs JsonTraits<Json> with Kind, the As* getters, ForEachMember, and ForEachElement or Size and At");
    detail::json::Copier<Json> copier(limits);
    return copier.Run(detail::json::Traits<Json>::Root(doc));
}

template<typename Json>
Value ReflectJson(std::shared_ptr<const void> owner, const Json& node)
{
    static_assert(IsJsonViewable<Json>, "jinja2::ReflectJson needs JsonTraits<Json> that support the view (IsJsonViewable)");
    return detail::json::MakeValue<Json>(detail::json::Ref<Json>{ detail::json::Traits<Json>::Root(node), std::move(owner) });
}
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_JSON_ADAPTER_H
