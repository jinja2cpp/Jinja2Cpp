#ifndef JINJA2CPP_VALUE_H
#define JINJA2CPP_VALUE_H

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value_ptr.h>

#include <variant>
#include <optional>
#include <string_view>

#include <atomic>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace jinja2
{
//! Empty value container
struct EmptyValue
{
    template<typename T>
    operator T() const { return T{}; }
};

inline bool operator==(const EmptyValue& lhs, const EmptyValue& rhs)
{
    (void)lhs;
    (void)rhs;
    return true;
}

class Value;

/*!
 * \brief Interface to the generic dictionary type which maps string to some value
 */
struct IMapItemAccessor : virtual IComparable
{
    //! Destructor
    ~IMapItemAccessor() override = default;

    //! Method is called to obtain number of items in the dictionary. Maximum possible size_t value means non-calculable size
    virtual size_t GetSize() const = 0;

    /*!
     * \brief Method is called to check presence of the item in the dictionary
     *
     * @param name Name of the item
     *
     * @return true if item is present and false otherwise.
     */
    virtual bool HasValue(const std::string& name) const = 0;
    /*!
     * \brief Method is called for retrieving the value by specified name
     *
     * @param name Name of the value to retrieve
     *
     * @return Requestd value or empty \ref Value if item is absent
     */
    virtual Value GetValueByName(const std::string& name) const = 0;
    /*!
     * \brief Method is called for retrieving collection of keys in the dictionary
     *
     * @return Collection of keys if any. Ordering of keys is unspecified.
     */
    virtual std::vector<std::string> GetKeys() const = 0;
};

/*!
 * \brief Helper class for accessing maps specified by the \ref IMapItemAccessor interface
 *
 * In the \ref Value type can be stored either ValuesMap instance or GenericMap instance. ValuesMap is a simple
 * dictionary object based on std::unordered_map. Rather than GenericMap is a more robust object which can provide
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
    bool HasValue(const std::string& name) const
    {
        return m_accessor ? m_accessor()->HasValue(name) : false;
    }

    /*!
     * \brief Get specific item from the dictionary
     *
     * @param name Name of the item to get
     *
     * @return Value of the item or empty \ref Value if no item
     */
    Value GetValueByName(const std::string& name) const;
    /*!
     * \brief Get size of the dictionary
     *
     * @return Size of the dictionary
     */
    size_t GetSize() const
    {
        return m_accessor ? m_accessor()->GetSize() : 0;
    }
    /*!
     * \brief  Get collection of keys from the dictionary
     *
     * @return Collection of the keys or empty collection if no keys
     */
    auto GetKeys() const
    {
        return m_accessor ? m_accessor()->GetKeys() : std::vector<std::string>();
    }
    /*!
     * \brief Get the underlying access interface to the dictionary
     *
     * @return Pointer to the underlying interface or nullptr if no
     */
    const IMapItemAccessor* GetAccessor() const
    {
        return m_accessor ? m_accessor() : nullptr;
    }

    auto operator[](const std::string& name) const;

    /*!
     * \brief Get iterator to the first item of the dictionary
     *
     * Iterates over `std::pair<std::string, Value>` items in the order of \ref GetKeys (unspecified for reflected
     * types), so `for (auto& [key, value] : map)` works. The keys are fetched once, here; each value is fetched when
     * the iterator reaches it. Iterators refer to this object and are invalidated when it is destroyed.
     *
     * @return Iterator to the first item or iterator equal to `end()` if the map is empty or not initialized
     */
    iterator begin() const;
    //! Get the end iterator
    iterator end() const;
    //! Same as \ref begin
    const_iterator cbegin() const;
    //! Same as \ref end
    const_iterator cend() const;

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

template<typename T>
using RecWrapper = types::ValuePtr<T>;

/*!
 * \brief Generic value class
 *
 * Variant-based class which is used for passing values to and from Jinja2C++ template engine. This class store the
 * following types of values:
 *
 *  - EmptyValue. In this case instance of this class threated as 'empty'
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
 *  Exact value can be accessed via std::visit method applied to the result of the Value::data() call or any of
 *  asXXX method (ex. \ref Value::asString). In case of string retrieval it's better to use \ref AsString or \ref
 *  AsWString functions. Thay hide all nececcary transformations between various types of strings (or string views).
 */
class Value
{
public:
    using ValueData = std::variant<
        EmptyValue,
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
    Value(T&& val, std::enable_if_t<!AnyOf<T, Value, ValuesList, ValuesMap, UserCallable>::value>* = nullptr)
        : m_data(std::forward<T>(val))
    {
    }
    /*!
     * \brief Initializing constructor from pointer to the null-terminated narrow string
     *
     * @param val Null-terminated string which should be used to initialize \ref Value instance
     */
    Value(const char* val)
        : m_data(std::string(val))
    {
    }
    /*!
     * \brief Initializing constructor from pointer to the null-terminated wide string
     *
     * @param val Null-terminated string which should be used to initialize \ref Value instance
     */
    Value(const wchar_t* val)
        : m_data(std::wstring(val))
    {
    }
    /*!
     * \brief Initializing constructor from the narrow string literal
     *
     * @param val String literal which should be used to initialize \ref Value instance
     */
    template<size_t N>
    Value(char (&val)[N])
        : m_data(std::string(val))
    {
    }
    /*!
     * \brief Initializing constructor from the wide string literal
     *
     * @param val String literal which should be used to initialize \ref Value instance
     */
    template<size_t N>
    Value(wchar_t (&val)[N])
        : m_data(std::wstring(val))
    {
    }
    /*!
     * \brief Initializing constructor from the int value
     *
     * @param val Integer value which should be used to initialize \ref Value instance
     */
    Value(int val)
        : m_data(static_cast<int64_t>(val))
    {
    }
    /*!
     * \brief Initializing constructor from the float value
     *
     * @param val Float value which should be used to initialize \ref Value instance
     */
    Value(float val)
        : m_data(static_cast<double>(val))
    {
    }
    /*!
     * \brief Initializing constructor from the double value
     *
     * @param val Double value which should be used to initialize \ref Value instance
     */
    Value(double val)
        : m_data(static_cast<double>(val))
    {
    }
    /*!
     * \brief Initializing constructor from the \ref ValuesList
     *
     * @param list List of values which should be used to initialize \ref Value instance
     */
    Value(const ValuesList& list);
    /*!
     * \brief Initializing constructor from the \ref ValuesMap
     *
     * @param map Map of values which should be used to initialize \ref Value instance
     */
    Value(const ValuesMap& map);
    /*!
     * \brief Initializing constructor from the \ref UserCallable
     *
     * @param callable UserCallable which should be used to initialize \ref Value instance
     */
    Value(const UserCallable& callable);
    /*!
     * \brief Initializing move constructor from the \ref ValuesList
     *
     * @param list List of values which should be used to initialize \ref Value instance
     */
    Value(ValuesList&& list) noexcept;
    /*!
     * \brief Initializing move constructor from the \ref ValuesMap
     *
     * @param map Map of values which should be used to initialize \ref Value instance
     */
    Value(ValuesMap&& map) noexcept;
    /*!
     * \brief Initializing move constructor from the \ref UserCallable
     *
     * @param callable UserCallable which should be used to initialize \ref Value instance
     */
    Value(UserCallable&& callable);

    /*!
     * \brief Get the non-mutable stored data object
     *
     * Returns the stored data object in order to get the typed value from it. For instance:
     * ```c++
     *  inline std::string AsString(const jinja2::Value& val)
     *  {
     *      return std::visit(StringGetter(), val.data());
     *  }
     *  ```
     *
     * @return Non-mutable stored data object
     */
    const ValueData& data() const { return m_data; }
    /*!
     * \brief Get the mutable stored data object
     *
     * Returns the stored data object in order to get the typed value from it. For instance:
     * ```c++
     *  inline std::string AsString(Value& val)
     *  {
     *      return std::visit(StringGetter(), val.data());
     *  }
     *  ```
     *
     * @return Mutable stored data object
     */
    ValueData& data() { return m_data; }

    //! Test Value for containing std::string object
    bool isString() const
    {
        return std::get_if<std::string>(&m_data) != nullptr;
    }
    /*!
     * \brief Returns mutable containing std::string object
     *
     * Returns containing std::string object. Appropriate exception is thrown in case non-string containing value
     *
     * @return Mutable containing std::string object
     */
    auto& asString()
    {
        return std::get<std::string>(m_data);
    }
    /*!
     * \brief Returns non-mutable containing std::string object
     *
     * Returns containing std::string object. Appropriate exception is thrown in case of non-string containing value
     *
     * @return Non-mutable containing std::string object
     */
    auto& asString() const
    {
        return std::get<std::string>(m_data);
    }

    //! Test Value for containing std::wstring object
    bool isWString() const
    {
        return std::get_if<std::wstring>(&m_data) != nullptr;
    }
    /*!
     * \brief Returns mutable containing std::wstring object
     *
     * Returns containing std::wstring object. Appropriate exception is thrown in case of non-wstring containing value
     *
     * @return Mutable containing std::wstring object
     */
    auto& asWString()
    {
        return std::get<std::wstring>(m_data);
    }
    /*!
     * \brief Returns non-mutable containing std::wstring object
     *
     * Returns containing std::wstring object. Appropriate exception is thrown in case of non-wstring containing value
     *
     * @return Non-mutable containing std::wstring object
     */
    auto& asWString() const
    {
        return std::get<std::wstring>(m_data);
    }

    //! Test Value for containing jinja2::ValuesList object
    bool isList() const
    {
        return std::get_if<RecWrapper<ValuesList>>(&m_data) != nullptr || std::get_if<GenericList>(&m_data) != nullptr;
    }
    /*!
     * \brief Returns mutable containing jinja2::ValuesList object
     *
     * Returns containing jinja2::ValuesList object. Appropriate exception is thrown in case of non-Valueslist containing value
     *
     * @return Mutable containing jinja2::ValuesList object
     */
    auto& asList()
    {
        return *std::get<RecWrapper<ValuesList>>(m_data);
    }
    /*!
     * \brief Returns non-mutable containing jinja2::ValuesList object
     *
     * Returns containing jinja2::ValuesList object. Appropriate exception is thrown in case of non-Valueslist containing value
     *
     * @return Non-mutable containing jinja2::ValuesList object
     */
    auto& asList() const
    {
        return *std::get<RecWrapper<ValuesList>>(m_data);
    }
    //! Test Value for containing jinja2::ValuesMap object
    bool isMap() const
    {
        return std::get_if<RecWrapper<ValuesMap>>(&m_data) != nullptr || std::get_if<GenericMap>(&m_data) != nullptr;
    }
    /*!
     * \brief Returns mutable containing jinja2::ValuesMap object
     *
     * Returns containing jinja2::ValuesMap object. Appropriate exception is thrown in case of non-ValuesMap containing value
     *
     * @return Mutable containing jinja2::ValuesMap object
     */
    auto& asMap()
    {
        return *std::get<RecWrapper<ValuesMap>>(m_data);
    }
    /*!
     * \brief Returns non-mutable containing jinja2::ValuesMap object
     *
     * Returns containing jinja2::ValuesMap object. Appropriate exception is thrown in case of non-ValuesMap containing value
     *
     * @return Non-mutable containing jinja2::ValuesMap object
     */
    auto& asMap() const
    {
        return *std::get<RecWrapper<ValuesMap>>(m_data);
    }

    template<typename T>
    auto get()
    {
        return std::get<T>(m_data);
    }

    template<typename T>
    auto get() const
    {
        return std::get<T>(m_data);
    }

    template<typename T>
    auto getPtr()
    {
        return std::get_if<T>(&m_data); // m_data.index() == ValueData::template index_of<T>() ? &m_data.get<T>() : nullptr;
    }

    template<typename T>
    auto getPtr() const
    {
        return std::get_if<T>(&m_data); // m_data.index() == ValueData::template index_of<T>() ? &m_data.get<T>() : nullptr;
    }

    //! Test Value for emptyness
    bool isEmpty() const
    {
        return std::get_if<EmptyValue>(&m_data) != nullptr;
    }

    bool IsEqual(const Value& rhs) const;

private:
    ValueData m_data;
};

JINJA2CPP_EXPORT bool operator==(const Value& lhs, const Value& rhs);
JINJA2CPP_EXPORT bool operator!=(const Value& lhs, const Value& rhs);
bool operator==(const types::ValuePtr<Value>& lhs, const types::ValuePtr<Value>& rhs);
bool operator!=(const types::ValuePtr<Value>& lhs, const types::ValuePtr<Value>& rhs);
bool operator==(const types::ValuePtr<std::vector<Value>>& lhs, const types::ValuePtr<std::vector<Value>>& rhs);
bool operator!=(const types::ValuePtr<std::vector<Value>>& lhs, const types::ValuePtr<std::vector<Value>>& rhs);

struct ValuesMap : std::unordered_map<std::string, Value>
{
    using unordered_map::unordered_map;
};

bool operator==(const types::ValuePtr<ValuesMap>& lhs, const types::ValuePtr<ValuesMap>& rhs);
bool operator!=(const types::ValuePtr<ValuesMap>& lhs, const types::ValuePtr<ValuesMap>& rhs);

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
            return Value();

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

    ArgInfo(std::string name, bool isMandat = false, Value defVal = Value())
        : paramName(std::move(name))
        , isMandatory(isMandat)
        , defValue(std::move(defVal)) {}
};

