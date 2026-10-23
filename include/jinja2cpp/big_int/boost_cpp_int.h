#ifndef JINJA2CPP_BIG_INT_BOOST_CPP_INT_H
#define JINJA2CPP_BIG_INT_BOOST_CPP_INT_H

// A big-integer backend over Boost.Multiprecision's cpp_int (docs/tasks/0272)
//
// Header-only, and the sample for writing a backend over another library: register it with
//
//     #include <jinja2cpp/big_int/boost_cpp_int.h>
//     env.SetBigIntBackend(jinja2::BoostCppIntBackend());
//
// It needs only the Boost.Multiprecision and Boost.Config headers; define BOOST_MP_STANDALONE
// to use Multiprecision without the rest of Boost. A library built with
// -DJINJA2CPP_WITH_BIGINT=boost uses this backend by default (GetBuiltinBigIntBackend).

#include <jinja2cpp/big_int_backend.h>

#include <boost/multiprecision/cpp_int.hpp>

#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <utility>

namespace jinja2
{
inline namespace v2
{
namespace detail::boost_cpp_int
{
using Int = boost::multiprecision::cpp_int;

static_assert(sizeof(Int) <= MaxBigIntObjectSize, "cpp_int does not fit a backend object");
static_assert(alignof(Int) <= MaxBigIntObjectAlign, "cpp_int is aligned beyond a backend object");

inline const Int& Get(const void* obj) noexcept
{
    return *static_cast<const Int*>(obj);
}

// Constructs the result of `make` in `out`. Nothing may leave a backend function: cpp_int
// reports std::bad_alloc, and any other exception becomes `otherwise`
template<typename Fn>
BigIntStatus Make(void* out, const Fn& make, BigIntStatus otherwise = BigIntStatus::Unsupported) noexcept
{
    try
    {
        new (out) Int(make());
        return BigIntStatus::Ok;
    }
    catch (const std::bad_alloc&)
    {
        return BigIntStatus::OutOfMemory;
    }
    catch (...)
    {
        return otherwise;
    }
}

inline void Destroy(const BigIntBackend* /*self*/, void* obj) noexcept
{
    static_cast<Int*>(obj)->~Int();
}

inline BigIntStatus FromInt64(const BigIntBackend* /*self*/, void* out, int64_t value) noexcept
{
    return Make(out, [value] { return Int(value); });
}

inline BigIntStatus FromDecimal(const BigIntBackend* /*self*/, void* out, const char* digits, size_t size) noexcept
{
    // cpp_int parses a null-terminated string
    return Make(out, [digits, size] { return Int(std::string(digits, size)); }, BigIntStatus::BadInput);
}

inline bool ToInt64(const BigIntBackend* /*self*/, const void* obj, int64_t* out) noexcept
{
    const auto& x = Get(obj);
    if (x < INT64_MIN || x > INT64_MAX)
    {
        return false;
    }
    *out = x.convert_to<int64_t>();
    return true;
}

inline BigIntStatus ToDecimal(const BigIntBackend* /*self*/, const void* obj, void (*sink)(void* ctx, const char* data, size_t size), void* ctx) noexcept
{
    try
    {
        const auto text = Get(obj).str();
        sink(ctx, text.data(), text.size());
        return BigIntStatus::Ok;
    }
    catch (const std::bad_alloc&)
    {
        return BigIntStatus::OutOfMemory;
    }
    catch (...)
    {
        return BigIntStatus::Unsupported;
    }
}

inline int Sign(const BigIntBackend* /*self*/, const void* obj) noexcept
{
    return Get(obj).sign();
}

inline int Compare(const BigIntBackend* /*self*/, const void* a, const void* b) noexcept
{
    const int order = Get(a).compare(Get(b));
    if (order < 0)
    {
        return -1;
    }
    return order > 0 ? 1 : 0;
}

inline size_t BitLength(const BigIntBackend* /*self*/, const void* obj) noexcept
{
    const auto& x = Get(obj);
    if (x.is_zero())
    {
        return 0;
    }
    // cpp_int stores the magnitude in limbs, least significant first; msb() refuses negative values
    const auto& digits = x.backend();
    auto top = digits.limbs()[digits.size() - 1];
    size_t bits = (digits.size() - 1) * sizeof(top) * 8;
    for (; top != 0; top >>= 1)
    {
        ++bits;
    }
    return bits;
}

inline BigIntStatus Negate(const BigIntBackend* /*self*/, void* out, const void* x) noexcept
{
    return Make(out, [x] { return Int(-Get(x)); });
}

inline BigIntStatus Add(const BigIntBackend* /*self*/, void* out, const void* a, const void* b) noexcept
{
    return Make(out, [a, b] { return Int(Get(a) + Get(b)); });
}

inline BigIntStatus Subtract(const BigIntBackend* /*self*/, void* out, const void* a, const void* b) noexcept
{
    return Make(out, [a, b] { return Int(Get(a) - Get(b)); });
}

inline BigIntStatus Multiply(const BigIntBackend* /*self*/, void* out, const void* a, const void* b) noexcept
{
    return Make(out, [a, b] { return Int(Get(a) * Get(b)); });
}

inline BigIntStatus DivideTruncated(const BigIntBackend* /*self*/, void* quotient, void* remainder, const void* a, const void* b) noexcept
{
    if (Get(b).is_zero())
    {
        return BigIntStatus::DivisionByZero;
    }
    try
    {
        // cpp_int divides as C++ does: the quotient rounds toward zero
        Int q;
        Int r;
        boost::multiprecision::divide_qr(Get(a), Get(b), q, r);
        if (quotient)
        {
            new (quotient) Int(std::move(q));
        }
        if (remainder)
        {
            new (remainder) Int(std::move(r));
        }
        return BigIntStatus::Ok;
    }
    catch (const std::bad_alloc&)
    {
        return BigIntStatus::OutOfMemory;
    }
    catch (...)
    {
        return BigIntStatus::Unsupported;
    }
}
} // namespace detail::boost_cpp_int

//! The backend over `boost::multiprecision::cpp_int`, named "boost-cpp_int"
inline const BigIntBackend* BoostCppIntBackend() noexcept
{
    namespace impl = detail::boost_cpp_int;
    static const BigIntBackend backend{
        sizeof(BigIntBackend),
        1,
        "boost-cpp_int",
        static_cast<uint32_t>(sizeof(impl::Int)),
        static_cast<uint32_t>(alignof(impl::Int)),
        &impl::Destroy,
        &impl::FromInt64,
        &impl::FromDecimal,
        &impl::ToInt64,
        &impl::ToDecimal,
        &impl::Sign,
        &impl::Compare,
        &impl::BitLength,
        &impl::Negate,
        &impl::Add,
        &impl::Subtract,
        &impl::Multiply,
        &impl::DivideTruncated,
    };
    return &backend;
}
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_BIG_INT_BOOST_CPP_INT_H
