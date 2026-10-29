#ifndef JINJA2CPP_USER_CALLABLE_H
#define JINJA2CPP_USER_CALLABLE_H

#include "config.h"
#include "fwd.h" // IWYU pragma: export
#include "markup.h"
#include "string_helpers.h"
#include "value.h"

#include <cassert>
#include <cstddef>
#include <optional>
#include <stdexcept> // IWYU pragma: keep (public header: user code may rely on it)
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>

namespace jinja2
{
inline namespace v2
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
    ArgPromoter(const T* val) // NOLINT(google-explicit-constructor)
        : m_ptr(val)
    {
    }

    operator T() const { return *m_ptr; } // NOLINT(google-explicit-constructor)

    const T* m_ptr;
};

template<>
struct ArgPromoter<NoneValue, void>
{
public:
    ArgPromoter(const NoneValue*) {} // NOLINT(google-explicit-constructor)

    template<typename T>
    operator T() // NOLINT(google-explicit-constructor)
    {
        return T();
    }
};

template<typename T>
struct ArgPromoter<T, std::enable_if_t<std::is_fundamental_v<T>>>
{
    ArgPromoter(const T* val) // NOLINT(google-explicit-constructor)
        : m_ptr(val)
    {
    }

    template<typename U = T, typename = std::enable_if_t<std::is_convertible_v<T, U>>>
    operator U() const // NOLINT(google-explicit-constructor)
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

    ArgPromoter(const string* str) // NOLINT(google-explicit-constructor)
        : m_ptr(str)
    {
    }

    operator const string&() const { return *m_ptr; } // NOLINT(google-explicit-constructor)
    operator string() const { return *m_ptr; }        // NOLINT(google-explicit-constructor)
    operator string_view() const { return *m_ptr; }   // NOLINT(google-explicit-constructor)
    operator other_string() const                     // NOLINT(google-explicit-constructor)
    {
        return ConvertString<other_string>(*m_ptr);
    }
    operator other_string_view() const JINJA2CPP_LIFETIMEBOUND // NOLINT(google-explicit-constructor)
    {
        m_convertedStr = ConvertString<other_string>(*m_ptr);
        return m_convertedStr.value();
    }

    //! A Markup parameter gets the string escaped, as `markupsafe.escape` would make one (docs/tasks/0277)
    operator BasicMarkup<CharT>() const { return Escape(string_view(*m_ptr)); } // NOLINT(google-explicit-constructor)

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

    ArgPromoter(const string_view* str) // NOLINT(google-explicit-constructor)
        : m_ptr(str)
    {
    }

    // No `operator const string_view&`: next to `operator string_view` it made a by-value view parameter
    // ambiguous, and a `string_view&&` one cannot bind its lvalue
    operator string_view() const { return *m_ptr; }                          // NOLINT(google-explicit-constructor)
    operator string() const { return string(m_ptr->begin(), m_ptr->end()); } // NOLINT(google-explicit-constructor)
    operator other_string() const                                            // NOLINT(google-explicit-constructor)
    {
        return ConvertString<other_string>(*m_ptr);
    }
    operator other_string_view() const JINJA2CPP_LIFETIMEBOUND // NOLINT(google-explicit-constructor)
    {
        m_convertedStr = ConvertString<other_string>(*m_ptr);
        return m_convertedStr.value();
    }

    //! A Markup parameter gets the string escaped, as `markupsafe.escape` would make one (docs/tasks/0277)
    operator BasicMarkup<CharT>() const { return Escape(*m_ptr); } // NOLINT(google-explicit-constructor)

    const string_view* m_ptr;
    mutable std::optional<other_string> m_convertedStr;
};

// A Markup argument fills a Markup parameter as is and a string parameter with its text
template<typename CharT>
struct ArgPromoter<BasicMarkup<CharT>, void>
{
    using markup = BasicMarkup<CharT>;
    using string = std::basic_string<CharT>;
    using string_view = std::basic_string_view<CharT>;
    using other_string = std::conditional_t<std::is_same_v<CharT, char>, std::wstring, std::string>;
    using other_string_view = std::conditional_t<std::is_same_v<CharT, char>, std::wstring_view, std::string_view>;

    ArgPromoter(const markup* text) // NOLINT(google-explicit-constructor)
        : m_ptr(text)
    {
    }

