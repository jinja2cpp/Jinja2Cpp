#ifndef JINJA2CPP_VALUE_H
#define JINJA2CPP_VALUE_H

#include <jinja2cpp/config.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/utils/ordered_map.h>
#include <jinja2cpp/value_ptr.h>

#include <atomic>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN

namespace jinja2
{
//! Python `None`: what a default-constructed \ref Value holds
struct NoneValue
{
};

inline bool operator==(const NoneValue& /*lhs*/, const NoneValue& /*rhs*/) noexcept
{
    return true;
}

inline bool operator!=(const NoneValue& /*lhs*/, const NoneValue& /*rhs*/) noexcept
{
    return false;
}

using EmptyValue JINJA2CPP_DEPRECATED("jinja2cpp-2: NoneValue") = NoneValue;

class Value;

/*!
 * \brief Interface to the generic dictionary type which maps string to some value
 *
 * An implementation overrides \ref GetSize, \ref GetKeys and the lookup: either \ref Find (and \ref Contains when
 * presence is cheaper to check than the value is to build), or the 1.x pair \ref HasValue and \ref GetValueByName.
 * Each pair has defaults that call the other, so both kinds of implementation answer every lookup; an implementation
 * that overrides neither gets `std::logic_error` from its first lookup.
 */
struct JINJA2CPP_EXPORT IMapItemAccessor : virtual IComparable
{
    //! Destructor
    ~IMapItemAccessor() override = default;

    //! Method is called to obtain number of items in the dictionary. Maximum possible size_t value means non-calculable size
    [[nodiscard]] virtual size_t GetSize() const = 0;

    /*!
     * \brief Looks up an item by name
     *
     * The default implementation calls \ref HasValue, then \ref GetValueByName.
     *
     * @param name Name of the item
     *
     * @return The item, or empty optional if the dictionary has no item with this name
     */
    [[nodiscard]] virtual std::optional<Value> Find(std::string_view name) const;
    /*!
     * \brief Checks whether the dictionary has an item with the given name
     *
     * The default implementation calls \ref HasValue.
     *
     * @param name Name of the item
     *
     * @return true if the item is present and false otherwise
     */
    [[nodiscard]] virtual bool Contains(std::string_view name) const;
    /*!
     * \brief Method is called to check presence of the item in the dictionary (1.x lookup)
     *
     * The default implementation calls \ref Find.
     *
     * @param name Name of the item
     *
     * @return true if item is present and false otherwise.
     */
    [[nodiscard]] virtual bool HasValue(const std::string& name) const;
    /*!
     * \brief Method is called for retrieving the value by specified name (1.x lookup)
     *
     * The default implementation calls \ref Find.
     *
     * @param name Name of the value to retrieve
     *
     * @return Requestd value or empty \ref Value if item is absent
     */
    [[nodiscard]] virtual Value GetValueByName(const std::string& name) const;
    /*!
     * \brief Method is called for retrieving collection of keys in the dictionary
     *
     * @return Collection of keys if any. Ordering of keys is unspecified.
     */
    [[nodiscard]] virtual std::vector<std::string> GetKeys() const = 0;
    /*!
     * \brief Tells what an empty \ref Value returned for a present item means
     *
     * Reflected struct fields return an empty \ref Value for "no value", which the template sees as undefined (the
     * default). A document whose items can hold a real null (the JSON bindings) overrides this to return true, and
     * then a present item that returns an empty \ref Value is `none` to the template, as in Python.
     */
    [[nodiscard]] virtual bool EmptyIsNone() const { return false; }
};

/*!
 * \brief Helper class for accessing maps specified by the \ref IMapItemAccessor interface
 *
 * In the \ref Value type can be stored either ValuesMap instance or GenericMap instance. ValuesMap is a simple
 * dictionary object that keeps insertion order. Rather than GenericMap is a more robust object which can provide
 * access to the different types of dictionary entities. GenericMap takes the \ref IMapItemAccessor interface instance
 * and uses it to access particular items in the dictionaries.
 */
class JINJA2CPP_EXPORT GenericMap
{
public:
    class Iterator;
    //! Input iterator over the `(key, value)` pairs, see \ref begin
    using iterator = Iterator;
    using const_iterator = Iterator;

    //! Default constructor
    GenericMap() = default;

    /*!
     * \brief Initializing constructor
     *
     * The only one way to get valid non-empty GeneridMap is to construct it with the specified \ref IMapItemAccessor
     * implementation provider. This provider is a functional object which returns pointer to the interface instance.
     *
     * @param accessor Functional object which returns pointer to the \ref IMapItemAccessor interface
     */
    explicit GenericMap(std::function<const IMapItemAccessor*()> accessor)
        : m_accessor(std::move(accessor))
    {
    }

    /*!
     * \brief Check the presence the specific item in the dictionary
     *
     * @param name Name of the the item
     *
     * @return true of item is present and false otherwise
     */
    [[nodiscard]] bool HasValue(std::string_view name) const
    {
        return m_accessor ? m_accessor()->Contains(name) : false;
    }

    /*!
     * \brief Get specific item from the dictionary
     *
     * @param name Name of the item to get
     *
     * @return Value of the item or empty \ref Value if no item
     */
    [[nodiscard]] Value GetValueByName(std::string_view name) const;
    /*!
     * \brief Looks up an item of the dictionary
     *
     * @param name Name of the item
     *
     * @return The item, or empty optional if there is no such item
     */
    [[nodiscard]] std::optional<Value> Find(std::string_view name) const;
    /*!
     * \brief Get size of the dictionary
     *
     * @return Size of the dictionary
     */
    [[nodiscard]] size_t GetSize() const
    {
        return m_accessor ? m_accessor()->GetSize() : 0;
    }
    /*!
     * \brief  Get collection of keys from the dictionary
     *
     * @return Collection of the keys or empty collection if no keys
     */
    [[nodiscard]] auto GetKeys() const
    {
        return m_accessor ? m_accessor()->GetKeys() : std::vector<std::string>();
    }
    /*!
     * \brief Get the underlying access interface to the dictionary
     *
     * @return Pointer to the underlying interface or nullptr if no
     */
    [[nodiscard]] const IMapItemAccessor* GetAccessor() const
    {
        return m_accessor ? m_accessor() : nullptr;
    }

