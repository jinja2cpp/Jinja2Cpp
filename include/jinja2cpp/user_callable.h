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

template<typename Fn>
struct UCInvoker
{
    const Fn& fn;
    const UserCallableParams& params;

    template<typename ... Args>
    struct FuncTester
    {
        template<typename F>
        static auto TestFn(F&& f) -> decltype(Value(f(std::declval<Args>()...)));
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
        return Value(fn(args...));
    }

    template<typename ... Args>
    auto operator()(Args&& ... args) const -> std::enable_if_t<!CanBeCalled<FuncTester<Args...>>::value, Value>
    {
        return Value();
    }

};

const Value& GetParamValue(const UserCallableParams& params, const ArgInfo& info)
{
    // static Value empty;
    auto p = params.args.find(info.paramName);
    if (p == params.args.end())
        return info.defValue;

    return p->second;
}

template<typename Fn, typename ... ArgDescr>
Value InvokeUserCallable(Fn&& fn, const UserCallableParams& params, ArgDescr&& ... ad)
{
    return nonstd::visit(UCInvoker<Fn>(fn, params), GetParamValue(params, ad).data()...);
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
} // jinja2

#endif // USER_CALLABLE_H