    operator const markup&() const { return *m_ptr; }       // NOLINT(google-explicit-constructor)
    operator const string&() const { return m_ptr->str(); } // NOLINT(google-explicit-constructor)
    operator string_view() const { return *m_ptr; }         // NOLINT(google-explicit-constructor)
    operator other_string() const                           // NOLINT(google-explicit-constructor)
    {
        return ConvertString<other_string>(m_ptr->str());
    }
    operator other_string_view() const JINJA2CPP_LIFETIMEBOUND // NOLINT(google-explicit-constructor)
    {
        m_convertedStr = ConvertString<other_string>(m_ptr->str());
        return m_convertedStr.value();
    }

    const markup* m_ptr;
    mutable std::optional<other_string> m_convertedStr;
};

template<typename Arg>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): keeps the argument's address, never moves it
auto Promote(Arg&& arg JINJA2CPP_LIFETIMEBOUND)
{
    return ArgPromoter<std::decay_t<Arg>>(&arg);
}

// A returned view may point into an argument, which dies when the call returns, so the result
// keeps its own copy of the text (docs/tasks/0296)
template<typename R>
Value MakeCallResult(R&& result)
{
    using ResultT = std::decay_t<R>;
    if constexpr (std::is_same_v<ResultT, std::string_view> || std::is_same_v<ResultT, std::wstring_view>)
    {
        return Value(std::basic_string<typename ResultT::value_type>(result));
    }
    else
    {
        return Value(std::forward<R>(result));
    }
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
    // NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): Promote takes each argument's address
    auto operator()(Args&&... args) const -> std::enable_if_t<CanBeCalled<FuncTester<Args...>>::value, Value>
    {
        return MakeCallResult(fn(Promote(args)...));
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
    {
        return p->second;
    }
    if (info.paramName == ArgInfo::VarKwArgs)
    {
        return params.extraKwArgs;
    }
    if (info.paramName == ArgInfo::VarArgs)
    {
        return params.extraPosArgs;
    }
    if (info.paramName == ArgInfo::Context)
    {
        return params.context;
    }

    return info.defValue;
}

// keepMarkup passes a Markup on as itself, for ArgPromoter to hand to a Markup or a string parameter
template<typename V, bool keepMarkup = false>
struct ParamUnwrapper
{
    V* m_visitor{};

    explicit ParamUnwrapper(V* v)
        : m_visitor(v)
    {}


    template<typename T>
    static const auto& UnwrapRecursive(const T& arg)
    {
        return arg; // std::forward<T>(arg);
    }

    // A Markup reaches a typed parameter as the string it is; UserCallableParams keep the flag
    template<typename CharT>
    static const auto& UnwrapRecursive(const BasicMarkup<CharT>& arg)
    {
        if constexpr (keepMarkup)
        {
            return arg;
        }
        else
        {
            return arg.str();
        }
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
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): fn is held by reference, ad only read
Value InvokeUserCallable(Fn&& fn, const UserCallableParams& params, ArgDescr&&... ad)
{
    // One visit over every combination of argument types: 17 alternatives make 17^N instantiations, so three string
    // parameters already took 41 s and 2.4 MB of code to compile (docs/tasks/0334)
    static_assert(sizeof...(ArgDescr) <= 2,
                  "MakeCallable: the parameter types of this callable cannot be read (a generic lambda or an overload set), "
                  "or there are more descriptors than parameters, so its arguments are matched jointly, which takes minutes "
                  "to compile from three arguments; describe the parameters with ArgInfoT<T> or give the callable concrete "
                  "parameter types");
    auto invoker = UCInvoker<Fn>(fn, params);
    return std::visit(ParamUnwrapper<UCInvoker<Fn>, true>(&invoker), GetParamValue(params, ad).GetData()...);
}

// A view parameter given text of the other width needs the converted string to outlive the call
template<typename T>
struct ConvertedTextStorage
{
    struct NoStorage
    {
    };
    using type = NoStorage;
    static constexpr bool IsView = false;
};

template<typename CharT>
struct ConvertedTextStorage<std::basic_string_view<CharT>>
{
    using type = std::optional<std::basic_string<CharT>>;
    static constexpr bool IsView = true;
};

template<typename T>
struct TypedParam
{
    using decayed_t = std::decay_t<T>;
    using storage_t = typename ConvertedTextStorage<decayed_t>::type;
    static constexpr bool IsView = ConvertedTextStorage<decayed_t>::IsView;
    std::variant<NoneValue, decayed_t, const decayed_t*> data;
    // Owned by the param, which the caller keeps until the function returns; the view is made in
    // GetValue, so moving the param (and the string's small buffer) is safe (docs/tasks/0296)
    storage_t converted;

    [[nodiscard]] bool HasValue() const { return data.index() != 0; }
    // A view comes back by value, so a `const std::string_view&` parameter binds to a view of `converted`
    [[nodiscard]] std::conditional_t<IsView, decayed_t, T> GetValue() const
    {
        if constexpr (IsView)
        {
            if (converted)
            {
                return decayed_t(*converted);
            }
        }
        if (data.index() == 1)
        {
            return std::get<decayed_t>(data);
        }
        return *std::get<const decayed_t*>(data);
    }

    void SetPointer(const decayed_t* ptr) { data = ptr; }

    // By value: a view argument is captured by the param, which Clang checks against a temporary that owns
    // the text (-Wdangling-capture, docs/tasks/0296, 0343)
    void SetValue(decayed_t val JINJA2CPP_LIFETIME_CAPTURE_BY(this)) { data = std::move(val); }

    template<typename String>
    void SetConverted(String&& str)
    {
        converted = std::forward<String>(str);
        data = decayed_t();
    }
};

template<typename T, typename CharT>
struct IsOtherWidthText : std::false_type
{
};

template<typename OtherCharT, typename CharT>
struct IsOtherWidthText<std::basic_string<OtherCharT>, CharT> : std::bool_constant<!std::is_same_v<OtherCharT, CharT>>
{
};

template<typename OtherCharT, typename CharT>
struct IsOtherWidthText<std::basic_string_view<OtherCharT>, CharT> : std::bool_constant<!std::is_same_v<OtherCharT, CharT>>
{
};

template<typename T>
struct TypedParamUnwrapper
{
    TypedParam<T>* param;
    using ValueType = typename TypedParam<T>::decayed_t;
    explicit TypedParamUnwrapper(TypedParam<T>& p)
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

        // `value` is the type-trait convention, so it keeps its standard-library name
        // NOLINTNEXTLINE(readability-identifier-naming)
        static constexpr bool value = !std::is_same_v<decayed_u, NoneValue> && sizeof(PromotedType(std::declval<U>())) == sizeof(int);
    };

    void operator()(const ValueType& val) const { param->SetPointer(&val); }

    // The overloads are all const: a non-const template beat the const None overload, so an omitted argument (None)
    // left the parameter without a value and the call was skipped (docs/tasks/0320)
    void operator()(const NoneValue&) const { param->SetValue(ValueType()); }

    template<typename U>
    auto operator()(const U& v) const -> std::enable_if_t<PromoteTester<U>::value && !std::is_same_v<std::decay_t<U>, ValueType>>
    {
        if constexpr (TypedParam<T>::IsView)
        {
            using CharT = typename ValueType::value_type;
            if constexpr (IsOtherWidthText<std::decay_t<U>, CharT>::value)
            {
                // Promote(v) would view a string its temporary promoter owns
                param->SetConverted(ConvertString<std::basic_string<CharT>>(v));
            }
            else
            {
                param->SetValue(Promote(v));
            }
        }
        else
        {
            param->SetValue(Promote(v));
        }
    }

    template<typename U>
    auto operator()(const U&) const -> std::enable_if_t<!PromoteTester<U>::value && !std::is_same_v<std::decay_t<U>, ValueType>>
    {
    }
};

template<typename T>
struct IsMarkupParam : std::false_type
{
};

template<typename CharT>
struct IsMarkupParam<BasicMarkup<CharT>> : std::true_type
{
};

// A Markup parameter takes a Markup argument as is and a string of its character type escaped, as
// `markupsafe.escape` would make one (docs/tasks/0277); None gives an empty Markup, anything else no value
template<typename T>
struct MarkupParamUnwrapper
{
    using ValueType = typename TypedParam<T>::decayed_t;
    using CharT = typename ValueType::value_type;
    TypedParam<T>* param;
    explicit MarkupParamUnwrapper(TypedParam<T>& p)
        : param(&p)
    {
    }

    void operator()(const ValueType& val) const { param->SetPointer(&val); }
    void operator()(const std::basic_string<CharT>& val) const { param->SetValue(Escape(std::basic_string_view<CharT>(val))); }
    void operator()(std::basic_string_view<CharT> val) const { param->SetValue(Escape(val)); }
    void operator()(const NoneValue&) const { param->SetValue(ValueType()); }

    // Text of the other width is converted first: a Markup stays a Markup, a string is escaped
    using OtherCharT = std::conditional_t<std::is_same_v<CharT, char>, wchar_t, char>;
    void operator()(const BasicMarkup<OtherCharT>& val) const { param->SetValue(ValueType(ConvertString<std::basic_string<CharT>>(val.str()))); }
    void operator()(const std::basic_string<OtherCharT>& val) const { param->SetValue(Escape(ConvertString<std::basic_string<CharT>>(val))); }
    void operator()(std::basic_string_view<OtherCharT> val) const { param->SetValue(Escape(ConvertString<std::basic_string<CharT>>(val))); }

    template<typename U>
    void operator()(const U&) const
    {
    }
};

// A Value parameter takes the argument as passed, None and Markup included (docs/tasks/0319)
template<typename T>
struct ValueParam
{
    const Value* value;

    [[nodiscard]] static bool HasValue() { return true; }
    // A by-value or `Value&&` parameter gets its own copy
    [[nodiscard]] std::conditional_t<std::is_reference_v<T>, const Value&, Value> GetValue() const { return *value; }
};

template<typename T>
auto TypedUnwrapParam(const Value& value)
{
    if constexpr (std::is_same_v<std::decay_t<T>, Value>)
    {
        return ValueParam<T>{ &value };
    }
    else
    {
        TypedParam<T> param;
        if constexpr (IsMarkupParam<std::decay_t<T>>::value)
        {
            // Not through ParamUnwrapper, which hands a Markup over as its string and would drop the flag
            std::visit(MarkupParamUnwrapper<T>(param), value.GetData());
        }
        else
        {
            TypedParamUnwrapper<T> visitor(param);
            std::visit(ParamUnwrapper<TypedParamUnwrapper<T>>(&visitor), value.GetData());
        }
        return param;
    }
}

template<typename... P>
struct ParamList
{
    static constexpr std::size_t Size = sizeof...(P);
};

// Each argument is converted to its parameter type `T` on its own, so a callable with N parameters costs N visits
// of the argument variant to compile, not one visit of every combination (docs/tasks/0334)
template<typename... T, typename Fn, typename... ArgDescr>
Value InvokeWithParamTypes(ParamList<T...> /*types*/, const Fn& fn, const UserCallableParams& params, const ArgDescr&... ad)
{
    auto typedParams = std::make_tuple(TypedUnwrapParam<T>(GetParamValue(params, ad))...);
    return std::apply(
        [&fn](auto&... args) {
            bool hasValue = (true && ... && args.HasValue());
            if (!hasValue)
            {
                return Value();
            }

            return MakeCallResult(fn(args.GetValue()...));
        },
        typedParams);
}

template<typename Fn, typename... ArgDescr>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward): fn is called through a reference, ad only read
Value InvokeTypedUserCallable(Fn&& fn, const UserCallableParams& params, ArgDescr&&... ad)
{
    return InvokeWithParamTypes(ParamList<typename std::decay_t<ArgDescr>::type...>(), fn, params, ad...);
}