    //! Same as \ref GetValueByName
    Value operator[](std::string_view name) const;

    /*!
     * \brief Get iterator to the first item of the dictionary
     *
     * Iterates over `std::pair<std::string, Value>` items in the order of \ref GetKeys (sorted for reflected
     * types), so `for (auto& [key, value] : map)` works. The keys are fetched once, here; each value is fetched when
     * the iterator reaches it. Iterators refer to this object and are invalidated when it is destroyed.
     *
     * @return Iterator to the first item or iterator equal to `end()` if the map is empty or not initialized
     */
    [[nodiscard]] iterator begin() const;
    //! Get the end iterator
    [[nodiscard]] iterator end() const; // NOLINT(readability-convert-member-functions-to-static): container API
    //! Same as \ref begin
    [[nodiscard]] const_iterator cbegin() const;
    //! Same as \ref end
    [[nodiscard]] const_iterator cend() const;

private:
    std::function<const IMapItemAccessor*()> m_accessor;
};

JINJA2CPP_EXPORT bool operator==(const GenericMap& lhs, const GenericMap& rhs);
JINJA2CPP_EXPORT bool operator!=(const GenericMap& lhs, const GenericMap& rhs);

using ValuesList = std::vector<Value>;
struct ValuesMap;
struct UserCallableArgs;
struct ParamInfo;
struct UserCallable;

//! Holds the recursive alternatives of \ref Value (\ref ValuesList, \ref ValuesMap, \ref UserCallable) by value
template<typename T>
using RecWrapper = detail::indirect<T>;

namespace detail
{
//! Character types: \ref Value refuses them, since a character is either a number or a one-letter string
template<typename T>
struct IsCharType
    : std::integral_constant<bool,
                             std::is_same_v<T, char> || std::is_same_v<T, wchar_t> || std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>
#ifdef __cpp_char8_t
                                 || std::is_same_v<T, char8_t>
#endif
                             >
{
};

//! Arithmetic types \ref Value stores as `int64_t`: every integral type except `bool` and the character types
template<typename T>
struct IsStoredAsInt : std::integral_constant<bool, std::is_integral_v<T> && !std::is_same_v<T, bool> && !IsCharType<T>::value>
{
};

//! Integral types with values outside the `int64_t` range \ref Value stores: `uint64_t`, `size_t` and wider
template<typename T>
inline constexpr bool CanExceedInt64 = std::numeric_limits<T>::digits > std::numeric_limits<int64_t>::digits;

//! Throws `jinja2::Exception{ErrorCode::IntegerOutOfRange}` (exception.h); out of line, so that callers keep only a call
[[noreturn]] JINJA2CPP_EXPORT void ThrowIntegerOutOfRange();

//! Whether an integral value fits the stored `int64_t`
template<typename T>
constexpr bool FitsStoredInt(T val) noexcept
{
    if constexpr (CanExceedInt64<T>)
    {
        bool fits = val <= static_cast<T>(std::numeric_limits<int64_t>::max());
        if constexpr (std::is_signed_v<T>)
        {
            fits = fits && val >= static_cast<T>(std::numeric_limits<int64_t>::min());
        }
        return fits;
    }
    else
    {
        static_cast<void>(val);
        return true;
    }
}

//! Converts an integral value to the stored `int64_t`; throws `jinja2::Exception{ErrorCode::IntegerOutOfRange}` for a
//! value outside its range
template<typename T>
constexpr int64_t ToStoredInt(T val) noexcept(!CanExceedInt64<T>)
{
    if constexpr (CanExceedInt64<T>)
    {
        if (!FitsStoredInt(val))
        {
            ThrowIntegerOutOfRange();
        }
    }
    return static_cast<int64_t>(val);
}
} // namespace detail

/*!
 * \brief Generic value class
 *
 * Variant-based class which is used for passing values to and from Jinja2C++ template engine. This class store the
 * following types of values:
 *
 *  - NoneValue: Python `None`, the default
 *  - Boolean value.
 *  - String value.
 *  - Wide string value
 *  - String view value (std::string_view)
 *  - Wide string view value (std::wstring_view)
 *  - integer (int64_t) value
 *  - floating point (double) value
 *  - Simple list of other values (\ref ValuesList)
 *  - Simple map of other values (\ref ValuesMap)
 *  - Generic list of other values (\ref GenericList)
 *  - Generic map of other values (\ref GenericMap)
 *  - User-defined callable (\ref UserCallable)
 *
 *  `IsX()` tests what is stored, `AsX()` and `As<T>()` return it (and throw `std::bad_variant_access` when the value
 *  holds something else), `bool AsX(out)` and `GetIf<T>()` do the same without throwing, and `ToString()` prints the
 *  value as `{{ value }}` does. \ref jinja2::Visit visits the stored object.
 */
class Value
{
public:
    using ValueData = std::variant<
        NoneValue,
        bool,
        std::string,
        std::wstring,
        std::string_view,
        std::wstring_view,
        int64_t,
        double,
        RecWrapper<ValuesList>,
        RecWrapper<ValuesMap>,
        GenericList,
        GenericMap,
        RecWrapper<UserCallable>>;

    template<typename T, typename... L>
    struct AnyOf : public std::false_type
    {
    };

