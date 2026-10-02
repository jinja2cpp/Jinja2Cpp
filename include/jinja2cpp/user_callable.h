#ifndef JINJA2CPP_USER_CALLABLE_H
#define JINJA2CPP_USER_CALLABLE_H

#include "string_helpers.h"
#include "value.h"

#include <optional>

#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace jinja2
{
#ifndef JINJA2CPP_NO_DOXYGEN
namespace detail
{
template<typename T, typename Enabled = void>
struct CanBeCalled : std::false_type
{
};

template<typename T>
struct CanBeCalled<T, std::enable_if_t<std::is_same_v<typename T::result_type, Value>>> : std::true_type
{
};

template<typename T, typename Tag = void>
struct ArgPromoter
{
    ArgPromoter(const T* val)
        : m_ptr(val)
    {
    }

    operator T() const { return *m_ptr; }

    const T* m_ptr;
};

template<>
struct ArgPromoter<EmptyValue, void>
{
public:
    ArgPromoter(const EmptyValue*) {}

    template<typename T>
    operator T()
    {
        return T();
    }
};

template<typename T>
struct ArgPromoter<T, std::enable_if_t<std::is_fundamental_v<T>>>
{
    ArgPromoter(const T* val)
        : m_ptr(val)
    {
    }

    template<typename U = T, typename = std::enable_if_t<std::is_convertible_v<T, U>>>
    operator U() const
    {
        return static_cast<U>(*m_ptr);
    }

    const T* m_ptr;
};

template<typename CharT>
struct ArgPromoter<std::basic_string<CharT>, void>
{
    using string = std::basic_string<CharT>;
    using string_view = std::basic_string_view<CharT>;
    using other_string = std::conditional_t<std::is_same_v<CharT, char>, std::wstring, std::string>;
    using other_string_view = std::conditional_t<std::is_same_v<CharT, char>, std::wstring_view, std::string_view>;

    ArgPromoter(const string* str)
        : m_ptr(str)
    {
    }

    operator const string&() const { return *m_ptr; }
    operator string() const { return *m_ptr; }
    operator string_view() const { return *m_ptr; }
    operator other_string() const
    {
        return ConvertString<other_string>(*m_ptr);
    }
    operator other_string_view() const
    {
        m_convertedStr = ConvertString<other_string>(*m_ptr);
        return m_convertedStr.value();
    }

    const string* m_ptr;
    mutable std::optional<other_string> m_convertedStr;
};

template<typename CharT>
struct ArgPromoter<std::basic_string_view<CharT>, void>
{
    using string = std::basic_string<CharT>;
    using string_view = std::basic_string_view<CharT>;
    using other_string = std::conditional_t<std::is_same_v<CharT, char>, std::wstring, std::string>;
    using other_string_view = std::conditional_t<std::is_same_v<CharT, char>, std::wstring_view, std::string_view>;

    ArgPromoter(const string_view* str)
        : m_ptr(str)
    {
    }

    operator const string_view&() const { return *m_ptr; }
    operator string_view() const { return *m_ptr; }
    operator string() const { return string(m_ptr->begin(), m_ptr->end()); }
    operator other_string() const
    {
        return ConvertString<other_string>(*m_ptr);
    }
    operator other_string_view() const
    {
        m_convertedStr = ConvertString<other_string>(*m_ptr);
        return m_convertedStr.value();
    }

    const string_view* m_ptr;
    mutable std::optional<other_string> m_convertedStr;
};

template<typename Arg>
auto Promote(Arg&& arg)
{
    return ArgPromoter<std::decay_t<Arg>>(&arg);
}

template<typename Fn>
struct UCInvoker
{
    const Fn& fn;
    const UserCallableParams& params;

    template<typename... Args>
    struct FuncTester
    {
        template<typename F>
        static auto TestFn(F&& f) -> decltype(Value(f(Promote(std::declval<Args>())...)));
        static auto TestFn(...) -> char;

        using result_type = decltype(TestFn(std::declval<Fn>()));
    };

    UCInvoker(const Fn& f, const UserCallableParams& p)
        : fn(f)
        , params(p)
    {}

    template<typename... Args>
    auto operator()(Args&&... args) const -> std::enable_if_t<CanBeCalled<FuncTester<Args...>>::value, Value>
    {
        return Value(fn(Promote(args)...));
    }

    template<typename... Args>
    auto operator()(Args&&...) const -> std::enable_if_t<!CanBeCalled<FuncTester<Args...>>::value, Value>
    {
        return Value();
    }
};

inline const Value& GetParamValue(const UserCallableParams& params, const ArgInfo& info)
{
    // static Value empty;
    auto p = params.args.find(info.paramName);
    if (p != params.args.end())
        return p->second;
    if (info.paramName == ArgInfo::VarKwArgs)
        return params.extraKwArgs;
    if (info.paramName == ArgInfo::VarArgs)
        return params.extraPosArgs;
    if (info.paramName == ArgInfo::Context)
        return params.context;

    return info.defValue;
}

template<typename V>
struct ParamUnwrapper
{
    V* m_visitor{};

    ParamUnwrapper(V* v)
        : m_visitor(v)
    {}


    template<typename T>
    static const auto& UnwrapRecursive(const T& arg)
    {
        return arg; // std::forward<T>(arg);
    }

    template<typename T>
    static auto& UnwrapRecursive(const RecWrapper<T>& arg)
    {
        return *arg;
    }

    template<typename... Args>
    auto operator()(const Args&... args) const
    {
        assert(m_visitor != nullptr);
        return (*m_visitor)(UnwrapRecursive(args)...);
    }
};

template<typename Fn, typename... ArgDescr>
Value InvokeUserCallable(Fn&& fn, const UserCallableParams& params, ArgDescr&&... ad)
{
    auto invoker = UCInvoker<Fn>(fn, params);
    return std::visit(ParamUnwrapper<UCInvoker<Fn>>(&invoker), GetParamValue(params, ad).data()...);
}

template<typename T>
struct TypedParam
{
    using decayed_t = std::decay_t<T>;
    std::variant<EmptyValue, decayed_t, const decayed_t*> data;

    [[nodiscard]] bool HasValue() const { return data.index() != 0; }
    [[nodiscard]] T GetValue() const
    {
        if (data.index() == 1)
            return std::get<decayed_t>(data);
        return *std::get<const decayed_t*>(data);
    }

    void SetPointer(const decayed_t* ptr) { data = ptr; }

    void SetValue(decayed_t&& val) { data = std::move(val); }
};

template<typename T>
struct TypedParamUnwrapper
{
    TypedParam<T>* param;
    using ValueType = typename TypedParam<T>::decayed_t;
    TypedParamUnwrapper(TypedParam<T>& p)
        : param(&p)
    {
    }

    template<typename U>
    struct PromoteTester
    {
        using decayed_u = std::decay_t<U>;
        struct InvalidType
        {
        };

        static auto TestFn(T) -> int;
        static auto TestFn(...) -> char;

        template<typename U1>
        static auto PromotedType(U1 u) -> decltype(TestFn(Promote(u)));
        static auto PromotedType(...) -> char;

        enum
        {
            value = std::is_same_v < decayed_u,
            EmptyValue > ? false : sizeof(PromotedType(std::declval<U>())) == sizeof(int)
        };
    };

    void operator()(const ValueType& val) const { param->SetPointer(&val); }

    void operator()(const EmptyValue&) const { param->SetValue(ValueType()); }

    template<typename U>
    auto operator()(const U& v) -> std::enable_if_t<PromoteTester<U>::value && !std::is_same_v<std::decay_t<U>, ValueType>>
    {
        param->SetValue(Promote(v));
    }

    template<typename U>
    auto operator()(const U&) -> std::enable_if_t<!PromoteTester<U>::value && !std::is_same_v<std::decay_t<U>, ValueType>>
    {
    }
};

template<typename T, typename V>
auto TypedUnwrapParam(const V& value)
{
    TypedParam<T> param;
    TypedParamUnwrapper<T> visitor(param);
    std::visit(ParamUnwrapper<TypedParamUnwrapper<T>>(&visitor), value);
    return param;
}

template<typename Fn, typename... ArgDescr>
Value InvokeTypedUserCallable(Fn&& fn, const UserCallableParams& params, ArgDescr&&... ad)
{
    auto typed_params = std::make_tuple(TypedUnwrapParam<typename std::decay_t<ArgDescr>::type>(GetParamValue(params, ad).data())...);
    return std::apply(
        [&fn](auto&... args) {
            bool has_value = (true && ... && args.HasValue());
            if (!has_value)
                return Value();

            return Value(fn(args.GetValue()...));
        },
        typed_params);
}

template<typename... ArgDescr>
struct ArgDescrHasType : std::false_type
{
};

template<typename... T>
struct ArgDescrHasType<ArgInfoT<T>...> : std::true_type
{
};
} // namespace detail
#endif // JINJA2CPP_NO_DOXYGEN

/*!
 * \brief Create user-defined callable from the specified function
 *
 * Creates instance of the UserCallable object which invokes specified function (f) with parsed params according
 * to the description (ad). For instance:
 * ```c++
 * MakeCallable(
 *              [](const std::string& str1, const std::string& str2) {
 *                  return str1 + " " + str2;
 *              },
 *              ArgInfo{"str1"}, ArgInfo{"str2", false, "default"}
 * );
 * ```
 * In this sample lambda function with two string params will be invoked from the jinja2 template and provided with
 * the specified params. Each param is described by \ref ArgInfo structure. Result of the function will be converted
 * and passed back to the jinja2 template.
 *
 * In case the function should accept extra positional args or extra named args this params should be described the
 * following name.
 *  - Extra positional args. \ref ArgInfo should describe this param with name `*args` (\ref ArgInfo::VarArgs). Param of the function should
 *    has \ref ValuesList type
 *  - Extra named args. \ref ArgInfo should describe this param with name `**kwargs` (\ref ArgInfo::VarKwArgs). Param of the function should
 *    has \ref ValuesMap type
 *  - Current template context. \ref ArgInfo should describe this param with name `*context` (\ref ArgInfo::Context). Param of the function should
 *    has \ref GenericMap type
 *
 * \param f  Function which should be called
 * \param ad Function param descriptors
 *
 * \returns Instance of the properly initialized \ref UserCallable structure
 */
template<typename Fn, typename... ArgDescr>
auto MakeCallable(Fn&& f, ArgDescr&&... ad) -> std::enable_if_t<!detail::ArgDescrHasType<ArgDescr...>::value, UserCallable>
{
    UserCallable::Function callable = [=, fn = std::forward<Fn>(f)](const UserCallableParams& params) {
        return detail::InvokeUserCallable(fn, params, ad...);
    };
    return UserCallable{
        callable,
        { ArgInfo(std::forward<ArgDescr>(ad))... }
    };
}

template<typename Fn, typename... ArgDescr>
auto MakeCallable(Fn&& f, ArgDescr&&... ad) -> std::enable_if_t<detail::ArgDescrHasType<ArgDescr...>::value, UserCallable>
{
    UserCallable::Function callable = [=, fn = std::forward<Fn>(f)](const UserCallableParams& params) {
        return detail::InvokeTypedUserCallable(fn, params, ad...);
    };
    return UserCallable{
        callable,
        { ArgInfo(std::forward<ArgDescr>(ad))... }
    };
}

template<typename R, typename... Args, typename... ArgDescr>
auto MakeCallable(R (*f)(Args...), ArgDescr&&... ad) -> UserCallable
{
    UserCallable::Function callable = [=, fn = f](const UserCallableParams& params) {
        return detail::InvokeTypedUserCallable(fn, params, ArgInfoT<Args>(ad)...);
    };
    return UserCallable{
        callable,
        { ArgInfoT<Args>(std::forward<ArgDescr>(ad))... }
    };
}

template<typename R, typename T, typename... Args, typename... ArgDescr>
auto MakeCallable(R (T::*f)(Args...), T* obj, ArgDescr&&... ad) -> UserCallable
{
    UserCallable::Function callable = [=, fn = f](const UserCallableParams& params) {
        return detail::InvokeTypedUserCallable(
            [fn, obj](Args&&... args) { return (obj->*fn)(std::forward<Args>(args)...); }, params, ArgInfoT<Args>(ad)...);
    };
    return UserCallable{
        callable,
        { ArgInfoT<Args>(std::forward<ArgDescr>(ad))... }
    };
}

template<typename R, typename T, typename... Args, typename... ArgDescr>
auto MakeCallable(R (T::*f)(Args...) const, const T* obj, ArgDescr&&... ad) -> UserCallable
{
    UserCallable::Function callable = [=, fn = f](const UserCallableParams& params) {
        return detail::InvokeTypedUserCallable(
            [fn, obj](Args&&... args) { return (obj->*fn)(std::forward<Args>(args)...); }, params, ArgInfoT<Args>(ad)...);
    };
    return UserCallable{
        callable,
        { ArgInfoT<Args>(std::forward<ArgDescr>(ad))... }
    };
}

/*!
 * \brief Create user-callable from the function with no params.
 *
 * \param f Function which should be called
 *
 * \returns Instance of the properly initialized \ref UserCallable structure
 */
template<typename Fn>
auto MakeCallable(Fn&& f)
{
    return UserCallable{ [=, fn = std::forward<Fn>(f)](const UserCallableParams&) { return fn(); }, {} };
}
} // namespace jinja2

#endif // JINJA2CPP_USER_CALLABLE_H