// The type an argument is converted to for a parameter of type `P`: a `T&&` parameter takes a converted `T`
template<typename P>
struct DeducedParam
{
    static_assert(!std::is_lvalue_reference_v<P> || std::is_const_v<std::remove_reference_t<P>>,
                  "MakeCallable: a parameter cannot be a non-const lvalue reference, take it by value or by const reference");
    using type = std::conditional_t<std::is_rvalue_reference_v<P>, std::remove_cv_t<std::remove_reference_t<P>>, P>;
};

// The parameter types of a callable whose signature can be read: a function, a function pointer, or a class with a
// single const, non-template `operator()` (a non-generic lambda, `std::function`)
template<typename Fn, typename = void>
struct CallableSignature
{
    static constexpr bool IsKnown = false;
};

template<typename R, typename... Args>
struct CallableSignature<R (*)(Args...), void>
{
    static constexpr bool IsKnown = true;
    using Params = ParamList<Args...>;
};

template<typename R, typename... Args>
struct CallableSignature<R (*)(Args...) noexcept, void> : CallableSignature<R (*)(Args...)>
{
};

template<typename R, typename C, typename... Args>
struct CallableSignature<R (C::*)(Args...) const, void> : CallableSignature<R (*)(Args...)>
{
};

template<typename R, typename C, typename... Args>
struct CallableSignature<R (C::*)(Args...) const noexcept, void> : CallableSignature<R (*)(Args...)>
{
};