    template<typename T, typename H, typename... L>
    struct AnyOf<T, H, L...> : public std::integral_constant<bool, std::is_same_v<std::decay_t<T>, H> || AnyOf<T, L...>::value>
    {
    };

    //! Default constructor
    Value();
    //! Copy constructor
    Value(const Value& val);
    //! Move constructor
    Value(Value&& val) noexcept;
    //! Desctructor
    // ~Value();
    ~Value();

    //! Assignment operator
    Value& operator=(const Value&);
    //! Move assignment operator
    Value& operator=(Value&&) noexcept;
    /*!
     * \brief Generic initializing constructor
     *
     * Creates \ref Value from the arbitrary type which is compatible with types listed in \ref Value::ValueData
     *
     * @tparam T  Type of value to create \ref Value instance from
     * @param val Value which should be used to initialize \ref Value instance
     */
    template<typename T>
    Value(T&& val, // NOLINT(google-explicit-constructor)
          std::enable_if_t<!AnyOf<T, Value, ValuesList, ValuesMap, UserCallable, std::nullptr_t>::value && (!std::is_integral_v<std::decay_t<T>> || std::is_same_v<std::decay_t<T>, bool>) && !std::is_floating_point_v<std::decay_t<T>>>* = nullptr)
        : m_data(std::forward<T>(val))
    {
    }
    /*!
     * \brief Initializing constructor from `nullptr`: makes `None`, as \ref Value()
     */
    Value(std::nullptr_t) noexcept // NOLINT(google-explicit-constructor)
    {
    }
    /*!
     * \brief Initializing constructor from pointer to the null-terminated narrow string
     *
     * @param val Null-terminated string which should be used to initialize \ref Value instance
     */
    Value(const char* val) // NOLINT(google-explicit-constructor)
        : m_data(std::string(val))
    {
    }
    /*!
     * \brief Initializing constructor from pointer to the null-terminated wide string
     *
     * @param val Null-terminated string which should be used to initialize \ref Value instance
     */
    Value(const wchar_t* val) // NOLINT(google-explicit-constructor)
        : m_data(std::wstring(val))
    {
    }
    /*!
     * \brief Initializing constructor from the narrow string literal
     *
     * @param val String literal which should be used to initialize \ref Value instance
     */
    template<size_t N>
    Value(char (&val)[N]) // NOLINT(google-explicit-constructor)
        : m_data(std::string(val))
    {
    }
    /*!
     * \brief Initializing constructor from the wide string literal
     *
     * @param val String literal which should be used to initialize \ref Value instance
     */
    template<size_t N>
    Value(wchar_t (&val)[N]) // NOLINT(google-explicit-constructor)
        : m_data(std::wstring(val))
    {
    }
    /*!
     * \brief Initializing constructor from any integral value except `bool` and characters: stores `int64_t`
     *
     * A value outside the `int64_t` range (a `uint64_t` or `size_t` above `INT64_MAX`) throws
     * `jinja2::Exception{ErrorCode::IntegerOutOfRange}` (a `std::runtime_error`) rather than wrapping; only types that
     * can hold such values may throw. \ref TryFrom is the non-throwing alternative.
     *
     * @param val Integer value which should be used to initialize \ref Value instance
     */
    template<typename I, std::enable_if_t<detail::IsStoredAsInt<I>::value>* = nullptr>
    Value(I val) noexcept(!detail::CanExceedInt64<I>) // NOLINT(google-explicit-constructor)
        : m_data(detail::ToStoredInt(val))
    {
    }
    /*!
     * \brief Non-throwing alternative to the integral constructor
     *
     * Stores `val` into `out` and returns `true` if it fits the stored `int64_t`; otherwise returns `false` and leaves
     * `out` unchanged.
     *
     * @param val Integer value which should be stored
     * @param out \ref Value which receives it
     * @return Whether `val` was stored
     */
    template<typename I, std::enable_if_t<detail::IsStoredAsInt<I>::value>* = nullptr>
    [[nodiscard]] static bool TryFrom(I val, Value& out) noexcept
    {
        if (!detail::FitsStoredInt(val))
        {
            return false;
        }
        out = Value(static_cast<int64_t>(val));
        return true;
    }
    /*!
     * \brief Construction from a character is deleted: say `Value(int64_t('c'))` or `Value(std::string(1, 'c'))`
     */
    template<typename C, std::enable_if_t<detail::IsCharType<C>::value>* = nullptr>
    Value(C val) = delete;
    /*!
     * \brief Initializing constructor from any floating-point value: stores `double`
     *
     * @param val Floating-point value which should be used to initialize \ref Value instance
     */
    template<typename F, std::enable_if_t<std::is_floating_point_v<F>>* = nullptr>
    Value(F val) noexcept // NOLINT(google-explicit-constructor)
        : m_data(static_cast<double>(val))
    {
    }
    /*!
     * \brief Initializing constructor from the \ref ValuesList
     *
     * @param list List of values which should be used to initialize \ref Value instance
     */
    Value(const ValuesList& list); // NOLINT(google-explicit-constructor)
    /*!
     * \brief Initializing constructor from the \ref ValuesMap
     *
     * @param map Map of values which should be used to initialize \ref Value instance
     */
    Value(const ValuesMap& map); // NOLINT(google-explicit-constructor)
    /*!
     * \brief Initializing constructor from the \ref UserCallable
     *
     * @param callable UserCallable which should be used to initialize \ref Value instance
     */
    Value(const UserCallable& callable); // NOLINT(google-explicit-constructor)
    /*!
     * \brief Initializing move constructor from the \ref ValuesList
     *
     * @param list List of values which should be used to initialize \ref Value instance
     */
    Value(ValuesList&& list); // NOLINT(google-explicit-constructor)
    /*!
     * \brief Initializing move constructor from the \ref ValuesMap
     *
     * @param map Map of values which should be used to initialize \ref Value instance
     */
    Value(ValuesMap&& map); // NOLINT(google-explicit-constructor)
    /*!
     * \brief Initializing move constructor from the \ref UserCallable
     *
     * @param callable UserCallable which should be used to initialize \ref Value instance
     */
    Value(UserCallable&& callable); // NOLINT(google-explicit-constructor)

