#ifndef USER_CALLABLE_H
#define USER_CALLABLE_H

#include "value.h"

#include <type_traits>

namespace jinja2
{
namespace detail
{
template<typename T, typename Enabled = void>
struct CanBeCalled : std::false_type {};

template<typename T>
struct CanBeCalled<T, typename std::enable_if<std::is_same<typename T::result_type, Value>::value>::type> : std::true_type {};

template<typename T>
struct ArgPromoter
{
    ArgPromoter(const T* val) : m_ptr(val) {}

    operator T() const { return *m_ptr; }

    const T* m_ptr;
};

template<>
struct ArgPromoter<EmptyValue>
{
public:
    ArgPromoter(const EmptyValue*) {}

    template<typename T>
    operator T() { return T(); }
};

template<typename CharT>
struct ArgPromoter<std::basic_string<CharT>>
{
    using string = std::basic_string<CharT>;
    using string_view = nonstd::basic_string_view<CharT>;

    ArgPromoter(const string* str) : m_ptr(str) {}

    operator const string&() const { return *m_ptr; }
    operator string () const { return *m_ptr; }
    operator string_view () const { return *m_ptr; }

    const string* m_ptr;
};

template<typename CharT>
struct ArgPromoter<nonstd::basic_string_view<CharT>>
{
    using string = std::basic_string<CharT>;
    using string_view = nonstd::basic_string_view<CharT>;

    ArgPromoter(const string_view* str) : m_ptr(str) {}

    operator const string_view& () const { return *m_ptr; }
    operator string_view () const { return *m_ptr; }
    operator string () const { return string(m_ptr->begin(), m_ptr->end()); }

    const string_view* m_ptr;
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

    template<typename ... Args>
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

    template<typename ... Args>
    auto operator()(Args&& ... args) const -> std::enable_if_t<CanBeCalled<FuncTester<Args...>>::value, Value>
    {
        return Value(fn(Promote(args)...));
    }

    template<typename ... Args>
    auto operator()(Args&& ... args) const -> std::enable_if_t<!CanBeCalled<FuncTester<Args...>>::value, Value>
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
    else if (info.paramName == "**kwargs")
        return params.extraKwArgs;
    else if (info.paramName == "*args")
        return params.extraPosArgs;

    return info.defValue;
}

template<typename V>
struct ParamUnwrapper
{
    V* m_visitor;

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
        return arg.value();
    }

    template<typename ... Args>
    auto operator()(const Args& ... args) const
    {
        assert(m_visitor != nullptr);
        return (*m_visitor)(UnwrapRecursive(args)...);
    }
};

template<typename Fn, typename ... ArgDescr>
Value InvokeUserCallable(Fn&& fn, const UserCallableParams& params, ArgDescr&& ... ad)
{
    auto invoker = UCInvoker<Fn>(fn, params);
    return nonstd::visit(ParamUnwrapper<UCInvoker<Fn>>(&invoker), GetParamValue(params, ad).data()...);
}
} // detail

template<typename Fn, typename ... ArgDescr>
auto MakeCallable(Fn&& f, ArgDescr&& ... ad)
{
    return UserCallable {
        [=, fn = std::forward<Fn>(f)](const UserCallableParams& params) {
            return detail::InvokeUserCallable(fn, params, ad...);
        },
        {ArgInfo(std::forward<ArgDescr>(ad))...}
    };
}

template<typename Fn>
auto MakeCallable(Fn&& f)
{
	return UserCallable{
		[=, fn = std::forward<Fn>(f)](const UserCallableParams& params) {
			return fn();
		},
		{}
	};
}
} // jinja2

#endif // USER_CALLABLE_H