inline bool operator==(const ArgInfo& lhs, const ArgInfo& rhs)
{
    if (lhs.paramName != rhs.paramName)
        return false;
    if (lhs.isMandatory != rhs.isMandatory)
        return false;
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
    ArgInfoT(const ArgInfo& info)
        : ArgInfo(info)
    {
    }
    ArgInfoT(ArgInfo&& info) noexcept
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
 *      if (str1.isString())
 *          return str1.asString() + " " + str2.asString();
 *
 *      return str1.asWString() + L" " + str2.asWString();
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
    using UserCallableFunctionPtr [[deprecated("jinja2cpp-2: use UserCallable::Function")]] = Function;

    UserCallable()
        : m_counter(++m_gen) {}
    UserCallable(const Function& fptr, const std::vector<ArgInfo>& argsInfos)
        : callable(fptr)
        , argsInfo(argsInfos)
        , m_counter(++m_gen)
    {
    }
    UserCallable(const UserCallable& other) = default;
    UserCallable& operator=(const UserCallable& other)
    {
        if (*this == other)
            return *this;
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

    bool IsEqual(const UserCallable& other) const
    {
        return m_counter == other.m_counter;
    }

    //! Functional object which is actually handle the call
    Function callable;
    //! Information about arguments of the user-defined callable
    std::vector<ArgInfo> argsInfo;

private:
    static std::atomic_uint64_t m_gen;
    uint64_t m_counter{};
};

bool operator==(const UserCallable& lhs, const UserCallable& rhs);
bool operator!=(const UserCallable& lhs, const UserCallable& rhs);
bool operator==(const types::ValuePtr<UserCallable>& lhs, const types::ValuePtr<UserCallable>& rhs);
bool operator!=(const types::ValuePtr<UserCallable>& lhs, const types::ValuePtr<UserCallable>& rhs);


inline Value::Value(const UserCallable& callable)
    : m_data(types::ValuePtr<UserCallable>(xyz::in_place_type_t<UserCallable>{}, callable))
{
}

inline Value::Value(UserCallable&& callable)
    : m_data(types::ValuePtr<UserCallable>(std::move(callable)))
{
}

inline Value GenericMap::GetValueByName(const std::string& name) const
{
    return m_accessor ? m_accessor()->GetValueByName(name) : Value();
}
inline auto GenericMap::operator[](const std::string& name) const
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
            return AtEnd() == other.AtEnd();
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

    bool AtEnd() const { return !m_keys || m_idx >= m_keys->size(); }

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
inline GenericMap::iterator GenericMap::end() const
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
        return *this;

    m_data.swap(val.m_data);
    return *this;
}
inline Value::Value(const ValuesMap& map)
    : m_data(types::ValuePtr<ValuesMap>(types::in_place_type_t<ValuesMap>{}, map))
{
}
inline Value::Value(const ValuesList& list)
    : m_data(types::ValuePtr<ValuesList>(types::in_place_type_t<ValuesList>{}, list))
{
}
inline Value::Value(ValuesList&& list) noexcept
    : m_data(types::ValuePtr<ValuesList>(std::move(list)))
{
}
inline Value::Value(ValuesMap&& map) noexcept
    : m_data(types::ValuePtr<ValuesMap>(std::move(map)))
{
}

} // namespace jinja2

#endif // JINJA2CPP_VALUE_H