    //! Test Value for holding `None` (\ref NoneValue, the default)
    [[nodiscard]] bool IsNone() const noexcept { return Holds<NoneValue>(); }
    //! Test Value for holding `bool`
    [[nodiscard]] bool IsBool() const noexcept { return Holds<bool>(); }
    //! Test Value for holding an integer (`int64_t`)
    [[nodiscard]] bool IsInt() const noexcept { return Holds<int64_t>(); }
    //! Test Value for holding a floating-point number (`double`)
    [[nodiscard]] bool IsDouble() const noexcept { return Holds<double>(); }
    //! Test Value for holding `std::string` (a `std::string_view` does not count)
    [[nodiscard]] bool IsString() const noexcept { return Holds<std::string>(); }
    //! Test Value for holding `std::wstring` (a `std::wstring_view` does not count)
    [[nodiscard]] bool IsWString() const noexcept { return Holds<std::wstring>(); }
    //! Test Value for holding a list: a \ref ValuesList or a \ref GenericList
    [[nodiscard]] bool IsList() const noexcept { return Holds<RecWrapper<ValuesList>>() || Holds<GenericList>(); }
    //! Test Value for holding a map: a \ref ValuesMap or a \ref GenericMap
    [[nodiscard]] bool IsMap() const noexcept { return Holds<RecWrapper<ValuesMap>>() || Holds<GenericMap>(); }
    //! Test Value for holding a \ref UserCallable
    [[nodiscard]] bool IsCallable() const noexcept { return Holds<RecWrapper<UserCallable>>(); }

    /*!
     * \brief Exact access to the stored value
     *
     * `AsX()` returns what is stored and throws `std::bad_variant_access` when the value holds something else.
     * The overload `bool AsX(out) noexcept` stores it in `out` instead and returns whether the value holds that
     * type: a view for strings, the value for scalars and a pointer for containers, so it never copies:
     * ```c++
     *  std::string_view s;
     *  if (v.AsString(s))
     *      use(s);
     * ```
     * A view or pointer is valid as long as the \ref Value is not changed; the view and pointer forms are deleted for
     * a temporary \ref Value, which would leave them dangling.
     */
    [[nodiscard]] bool AsBool() const { return As<bool>(); }
    //! Same as \ref AsBool(), without throwing
    bool AsBool(bool& out) const noexcept { return Load(out); }
    //! The stored `int64_t`; throws `std::bad_variant_access` for anything else
    [[nodiscard]] int64_t AsInt() const { return As<int64_t>(); }
    //! Same as \ref AsInt(), without throwing
    bool AsInt(int64_t& out) const noexcept { return Load(out); }
    //! The stored `double`; throws `std::bad_variant_access` for anything else
    [[nodiscard]] double AsDouble() const { return As<double>(); }
    //! Same as \ref AsDouble(), without throwing
    bool AsDouble(double& out) const noexcept { return Load(out); }
    //! The stored `std::string`; throws `std::bad_variant_access` for anything else
    [[nodiscard]] const std::string& AsString() const { return As<std::string>(); }
    //! The stored `std::string`, mutable; throws `std::bad_variant_access` for anything else
    std::string& AsString() { return As<std::string>(); }
    //! A view of the stored `std::string`, without throwing or copying
    bool AsString(std::string_view& out) const& noexcept { return LoadView<std::string>(out); }
    //! Deleted: the view would outlive the temporary it points into
    bool AsString(std::string_view& out) && = delete;
    //! The stored `std::wstring`; throws `std::bad_variant_access` for anything else
    [[nodiscard]] const std::wstring& AsWString() const { return As<std::wstring>(); }
    //! The stored `std::wstring`, mutable; throws `std::bad_variant_access` for anything else
    std::wstring& AsWString() { return As<std::wstring>(); }
    //! A view of the stored `std::wstring`, without throwing or copying
    bool AsWString(std::wstring_view& out) const& noexcept { return LoadView<std::wstring>(out); }
    //! Deleted: the view would outlive the temporary it points into
    bool AsWString(std::wstring_view& out) && = delete;
    //! The stored \ref ValuesList (not a \ref GenericList, see \ref ToGenericList); throws `std::bad_variant_access` for anything else
    [[nodiscard]] const ValuesList& AsList() const { return As<ValuesList>(); }
    //! The stored \ref ValuesList, mutable; throws `std::bad_variant_access` for anything else
    ValuesList& AsList() { return As<ValuesList>(); }
    //! A pointer to the stored \ref ValuesList, without throwing
    bool AsList(const ValuesList*& out) const& noexcept { return LoadPtr(out); }
    //! A mutable pointer to the stored \ref ValuesList, without throwing
    bool AsList(ValuesList*& out) & noexcept { return LoadPtr(out); }
    //! Deleted: the pointer would outlive the temporary it points into
    bool AsList(const ValuesList*& out) && = delete;
    //! The stored \ref ValuesMap (not a \ref GenericMap, see \ref ToGenericMap); throws `std::bad_variant_access` for anything else
    [[nodiscard]] const ValuesMap& AsMap() const { return As<ValuesMap>(); }
    //! The stored \ref ValuesMap, mutable; throws `std::bad_variant_access` for anything else
    ValuesMap& AsMap() { return As<ValuesMap>(); }
    //! A pointer to the stored \ref ValuesMap, without throwing
    bool AsMap(const ValuesMap*& out) const& noexcept { return LoadPtr(out); }
    //! A mutable pointer to the stored \ref ValuesMap, without throwing
    bool AsMap(ValuesMap*& out) & noexcept { return LoadPtr(out); }
    //! Deleted: the pointer would outlive the temporary it points into
    bool AsMap(const ValuesMap*& out) && = delete;
    //! The stored \ref UserCallable; throws `std::bad_variant_access` for anything else
    [[nodiscard]] const UserCallable& AsCallable() const { return As<UserCallable>(); }
    //! The stored \ref UserCallable, mutable; throws `std::bad_variant_access` for anything else
    UserCallable& AsCallable() { return As<UserCallable>(); }
    //! A pointer to the stored \ref UserCallable, without throwing
    bool AsCallable(const UserCallable*& out) const& noexcept { return LoadPtr(out); }
    //! A mutable pointer to the stored \ref UserCallable, without throwing
    bool AsCallable(UserCallable*& out) & noexcept { return LoadPtr(out); }
    //! Deleted: the pointer would outlive the temporary it points into
    bool AsCallable(const UserCallable*& out) && = delete;