template<typename Fn>
struct CallableSignature<Fn, std::enable_if_t<std::is_class_v<Fn>, std::void_t<decltype(&Fn::operator())>>>
    : CallableSignature<decltype(&Fn::operator())>
{
};

// A class whose only `operator()` is non-const (a mutable lambda) cannot be called through the const copy MakeCallable keeps
template<typename Fn, typename = void>
struct HasNonConstCallOperator : std::false_type
{
};

template<typename R, typename C, typename... Args>
struct HasNonConstCallOperator<R (C::*)(Args...), void> : std::true_type
{
};

template<typename R, typename C, typename... Args>
struct HasNonConstCallOperator<R (C::*)(Args...) noexcept, void> : std::true_type
{
};

template<typename Fn>
struct HasNonConstCallOperator<Fn, std::enable_if_t<std::is_class_v<Fn>, std::void_t<decltype(&Fn::operator())>>>
    : HasNonConstCallOperator<decltype(&Fn::operator())>
{
};

// Arguments are dispatched one by one when the descriptors name every parameter of a readable signature
template<typename Fn, std::size_t argsCount, typename Signature = CallableSignature<Fn>, typename = void>
struct HasDeducibleParams : std::false_type
{
};

template<typename Fn, std::size_t argsCount, typename Signature>
struct HasDeducibleParams<Fn, argsCount, Signature, std::enable_if_t<Signature::IsKnown>>
    : std::bool_constant<Signature::Params::Size == argsCount>
{
};

