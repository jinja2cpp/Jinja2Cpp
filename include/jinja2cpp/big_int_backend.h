#ifndef JINJA2CPP_BIG_INT_BACKEND_H
#define JINJA2CPP_BIG_INT_BACKEND_H

#include "config.h"

#include <cstddef>
#include <cstdint>

namespace jinja2
{
inline namespace v2
{
//! What a \ref BigIntBackend operation reports
enum class BigIntStatus : int32_t
{
    Ok = 0,         //!< The result is in `out`
    DivisionByZero, //!< The divisor is zero
    OutOfMemory,    //!< The backend could not allocate; the render fails with `std::bad_alloc`
    TooLarge,       //!< The result is wider than the backend supports; the render fails with ResourceLimitExceeded
    Unsupported,    //!< The backend does not implement the operation
    BadInput,       //!< The input is not a canonical decimal integer
};

/*!
 * \brief Big-integer arithmetic for templates: Python's `int` beyond `int64_t`
 *
 * Without a backend, a template prints, compares and converts integers of any size, and integer arithmetic whose
 * result leaves `int64_t` fails with ErrorCode::IntegerOutOfRange. With one (\ref TemplateEnv::SetBigIntBackend),
 * that arithmetic is exact. The library calls a backend only after an `int64_t` operation overflows, or when an
 * operand is already outside `int64_t`, and turns every result that fits `int64_t` back into one.
 *
 * The table is plain data so that a backend built separately from the library, against another compiler or
 * standard library, plugs into a prebuilt one. It grows only at its end: the library reads `structSize` and
 * `version`. Every function is `noexcept` and reports failure by \ref BigIntStatus.
 *
 * Objects: `out` is uninitialised storage of `objectSize` bytes aligned to `objectAlign`. On `Ok` it holds an
 * object the library later passes to `destroy` exactly once; on any other status it holds nothing. Objects are
 * not changed once made, and calls on distinct or shared objects may come from several threads at once. The
 * library composes floor division, modulo, `**`, rounding and conversion to other bases from these operations.
 *
 * A backend must outlive every template that uses it: one in static storage is the supported case.
 */
struct BigIntBackend
{
    //! `sizeof(BigIntBackend)` as the backend was compiled
    uint32_t structSize;
    //! 1
    uint32_t version;
    //! A name for messages, such as "boost-cpp_int"
    const char* name;
    //! The size of an object, at most 64
    uint32_t objectSize;
    //! The alignment of an object: a power of two, at most 16
    uint32_t objectAlign;

    void (*destroy)(const BigIntBackend* self, void* obj) noexcept;
    BigIntStatus (*fromInt64)(const BigIntBackend* self, void* out, int64_t value) noexcept;
    //! `digits` is canonical: an optional `-` and decimal digits without leading zeros, not `-0`; not null-terminated
    BigIntStatus (*fromDecimal)(const BigIntBackend* self, void* out, const char* digits, size_t size) noexcept;
    //! Whether the value fits `int64_t`, and then the value
    bool (*toInt64)(const BigIntBackend* self, const void* obj, int64_t* out) noexcept;
    //! Writes the canonical decimal form through `sink`, in one call or several
    BigIntStatus (*toDecimal)(const BigIntBackend* self, const void* obj, void (*sink)(void* ctx, const char* data, size_t size), void* ctx) noexcept;
    //! -1, 0 or 1
    int (*sign)(const BigIntBackend* self, const void* obj) noexcept;
    //! -1, 0 or 1 as `a` is less than, equal to or greater than `b`
    int (*compare)(const BigIntBackend* self, const void* a, const void* b) noexcept;
    //! The bits of |x|: Python's `int.bit_length()`
    size_t (*bitLength)(const BigIntBackend* self, const void* obj) noexcept;
    BigIntStatus (*negate)(const BigIntBackend* self, void* out, const void* x) noexcept;
    BigIntStatus (*add)(const BigIntBackend* self, void* out, const void* a, const void* b) noexcept;
    BigIntStatus (*subtract)(const BigIntBackend* self, void* out, const void* a, const void* b) noexcept;
    BigIntStatus (*multiply)(const BigIntBackend* self, void* out, const void* a, const void* b) noexcept;
    //! Truncating division, as C++ divides: the quotient rounds toward zero and the remainder takes the sign of `a`.
    //! Either output may be null, and then is not made
    BigIntStatus (*divideTruncated)(const BigIntBackend* self, void* quotient, void* remainder, const void* a, const void* b) noexcept;

    // Optional entries, still version 1 (docs/tasks/0543 P4). The library uses one only when `structSize` covers it
    // and it is not null, so a table compiled against an older header, or one that leaves them null, still works.
    // They spare the decimal conversion of an integer past 14285 bits, which the library keeps as binary words:
    // without them each such operand and result goes through `fromDecimal`/`toDecimal`, about 0.1 s at 2^20 bits

    //! The integer whose magnitude is `count` 32-bit `words`, least significant first, with a non-zero top word
    //! (`count` >= 1), negated when `sign` is -1 (`sign` is -1 or 1)
    BigIntStatus (*fromWords)(const BigIntBackend* self, void* out, const uint32_t* words, size_t count, int sign) noexcept;
    //! Writes |x| into `count` 32-bit words, least significant first; `count` is `(bitLength + 31) / 32` and at least 1
    BigIntStatus (*toWords)(const BigIntBackend* self, const void* obj, uint32_t* out, size_t count) noexcept;
};

//! The largest \ref BigIntBackend::objectSize the library accepts
inline constexpr uint32_t MaxBigIntObjectSize = 64;
//! The largest \ref BigIntBackend::objectAlign the library accepts
inline constexpr uint32_t MaxBigIntObjectAlign = 16;

/*!
 * \brief The backend every \ref TemplateEnv starts with
 *
 * The one the library was built with (`-DJINJA2CPP_WITH_BIGINT`), or null when it was built without one.
 */
JINJA2CPP_EXPORT const BigIntBackend* GetBuiltinBigIntBackend() noexcept;
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_BIG_INT_BACKEND_H