    /*!
     * \brief A \ref GenericList over either list representation
     *
     * For a \ref ValuesList the result refers to the list stored here and is valid as long as this \ref Value is
     * not changed or destroyed. A value that is not a list gives an empty \ref GenericList (`!IsValid()`).
     */
    [[nodiscard]] JINJA2CPP_EXPORT GenericList ToGenericList() const&;
    //! Same as \ref ToGenericList, for a temporary: the result owns the list
    [[nodiscard]] JINJA2CPP_EXPORT GenericList ToGenericList() &&;
    //! A \ref GenericMap over either map representation, empty for a non-map; see \ref ToGenericList
    [[nodiscard]] JINJA2CPP_EXPORT GenericMap ToGenericMap() const&;
    //! Same as \ref ToGenericMap, for a temporary: the result owns the map
    [[nodiscard]] JINJA2CPP_EXPORT GenericMap ToGenericMap() &&;

    /*!
     * \brief Exact access by type, as `std::get`
     *
     * `T` is one of the alternatives of \ref ValueData, or \ref ValuesList, \ref ValuesMap or \ref UserCallable
     * (the alternative \ref RecWrapper holds). Throws `std::bad_variant_access` when the value holds something else.
     */
    template<typename T>
    [[nodiscard]] const T& As() const
    {
        if constexpr (IsWrapped<T>::value)
        {
            return *std::get<RecWrapper<T>>(m_data);
        }
        else
        {
            return std::get<T>(m_data);
        }
    }
    //! Mutable \ref As
    template<typename T>
    T& As()
    {
        if constexpr (IsWrapped<T>::value)
        {
            return *std::get<RecWrapper<T>>(m_data);
        }
        else
        {
            return std::get<T>(m_data);
        }
    }
    //! A pointer to the stored `T`, or `nullptr`, as `std::get_if`; `T` as for \ref As
    template<typename T>
    [[nodiscard]] const T* GetIf() const noexcept
    {
        if constexpr (IsWrapped<T>::value)
        {
            // A moved-from wrapper holds nothing: report it as absent rather than dereference it
            const auto* wrapper = std::get_if<RecWrapper<T>>(&m_data);
            return wrapper && !wrapper->valueless_after_move() ? &**wrapper : nullptr;
        }
        else
        {
            return std::get_if<T>(&m_data);
        }
    }
    //! Mutable \ref GetIf
    template<typename T>
    T* GetIf() noexcept
    {
        if constexpr (IsWrapped<T>::value)
        {
            auto* wrapper = std::get_if<RecWrapper<T>>(&m_data);
            return wrapper && !wrapper->valueless_after_move() ? &**wrapper : nullptr;
        }
        else
        {
            return std::get_if<T>(&m_data);
        }
    }

    /*!
     * \brief The stored variant
     *
     * Its container alternatives are \ref RecWrapper; \ref jinja2::Visit visits the unwrapped objects instead.
     */
    [[nodiscard]] const ValueData& GetData() const noexcept { return m_data; }
    //! Mutable \ref GetData
    ValueData& GetData() noexcept { return m_data; }

    /*!
     * \brief The value as `{{ value }}` prints it, Python `str()`
     *
     * `5`, `1.5`, `True`, `None`, `[1, 'a']`, `{'k': 2}`; a string prints as itself, converted to the narrow
     * encoding when it is wide (UTF-8, as \ref ConvertString converts it: a lone surrogate or out-of-range unit
     * becomes U+FFFD). A callable prints as nothing.
     */
    [[nodiscard]] JINJA2CPP_EXPORT std::string ToString() const;
    //! Same as \ref ToString(), appended to `out`: reuses its capacity
    JINJA2CPP_EXPORT void ToString(std::string& out) const;
    //! Same as \ref ToString(), in the wide encoding
    [[nodiscard]] JINJA2CPP_EXPORT std::wstring ToWString() const;
    //! Same as \ref ToWString(), appended to `out`
    JINJA2CPP_EXPORT void ToWString(std::wstring& out) const;
    /*!
     * \brief Same as \ref ToString(), written to `[first, last)` as `std::to_chars` does
     *
     * Returns the end of the written text and `std::errc()`, or `last` and `std::errc::value_too_large` when the
     * text does not fit (the buffer then holds an unspecified prefix of it). Writes no terminating zero.
     */
    JINJA2CPP_EXPORT std::to_chars_result ToChars(char* first, char* last) const noexcept;

    [[nodiscard]] bool IsEqual(const Value& rhs) const;

    // 1.x names, kept as one-line forwards until 3.0 (docs/api-2.0.md 5.1)