template<typename Fn, typename... P, typename... ArgDescr>
Value InvokeDeducedUserCallable(const Fn& fn, ParamList<P...> /*params*/, const UserCallableParams& params, const ArgDescr&... ad)
{
    return InvokeWithParamTypes(ParamList<typename DeducedParam<P>::type...>(), fn, params, ad...);
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
 * Each argument is converted to the type of its parameter: the stored alternative as is or converted (a number to
 * another arithmetic type, a string to the other character width, a string to an escaped \ref Markup), None to a
 * default-constructed value, an omitted argument without a default the same as None. When an argument cannot be
 * converted the function is not called and the call returns None. A \ref Value parameter (by value or `const&`) takes
 * the argument as passed. The parameter types are read from the signature of a function, a function pointer or a
 * class with one const, non-template `operator()`; for a generic lambda or an overload set describe them with
 * \ref ArgInfoT (a generic callable with untyped descriptors is limited to two parameters).
 *
 * \param f  Function which should be called
 * \param ad Function param descriptors
 *
 * \returns Instance of the properly initialized \ref UserCallable structure
 */
template<typename Fn, typename... ArgDescr>
auto MakeCallable(Fn&& f, ArgDescr&&... ad) -> std::enable_if_t<!detail::ArgDescrHasType<ArgDescr...>::value, UserCallable>
{
    using Signature = detail::CallableSignature<std::decay_t<Fn>>;
    static_assert(!detail::HasNonConstCallOperator<std::decay_t<Fn>>::value,
                  "MakeCallable: the callable is kept as a const copy, so its operator() must be const (not a mutable lambda)");
    UserCallable::Function callable = [=, fn = std::forward<Fn>(f)](const UserCallableParams& params) {
        if constexpr (detail::HasDeducibleParams<std::decay_t<Fn>, sizeof...(ArgDescr)>::value)
        {
            return detail::InvokeDeducedUserCallable(fn, typename Signature::Params(), params, ad...);
        }
        else
        {
            return detail::InvokeUserCallable(fn, params, ad...);
        }
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
        return detail::InvokeDeducedUserCallable(fn, detail::ParamList<Args...>(), params, ad...);
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
        return detail::InvokeDeducedUserCallable(
            [fn, obj](Args&&... args) { return (obj->*fn)(std::forward<Args>(args)...); }, detail::ParamList<Args...>(), params, ad...);
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
        return detail::InvokeDeducedUserCallable(
            [fn, obj](Args&&... args) { return (obj->*fn)(std::forward<Args>(args)...); }, detail::ParamList<Args...>(), params, ad...);
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
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_USER_CALLABLE_H