    JINJA2CPP_DEPRECATED("jinja2cpp-2: GetData")
    [[nodiscard]] const ValueData& data() const { return m_data; }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: GetData")
    ValueData& data() { return m_data; }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: IsString")
    [[nodiscard]] bool isString() const { return IsString(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: AsString")
    auto& asString() { return AsString(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: AsString")
    [[nodiscard]] auto& asString() const { return AsString(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: IsWString")
    [[nodiscard]] bool isWString() const { return IsWString(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: AsWString")
    auto& asWString() { return AsWString(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: AsWString")
    [[nodiscard]] auto& asWString() const { return AsWString(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: IsList")
    [[nodiscard]] bool isList() const { return IsList(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: AsList")
    auto& asList() { return AsList(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: AsList")
    [[nodiscard]] auto& asList() const { return AsList(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: IsMap")
    [[nodiscard]] bool isMap() const { return IsMap(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: AsMap")
    auto& asMap() { return AsMap(); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: AsMap")
    [[nodiscard]] auto& asMap() const { return AsMap(); }
    //! Returns a copy, as in 1.x; \ref As returns a reference
    template<typename T>
    JINJA2CPP_DEPRECATED("jinja2cpp-2: As")
    auto get() { return std::get<T>(m_data); }
    template<typename T>
    JINJA2CPP_DEPRECATED("jinja2cpp-2: As")
    [[nodiscard]] auto get() const { return std::get<T>(m_data); }
    template<typename T>
    JINJA2CPP_DEPRECATED("jinja2cpp-2: GetIf")
    auto getPtr() { return std::get_if<T>(&m_data); }
    template<typename T>
    JINJA2CPP_DEPRECATED("jinja2cpp-2: GetIf")
    auto getPtr() const { return std::get_if<T>(&m_data); }
    JINJA2CPP_DEPRECATED("jinja2cpp-2: IsNone")
    [[nodiscard]] bool isEmpty() const { return IsNone(); }

private:
    // ValuesList, ValuesMap and UserCallable are stored in a RecWrapper; As and GetIf unwrap it
    template<typename T>
    struct IsWrapped : std::integral_constant<bool, AnyOf<T, ValuesList, ValuesMap, UserCallable>::value>
    {
    };

    template<typename T>
    [[nodiscard]] bool Holds() const noexcept
    {
        return std::holds_alternative<T>(m_data);
    }
    template<typename T>
    bool Load(T& out) const noexcept
    {
        const auto* p = std::get_if<T>(&m_data);
        if (p)
        {
            out = *p;
        }
        return p != nullptr;
    }
    template<typename S>
    bool LoadView(std::basic_string_view<typename S::value_type>& out) const noexcept
    {
        const auto* p = std::get_if<S>(&m_data);
        if (p)
        {
            out = *p;
        }
        return p != nullptr;
    }
    template<typename T>
    bool LoadPtr(const T*& out) const noexcept
    {
        out = GetIf<T>();
        return out != nullptr;
    }
    template<typename T>
    bool LoadPtr(T*& out) noexcept
    {
        out = GetIf<T>();
        return out != nullptr;
    }

    ValueData m_data;
};

JINJA2CPP_EXPORT bool operator==(const Value& lhs, const Value& rhs);
JINJA2CPP_EXPORT bool operator!=(const Value& lhs, const Value& rhs);

/*!
 * \brief Simple map of values, keyed by name
 *
 * Iterates in insertion order, like a Python dict: a template sees the keys of a context dict, its repr and
 * `items()` in the order the C++ code added them. The interface follows `std::unordered_map` without the bucket
 * API, node handles, `hash_function`, `key_eq` and `get_allocator` (see \ref OrderedMap).
 */
struct ValuesMap : OrderedMap<std::string, Value>
{
    using OrderedMap::OrderedMap;
};


/*!
 * \brief Information about user-callable parameters passed from Jinja2 call context
 *
 * This structure prepared by the Jinja2C++ engine and filled by information about call parameters gathered from the
 * call context. See documentation for \ref UserCallable for detailed information
 *
 */
struct UserCallableParams
{
    //! Values of parameters mapped according to \ref UserCallable::argsInfo
    ValuesMap args;
    //! Values of extra positional args got from the call expression
    Value extraPosArgs;
    //! Values of extra named args got from the call expression
    Value extraKwArgs;
    //! Context object which provides access to the current variables set of the template
    Value context;
    bool paramsParsed = false;

    Value operator[](const std::string& paramName) const
    {
        auto p = args.find(paramName);
        if (p == args.end())
        {
            return Value();
        }

        return p->second;
    }
};

/*!
 * \brief Information about one argument of the user-defined callable
 *
 * This structure is used as a description of the user-callable argument. Information from this structure is used
 * by the Jinja2C++ engine to map actual call parameters to the expected ones by the user-defined callable.
 */
struct ArgInfo
{
    //! Name of the parameter which receives the extra positional arguments (Python `*args`)
    static constexpr char VarArgs[] = "*args";
    //! Name of the parameter which receives the extra keyword arguments (Python `**kwargs`)
    static constexpr char VarKwArgs[] = "**kwargs";
    //! Name of the parameter which receives the current template context
    static constexpr char Context[] = "*context";

    //! Name of the argument
    std::string paramName;
    //! Mandatory flag
    bool isMandatory;
    //! Default value for the argument
    Value defValue;

    ArgInfo(std::string name, bool isMandat = false, Value defVal = Value()) // NOLINT(google-explicit-constructor)
        : paramName(std::move(name))
        , isMandatory(isMandat)
        , defValue(std::move(defVal))
    {}
};

inline bool operator==(const ArgInfo& lhs, const ArgInfo& rhs)
{
    if (lhs.paramName != rhs.paramName)
    {
        return false;
    }
    if (lhs.isMandatory != rhs.isMandatory)
    {
        return false;
    }
    return lhs.defValue == rhs.defValue;
}

inline bool operator!=(const ArgInfo& lhs, const ArgInfo& rhs)
{
    return !(lhs == rhs);
}

template<typename T>
struct ArgInfoT : public ArgInfo
{
    using type = T;

    using ArgInfo::ArgInfo;
    ArgInfoT(const ArgInfo& info) // NOLINT(google-explicit-constructor)
        : ArgInfo(info)
    {
    }
    ArgInfoT(ArgInfo&& info) noexcept // NOLINT(google-explicit-constructor)
        : ArgInfo(std::move(info))
    {
    }
};

/*!
 * \brief User-callable descriptor
 *
 * This descriptor is used for description of the user-defined callables passed to the Jinja2C++ engine. Information
 * from this descriptor is used by the engine to properly parse and prepare of the call parameters and pass it to the
 * user-callable. For instance, such kind of user-defined callable passed as a parameter:
 * ```c++
 *  jinja2::UserCallable uc;
 *  uc.callable = [](auto& params)->jinja2::Value {
 *      auto str1 = params["str1"];
 *      auto str2 = params["str2"];
 *
 *      if (str1.IsString())
 *          return str1.AsString() + " " + str2.AsString();
 *
 *      return str1.AsWString() + L" " + str2.AsWString();
 *  };
 *  uc.argsInfo = {{"str1", true}, {"str2", true}};
 *  params["test"] = std::move(uc);
 * ```
 * This declaration defines user-defined callable which takes two named parameters: `str1` and `str2`. Further, it's
 * possible to call this user-defined callable from the Jinja2 template this way:
 * ```jinja2
 * {{ test('Hello', 'World!') }}
 * ```
 * or:
 * ```
 * {{ test(str2='World!', str1='Hello') }}
 * ```
 * Jinja2C++ engine maps actual call parameters according the information from \ref UserCallable::argsInfo field and
 * pass them as a \ref UserCallableParams structure. Every named param (explicitly defined in the call or it's default value)
 * passed throught \ref UserCallableParams::args field. Every extra positional param mentoined in call passed as \ref
 * UserCallableParams::extraPosArgs. Every extra named param mentoined in call passed as \ref
 * UserCallableParams::extraKwArgs.
 *
 * If any of argument, marked as `mandatory` in the \ref UserCallable::argsInfo field is missed in the point of the
 * user-defined call the call is failed.
 */
struct JINJA2CPP_EXPORT UserCallable
{
    //! Type of the functional object which handles the call
    using Function = std::function<Value(const UserCallableParams&)>;
    using UserCallableFunctionPtr JINJA2CPP_DEPRECATED("jinja2cpp-2: UserCallable::Function") = Function;

    UserCallable()
        : m_counter(++m_gen) {}
    UserCallable(Function fptr, const std::vector<ArgInfo>& argsInfos)
        : callable(std::move(fptr))
        , argsInfo(argsInfos)
        , m_counter(++m_gen)
    {
    }
    UserCallable(const UserCallable& other) = default;
    UserCallable& operator=(const UserCallable& other)
    {
        if (*this == other)
        {
            return *this;
        }
        UserCallable temp(other);

        using std::swap;
        swap(callable, temp.callable);
        swap(argsInfo, temp.argsInfo);
        swap(m_counter, temp.m_counter);

        return *this;
    }

    UserCallable(UserCallable&& other) noexcept
        : callable(std::move(other.callable))
        , argsInfo(std::move(other.argsInfo))
        , m_counter(other.m_counter)
    {
    }

    UserCallable& operator=(UserCallable&& other) noexcept
    {
        callable = std::move(other.callable);
        argsInfo = std::move(other.argsInfo);
        m_counter = other.m_counter;

        return *this;
    }
    ~UserCallable() = default;

    [[nodiscard]] bool IsEqual(const UserCallable& other) const
    {
        return m_counter == other.m_counter;
    }

    //! Functional object which is actually handle the call
    Function callable;
    //! Information about arguments of the user-defined callable
    std::vector<ArgInfo> argsInfo;

private:
    static std::atomic_uint64_t m_gen; // NOLINT(readability-identifier-naming): include/ follows 0056
    uint64_t m_counter{};
};

bool operator==(const UserCallable& lhs, const UserCallable& rhs);
bool operator!=(const UserCallable& lhs, const UserCallable& rhs);


inline Value::Value(const UserCallable& callable)
    : m_data(RecWrapper<UserCallable>(xyz::in_place_t{}, callable))
{
}

inline Value::Value(UserCallable&& callable)
    : m_data(RecWrapper<UserCallable>(xyz::in_place_t{}, std::move(callable)))
{
}

inline std::optional<Value> GenericMap::Find(std::string_view name) const
{
    return m_accessor ? m_accessor()->Find(name) : std::nullopt;
}
inline Value GenericMap::GetValueByName(std::string_view name) const
{
    auto value = Find(name);
    return value ? std::move(*value) : Value();
}
inline Value GenericMap::operator[](std::string_view name) const
{
    return GetValueByName(name);
}

/*!
 * \brief Input iterator over the items of a \ref GenericMap
 *
 * Dereferences to `std::pair<std::string, Value>`. Two iterators compare equal when both are at the end or both point
 * to the same position of the same `begin()` call.
 */
class GenericMap::Iterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::pair<std::string, Value>;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type&;
    using pointer = const value_type*;

    Iterator() = default;

    reference operator*() const { return m_current; }
    pointer operator->() const { return &m_current; }

    Iterator& operator++()
    {
        ++m_idx;
        Load();
        return *this;
    }
    Iterator operator++(int)
    {
        Iterator result(*this);
        ++*this;
        return result;
    }

    bool operator==(const Iterator& other) const
    {
        if (AtEnd() || other.AtEnd())
        {
            return AtEnd() == other.AtEnd();
        }
        return m_keys == other.m_keys && m_idx == other.m_idx;
    }
    bool operator!=(const Iterator& other) const { return !(*this == other); }

private:
    friend class GenericMap;

    Iterator(const GenericMap* map, std::vector<std::string> keys)
        : m_map(map)
        , m_keys(std::make_shared<const std::vector<std::string>>(std::move(keys)))
    {
        Load();
    }

    [[nodiscard]] bool AtEnd() const { return !m_keys || m_idx >= m_keys->size(); }

    void Load()
    {
        if (AtEnd())
        {
            m_current = value_type();
            return;
        }
        const auto& key = (*m_keys)[m_idx];
        m_current = value_type(key, m_map->GetValueByName(key));
    }

    const GenericMap* m_map = nullptr;
    std::shared_ptr<const std::vector<std::string>> m_keys;
    size_t m_idx = 0;
    value_type m_current;
};

inline GenericMap::iterator GenericMap::begin() const
{
    return m_accessor ? Iterator(this, GetKeys()) : Iterator();
}
inline GenericMap::iterator GenericMap::end() const // NOLINT(readability-convert-member-functions-to-static): container API
{
    return Iterator();
}
inline GenericMap::const_iterator GenericMap::cbegin() const
{
    return begin();
}
inline GenericMap::const_iterator GenericMap::cend() const
{
    return end();
}

inline Value::Value() = default;
inline Value::Value(const Value& val) = default;
inline Value::Value(Value&& val) noexcept
    : m_data(std::move(val.m_data))
{
}
inline Value::~Value() = default;
inline Value& Value::operator=(const Value&) = default;
inline Value& Value::operator=(Value&& val) noexcept
{
    if (this == &val)
    {
        return *this;
    }

    m_data.swap(val.m_data);
    return *this;
}
inline Value::Value(const ValuesMap& map)
    : m_data(RecWrapper<ValuesMap>(xyz::in_place_t{}, map))
{
}
inline Value::Value(const ValuesList& list)
    : m_data(RecWrapper<ValuesList>(xyz::in_place_t{}, list))
{
}
inline Value::Value(ValuesList&& list)
    : m_data(RecWrapper<ValuesList>(xyz::in_place_t{}, std::move(list)))
{
}
inline Value::Value(ValuesMap&& map)
    : m_data(RecWrapper<ValuesMap>(xyz::in_place_t{}, std::move(map)))
{
}

//! Same as \ref Value::ToString(): the value as `{{ value }}` prints it
inline std::string ToString(const Value& val)
{
    return val.ToString();
}

//! Same as \ref Value::ToWString()
inline std::wstring ToWString(const Value& val)
{
    return val.ToWString();
}

//! Same as `ToString(Value(val))` for a number or `bool`: exact, so a literal 0 never looks like a null string;
//! throws `jinja2::Exception{ErrorCode::IntegerOutOfRange}` for an integer outside the `int64_t` range, as \ref Value does
template<typename T, std::enable_if_t<std::is_arithmetic_v<T> && !detail::IsCharType<T>::value>* = nullptr>
std::string ToString(T val)
{
    return Value(val).ToString();
}

//! Same as `ToWString(Value(val))` for a number or `bool`; throws as \ref ToString(T) does
template<typename T, std::enable_if_t<std::is_arithmetic_v<T> && !detail::IsCharType<T>::value>* = nullptr>
std::wstring ToWString(T val)
{
    return Value(val).ToWString();
}

//! `None`, as `{{ none }}` prints
inline std::string ToString(std::nullptr_t /*val*/)
{
    return "None";
}

//! `None`, as `{{ none }}` prints
inline std::wstring ToWString(std::nullptr_t /*val*/)
{
    return L"None";
}

namespace detail
{
template<typename T>
struct IsRecWrapper : std::false_type
{
};

template<typename T>
struct IsRecWrapper<RecWrapper<T>> : std::true_type
{
};

template<typename Fn, typename Data>
decltype(auto) VisitUnwrapped(Fn&& fn, Data& data)
{
    // std::visit calls the lambda once, so fn is forwarded once
    return std::visit(
        [&fn](auto& alternative) -> decltype(auto) {
            if constexpr (IsRecWrapper<std::remove_const_t<std::remove_reference_t<decltype(alternative)>>>::value)
            {
                return std::forward<Fn>(fn)(*alternative);
            }
            else
            {
                return std::forward<Fn>(fn)(alternative);
            }
        },
        data);
}
} // namespace detail

/*!
 * \brief Calls `fn` with the object stored in `val`, as `std::visit` does
 *
 * Containers and callables come unwrapped: `fn` receives a \ref ValuesList, \ref ValuesMap or \ref UserCallable,
 * never a \ref RecWrapper. Every call of `fn` must return the same type.
 */
template<typename Fn>
decltype(auto) Visit(Fn&& fn, const Value& val)
{
    return detail::VisitUnwrapped(std::forward<Fn>(fn), val.GetData());
}

//! Mutable \ref Visit: `fn` receives non-const references
template<typename Fn>
decltype(auto) Visit(Fn&& fn, Value& val)
{
    return detail::VisitUnwrapped(std::forward<Fn>(fn), val.GetData());
}

} // namespace jinja2

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END

#if (__cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)) && __has_include(<format>)
#include <format>
#endif

#ifdef __cpp_lib_format
//! Formats a \ref jinja2::Value as `{{ value }}` prints it; takes the string format spec (`{:>10}`)
template<>
struct std::formatter<jinja2::Value, char> : std::formatter<std::string_view, char>
{
    template<typename FormatContext>
    auto format(const jinja2::Value& val, FormatContext& ctx) const
    {
        std::string text;
        val.ToString(text);
        return std::formatter<std::string_view, char>::format(text, ctx);
    }
};
#endif

#endif // JINJA2CPP_VALUE_H
