#ifndef JINJA2CPP_VALUE_PTR_HPP
#define JINJA2CPP_VALUE_PTR_HPP

//
// Copyright 2017-2018 by Martin Moene
//
// https://github.com/martinmoene/value-ptr-lite
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#ifndef NONSTD_VALUE_PTR_LITE_HPP
#define NONSTD_VALUE_PTR_LITE_HPP

#define value_ptr_lite_MAJOR  0
#define value_ptr_lite_MINOR  2
#define value_ptr_lite_PATCH  1

#define value_ptr_lite_VERSION  nsvp_STRINGIFY(value_ptr_lite_MAJOR) "." nsvp_STRINGIFY(value_ptr_lite_MINOR) "." nsvp_STRINGIFY(value_ptr_lite_PATCH)

#define nsvp_STRINGIFY(  x )  nsvp_STRINGIFY_( x )
#define nsvp_STRINGIFY_( x )  #x

// value-ptr-lite configuration:

#ifndef  nsvp_CONFIG_COMPARE_POINTERS
# define nsvp_CONFIG_COMPARE_POINTERS  0
#endif

// Control presence of exception handling (try and auto discover):

#ifndef nsvp_CONFIG_NO_EXCEPTIONS
# if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#  define nsvp_CONFIG_NO_EXCEPTIONS  0
# else
#  define nsvp_CONFIG_NO_EXCEPTIONS  1
# endif
#endif

// C++ language version detection (C++20 is speculative):
// Note: VC14.0/1900 (VS2015) lacks too much from C++14.

#ifndef   nsvp_CPLUSPLUS
# if defined(_MSVC_LANG ) && !defined(__clang__)
#  define nsvp_CPLUSPLUS  (_MSC_VER == 1900 ? 201103L : _MSVC_LANG )
# else
#  define nsvp_CPLUSPLUS  __cplusplus
# endif
#endif

#define nsvp_CPP98_OR_GREATER  ( nsvp_CPLUSPLUS >= 199711L )
#define nsvp_CPP11_OR_GREATER  ( nsvp_CPLUSPLUS >= 201103L )
#define nsvp_CPP11_OR_GREATER_ ( nsvp_CPLUSPLUS >= 201103L )
#define nsvp_CPP14_OR_GREATER  ( nsvp_CPLUSPLUS >= 201402L )
#define nsvp_CPP17_OR_GREATER  ( nsvp_CPLUSPLUS >= 201703L )
#define nsvp_CPP20_OR_GREATER  ( nsvp_CPLUSPLUS >= 202000L )

// half-open range [lo..hi):
#define nsvp_BETWEEN( v, lo, hi ) ( (lo) <= (v) && (v) < (hi) )

// Compiler versions:
//
// MSVC++ 6.0  _MSC_VER == 1200 (Visual Studio 6.0)
// MSVC++ 7.0  _MSC_VER == 1300 (Visual Studio .NET 2002)
// MSVC++ 7.1  _MSC_VER == 1310 (Visual Studio .NET 2003)
// MSVC++ 8.0  _MSC_VER == 1400 (Visual Studio 2005)
// MSVC++ 9.0  _MSC_VER == 1500 (Visual Studio 2008)
// MSVC++ 10.0 _MSC_VER == 1600 (Visual Studio 2010)
// MSVC++ 11.0 _MSC_VER == 1700 (Visual Studio 2012)
// MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)
// MSVC++ 14.0 _MSC_VER == 1900 (Visual Studio 2015)
// MSVC++ 14.1 _MSC_VER >= 1910 (Visual Studio 2017)

#if defined(_MSC_VER ) && !defined(__clang__)
# define nsvp_COMPILER_MSVC_VER      (_MSC_VER )
# define nsvp_COMPILER_MSVC_VERSION  (_MSC_VER / 10 - 10 * ( 5 + (_MSC_VER < 1900 ) ) )
#else
# define nsvp_COMPILER_MSVC_VER      0
# define nsvp_COMPILER_MSVC_VERSION  0
#endif

#define nsvp_COMPILER_VERSION( major, minor, patch )  ( 10 * ( 10 * (major) + (minor) ) + (patch) )

#if defined(__clang__)
# define nsvp_COMPILER_CLANG_VERSION  nsvp_COMPILER_VERSION(__clang_major__, __clang_minor__, __clang_patchlevel__)
#else
# define nsvp_COMPILER_CLANG_VERSION  0
#endif

#if defined(__GNUC__) && !defined(__clang__)
# define nsvp_COMPILER_GNUC_VERSION  nsvp_COMPILER_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
# define nsvp_COMPILER_GNUC_VERSION  0
#endif

#if nsvp_BETWEEN( nsvp_COMPILER_MSVC_VER, 1300, 1900 )
# pragma warning( push )
# pragma warning( disable: 4345 )   // initialization behavior changed
#endif

// Presence of language and library features:

#define nsvp_HAVE( feature )  ( nsvp_HAVE_##feature )

#ifdef _HAS_CPP0X
# define nsvp_HAS_CPP0X  _HAS_CPP0X
#else
# define nsvp_HAS_CPP0X  0
#endif

// Unless defined otherwise below, consider VC14 as C++11 for value_ptr-lite:

#if nsvp_COMPILER_MSVC_VER >= 1900
# undef  nsvp_CPP11_OR_GREATER
# define nsvp_CPP11_OR_GREATER  1
#endif

#define nsvp_CPP11_90   (nsvp_CPP11_OR_GREATER_ || nsvp_COMPILER_MSVC_VER >= 1500)
#define nsvp_CPP11_100  (nsvp_CPP11_OR_GREATER_ || nsvp_COMPILER_MSVC_VER >= 1600)
#define nsvp_CPP11_110  (nsvp_CPP11_OR_GREATER_ || nsvp_COMPILER_MSVC_VER >= 1700)
#define nsvp_CPP11_120  (nsvp_CPP11_OR_GREATER_ || nsvp_COMPILER_MSVC_VER >= 1800)
#define nsvp_CPP11_140  (nsvp_CPP11_OR_GREATER_ || nsvp_COMPILER_MSVC_VER >= 1900)
#define nsvp_CPP11_141  (nsvp_CPP11_OR_GREATER_ || nsvp_COMPILER_MSVC_VER >= 1910)

#define nsvp_CPP14_000  (nsvp_CPP14_OR_GREATER)
#define nsvp_CPP17_000  (nsvp_CPP17_OR_GREATER)

// empty bases:

#if nsvp_COMPILER_MSVC_VER >= 1900
# define nsvp_DECLSPEC_EMPTY_BASES  __declspec(empty_bases)
#else
# define nsvp_DECLSPEC_EMPTY_BASES
#endif

// Presence of C++11 language features:

#define nsvp_HAVE_CONSTEXPR_11          nsvp_CPP11_140
#define nsvp_HAVE_INITIALIZER_LIST      nsvp_CPP11_120
#define nsvp_HAVE_IS_DEFAULT            nsvp_CPP11_140
#define nsvp_HAVE_NOEXCEPT              nsvp_CPP11_140
#define nsvp_HAVE_NULLPTR               nsvp_CPP11_100
#define nsvp_HAVE_REF_QUALIFIER         nsvp_CPP11_140

// Presence of C++14 language features:

#define nsvp_HAVE_CONSTEXPR_14          nsvp_CPP14_000

// Presence of C++17 language features:

// no flag

// Presence of C++ library features:

#define nsvp_HAVE_TR1_TYPE_TRAITS       (!! nsvp_COMPILER_GNUC_VERSION )
#define nsvp_HAVE_TR1_ADD_POINTER       (!! nsvp_COMPILER_GNUC_VERSION )

#define nsvp_HAVE_TYPE_TRAITS           nsvp_CPP11_90

// C++ feature usage:

#if nsvp_HAVE_CONSTEXPR_11
# define nsvp_constexpr constexpr
#else
# define nsvp_constexpr /*constexpr*/
#endif

#if nsvp_HAVE_CONSTEXPR_14
# define nsvp_constexpr14 constexpr
#else
# define nsvp_constexpr14 /*constexpr*/
#endif

#if nsvp_HAVE_NOEXCEPT
# define nsvp_noexcept noexcept
# define nsvp_noexcept_op noexcept
#else
# define nsvp_noexcept /*noexcept*/
# define nsvp_noexcept_op(expr) /*noexcept(expr)*/
#endif

#if nsvp_HAVE_NULLPTR
# define nsvp_nullptr nullptr
#else
# define nsvp_nullptr NULL
#endif

#if nsvp_HAVE_REF_QUALIFIER
# define nsvp_ref_qual  &
# define nsvp_refref_qual  &&
#else
# define nsvp_ref_qual  /*&*/
# define nsvp_refref_qual  /*&&*/
#endif

// additional includes:

#if ! nsvp_CPP11_OR_GREATER
# include <algorithm>           // std::swap() until C++11
#endif

#if nsvp_HAVE_INITIALIZER_LIST
# include <initializer_list>
#endif

#if nsvp_HAVE_TYPE_TRAITS
# include <type_traits>
#elif nsvp_HAVE_TR1_TYPE_TRAITS
# include <tr1/type_traits>
#endif

// static assert:

#if nsvp_CPP11_OR_GREATER
# define nsvp_static_assert( expr, msg ) \
    static_assert( expr, msg )
#else
# define nsvp_static_assert( expr, msg ) \
    do { typedef int x[(expr) ? 1 : -1]; } while(0)
#endif

// Method enabling

#if nsvp_CPP11_OR_GREATER

#define nsvp_REQUIRES_0(...) \
    template< bool B = (__VA_ARGS__), typename std::enable_if<B, int>::type = 0 >

#define nsvp_REQUIRES_T(...) \
    , typename = typename std::enable_if< (__VA_ARGS__), nonstd::vptr::detail::enabler >::type

#define nsvp_REQUIRES_R(R, ...) \
    typename std::enable_if< (__VA_ARGS__), R>::type

#define nsvp_REQUIRES_A(...) \
    , typename std::enable_if< (__VA_ARGS__), void*>::type = nullptr

#endif

#include <cassert>
#include <functional>
#include <memory>
#include <utility>

#if ! nsvp_CONFIG_NO_EXCEPTIONS
# include <stdexcept>
#endif

//
// in_place: code duplicated in any-lite, expected-lite, optional-lite, value-ptr-lite, variant-lite:
//

#ifndef nonstd_lite_HAVE_IN_PLACE_TYPES
#define nonstd_lite_HAVE_IN_PLACE_TYPES  1

// C++17 std::in_place in <utility>:

#if nsvp_CPP17_OR_GREATER

#include <utility>

namespace nonstd {

using std::in_place;
using std::in_place_type;
using std::in_place_index;
using std::in_place_t;
using std::in_place_type_t;
using std::in_place_index_t;

#define nonstd_lite_in_place_t(      T)  std::in_place_t
#define nonstd_lite_in_place_type_t( T)  std::in_place_type_t<T>
#define nonstd_lite_in_place_index_t(K)  std::in_place_index_t<K>

#define nonstd_lite_in_place(      T)    std::in_place_t{}
#define nonstd_lite_in_place_type( T)    std::in_place_type_t<T>{}
#define nonstd_lite_in_place_index(K)    std::in_place_index_t<K>{}

} // namespace nonstd

#else // nsvp_CPP17_OR_GREATER

#include <cstddef>

namespace nonstd {
namespace detail {

template< class T >
struct in_place_type_tag {};

template< std::size_t K >
struct in_place_index_tag {};

} // namespace detail

struct in_place_t {};

template< class T >
inline in_place_t in_place( detail::in_place_type_tag<T> = detail::in_place_type_tag<T>() )
{
    return in_place_t();
}

template< std::size_t K >
inline in_place_t in_place( detail::in_place_index_tag<K> = detail::in_place_index_tag<K>() )
{
    return in_place_t();
}

template< class T >
inline in_place_t in_place_type( detail::in_place_type_tag<T> = detail::in_place_type_tag<T>() )
{
    return in_place_t();
}

template< std::size_t K >
inline in_place_t in_place_index( detail::in_place_index_tag<K> = detail::in_place_index_tag<K>() )
{
    return in_place_t();
}

// mimic templated typedef:

#define nonstd_lite_in_place_t(      T)  nonstd::in_place_t(&)( nonstd::detail::in_place_type_tag<T>  )
#define nonstd_lite_in_place_type_t( T)  nonstd::in_place_t(&)( nonstd::detail::in_place_type_tag<T>  )
#define nonstd_lite_in_place_index_t(K)  nonstd::in_place_t(&)( nonstd::detail::in_place_index_tag<K> )

#define nonstd_lite_in_place(      T)    nonstd::in_place_type<T>
#define nonstd_lite_in_place_type( T)    nonstd::in_place_type<T>
#define nonstd_lite_in_place_index(K)    nonstd::in_place_index<K>

} // namespace nonstd

#endif // nsvp_CPP17_OR_GREATER
#endif // nonstd_lite_HAVE_IN_PLACE_TYPES

//
// value_ptr:
//

namespace nonstd { namespace vptr {

#if nsvp_CPP11_OR_GREATER

namespace std20 {

// type traits C++20:

template< typename T >
struct remove_cvref
{
    typedef typename std::remove_cv< typename std::remove_reference<T>::type >::type type;
};

} // namespace std20

#endif // nsvp_CPP11_OR_GREATER

namespace detail {

/*enum*/ class  enabler{};

#if nsvp_CPP11_OR_GREATER
using std::default_delete;
#else
template< class T >
struct default_delete
{
    default_delete() nsvp_noexcept {};

    void operator()( T * ptr ) const nsvp_noexcept
    {
        nsvp_static_assert( sizeof(T) > 0, "default_delete cannot delete incomplete type");
#if nsvp_CPP11_OR_GREATER
        nsvp_static_assert( ! std::is_void<T>::value, "default_delete cannot delete incomplete type");
#endif
        delete ptr;
    }
};
#endif

template< class T >
struct default_clone
{
#if nsvp_CPP11_OR_GREATER
    default_clone() = default;
#else
    default_clone() {};
#endif

    T * operator()( T const & x ) const
    {
        nsvp_static_assert( sizeof(T) > 0, "default_clone cannot clone incomplete type");
#if nsvp_CPP11_OR_GREATER
        nsvp_static_assert( ! std::is_void<T>::value, "default_clone cannot clone incomplete type");
#endif
        return new T( x );
    }

#if nsvp_CPP11_OR_GREATER
    T * operator()( T && x ) const
    {
        return new T( std::move( x ) );
    }

    template< class... Args >
    T * operator()( nonstd_lite_in_place_t(T), Args&&... args ) const
    {
        return new T( std::forward<Args>(args)...);
    }

    template< class U, class... Args >
    T * operator()( nonstd_lite_in_place_t(T), std::initializer_list<U> il, Args&&... args ) const
    {
        return new T( il, std::forward<Args>(args)...);
    }
#endif
};

template <class T, class Cloner, class Deleter>
struct nsvp_DECLSPEC_EMPTY_BASES compressed_ptr : Cloner, Deleter
{
    typedef T       element_type;
    typedef T *     pointer;

    typedef Cloner  cloner_type;
    typedef Deleter deleter_type;

    // Lifetime:

    ~compressed_ptr()
    {
        deleter_type()( ptr );
    }

    compressed_ptr() nsvp_noexcept
    : ptr( nsvp_nullptr )
    {}

    explicit compressed_ptr( pointer p ) nsvp_noexcept
    : ptr( p )
    {}

    compressed_ptr( compressed_ptr const & other )
    : cloner_type ( other )
    , deleter_type( other )
    , ptr( other.ptr ? cloner_type()( *other.ptr ) : nsvp_nullptr )
    {}

#if  nsvp_CPP11_OR_GREATER
    compressed_ptr( compressed_ptr && other ) nsvp_noexcept
    : cloner_type ( std::move( other ) )
    , deleter_type( std::move( other ) )
    , ptr( std::move( other.ptr ) )
    {
        other.ptr = nullptr;
    }
#endif

    explicit compressed_ptr( element_type const & value )
    : ptr( cloner_type()( value ) )
    {}

#if  nsvp_CPP11_OR_GREATER

    explicit compressed_ptr( element_type && value ) nsvp_noexcept
    : ptr( cloner_type()( std::move( value ) ) )
    {}

    template< class... Args >
    explicit compressed_ptr( nonstd_lite_in_place_t(T), Args&&... args )
    : ptr( cloner_type()( nonstd_lite_in_place(T), std::forward<Args>(args)...) )
    {}

    template< class U, class... Args >
    explicit compressed_ptr( nonstd_lite_in_place_t(T), std::initializer_list<U> il, Args&&... args )
    : ptr( cloner_type()( nonstd_lite_in_place(T), il, std::forward<Args>(args)...) )
    {}

#endif

    compressed_ptr( element_type const & value, cloner_type const & cloner )
    : cloner_type ( cloner  )
    , ptr( cloner_type()( value ) )
    {}

#if  nsvp_CPP11_OR_GREATER
    compressed_ptr( element_type && value, cloner_type && cloner ) nsvp_noexcept
    : cloner_type ( std::move( cloner  ) )
    , ptr( cloner_type()( std::move( value ) ) )
    {}
#endif

    compressed_ptr( element_type const & value, cloner_type const & cloner, deleter_type const & deleter )
    : cloner_type ( cloner  )
    , deleter_type( deleter )
    , ptr( cloner_type()( value ) )
    {}

#if  nsvp_CPP11_OR_GREATER
    compressed_ptr( element_type && value, cloner_type && cloner, deleter_type && deleter ) nsvp_noexcept
    : cloner_type ( std::move( cloner  ) )
    , deleter_type( std::move( deleter ) )
    , ptr( cloner_type()( std::move( value ) ) )
    {}
#endif

    explicit compressed_ptr( cloner_type const & cloner )
    : cloner_type( cloner )
    , ptr( nsvp_nullptr )
    {}

#if  nsvp_CPP11_OR_GREATER
    explicit compressed_ptr( cloner_type && cloner ) nsvp_noexcept
    : cloner_type( std::move( cloner ) )
    , ptr( nsvp_nullptr )
    {}
#endif

    explicit compressed_ptr( deleter_type const & deleter )
    : deleter_type( deleter )
    , ptr( nsvp_nullptr )
    {}

# if  nsvp_CPP11_OR_GREATER
    explicit compressed_ptr( deleter_type && deleter ) nsvp_noexcept
    : deleter_type( std::move( deleter ) )
    , ptr( nsvp_nullptr )
    {}
#endif

    compressed_ptr( cloner_type const & cloner, deleter_type const & deleter )
    : cloner_type ( cloner  )
    , deleter_type( deleter )
    , ptr( nsvp_nullptr )
    {}

#if  nsvp_CPP11_OR_GREATER
    compressed_ptr( cloner_type && cloner, deleter_type && deleter ) nsvp_noexcept
    : cloner_type ( std::move( cloner  ) )
    , deleter_type( std::move( deleter ) )
    , ptr( nsvp_nullptr )
    {}
#endif

    // Observers:

    pointer get() const nsvp_noexcept
    {
        return ptr;
    }

    cloner_type & get_cloner() nsvp_noexcept
    {
        return *this;
    }

    deleter_type & get_deleter() nsvp_noexcept
    {
        return *this;
    }

    // Modifiers:

    pointer release() nsvp_noexcept
    {
        using std::swap;
        pointer result = nsvp_nullptr;
        swap( result, ptr );
        return result;
    }

    void reset( pointer p ) nsvp_noexcept
    {
        get_deleter()( ptr );
        ptr = p;
    }

    void reset( element_type const & v )
    {
        reset( get_cloner()( v ) );
    }

#if  nsvp_CPP11_OR_GREATER
    void reset( element_type && v )
    {
        reset( get_cloner()( std::move( v ) ) );
    }
#endif

    void swap(compressed_ptr& other) nsvp_noexcept
    {
        using std::swap;
        swap(ptr, other.ptr);
    }

    pointer ptr;
};

} // namespace detail

#if ! nsvp_CONFIG_NO_EXCEPTIONS

// value_ptr access error

class bad_value_access : public std::logic_error
{
public:
    explicit bad_value_access()
    : logic_error( "bad value_ptr access" ) {}
};

#endif

// class value_ptr:

template
<
    class T
    , class Cloner  = detail::default_clone<T>
    , class Deleter = detail::default_delete<T>
>
class value_ptr
{
public:
    typedef T         element_type;
    typedef T *       pointer;
    typedef T &       reference;
    typedef T const * const_pointer;
    typedef T const & const_reference;

    typedef Cloner   cloner_type;
    typedef Deleter  deleter_type;

    // Lifetime

#if nsvp_HAVE_IS_DEFAULT
    ~value_ptr() = default;
#endif

    value_ptr() nsvp_noexcept
    : ptr( cloner_type(), deleter_type() )
    {}

#if nsvp_HAVE_NULLPTR
    explicit value_ptr( std::nullptr_t ) nsvp_noexcept
    : ptr( cloner_type(), deleter_type() )
    {}
#endif

    explicit value_ptr( pointer p ) nsvp_noexcept
    : ptr( p )
    {}

    value_ptr(value_ptr const& other)
        : ptr(other.ptr)
    {}

#if nsvp_CPP11_OR_GREATER
    value_ptr( value_ptr && other ) nsvp_noexcept
    : ptr( std::move( other.ptr ) )
    {}
#endif

    explicit value_ptr( element_type const & value )
    : ptr( value )
    {}

#if nsvp_CPP11_OR_GREATER

    explicit value_ptr( element_type && value ) nsvp_noexcept
    : ptr( std::move( value ) )
    {}

#endif // nsvp_CPP11_OR_GREATER

    explicit value_ptr( cloner_type const & cloner )
    : ptr( cloner )
    {}

#if  nsvp_CPP11_OR_GREATER
    explicit value_ptr( cloner_type && cloner ) nsvp_noexcept
    : ptr( std::move( cloner ) )
    {}
#endif

    explicit value_ptr( deleter_type const & deleter )
    : ptr( deleter )
    {}

#if  nsvp_CPP11_OR_GREATER
    explicit value_ptr( deleter_type && deleter ) nsvp_noexcept
    : ptr( std::move( deleter ) )
    {}
#endif

#if  nsvp_CPP11_OR_GREATER
    template< class V, class ClonerOrDeleter
        nsvp_REQUIRES_T(
            !std::is_same<typename std20::remove_cvref<V>::type, nonstd_lite_in_place_t(V)>::value )
    >
    value_ptr( V && value, ClonerOrDeleter && cloner_or_deleter )
    : ptr( std::forward<V>( value ), std::forward<ClonerOrDeleter>( cloner_or_deleter ) )
    {}
#else
    template< class V, class ClonerOrDeleter >
    value_ptr( V const & value, ClonerOrDeleter const & cloner_or_deleter )
    : ptr( value, cloner_or_deleter )
    {}
#endif

#if  nsvp_CPP11_OR_GREATER
    template< class V, class C, class D
        nsvp_REQUIRES_T(
            !std::is_same<typename std20::remove_cvref<V>::type, nonstd_lite_in_place_t(V)>::value )
    >
    value_ptr( V && value, C && cloner, D && deleter )
    : ptr( std::forward<V>( value ), std::forward<C>( cloner ), std::forward<D>( deleter ) )
    {}
#else
    template< class V, class C, class D >
    value_ptr( V const & value, C const & cloner, D const & deleter )
    : ptr( value, cloner, deleter )
    {}
#endif

#if nsvp_HAVE_NULLPTR
    value_ptr & operator=( std::nullptr_t ) nsvp_noexcept
    {
        ptr.reset( nullptr );
        return *this;
    }
#endif

    value_ptr & operator=( T const & value )
    {
        ptr.reset( value );
        return *this;
    }

#if nsvp_CPP11_OR_GREATER
    template< class U
        nsvp_REQUIRES_T(
            std::is_same< typename std::decay<U>::type, T>::value )
    >
    value_ptr & operator=( U && value )
    {
        ptr.reset( std::forward<U>( value ) );
        return *this;
    }
#endif

    value_ptr & operator=( value_ptr const & rhs )
    {
        if ( this == &rhs )
            return *this;

        if ( rhs ) ptr.reset( *rhs );
#if nsvp_HAVE_NULLPTR
        else       ptr.reset( nullptr );
#else
        else       ptr.reset( pointer(0) );
#endif
        return *this;
    }

#if  nsvp_CPP11_OR_GREATER

    value_ptr & operator=( value_ptr && rhs ) nsvp_noexcept
    {
        if ( this == &rhs )
            return *this;

        swap( rhs );

        return *this;
    }

    template< class... Args >
    void emplace( Args&&... args )
    {
        ptr.reset( T( std::forward<Args>(args)...) );
    }

    template< class U, class... Args >
    void emplace( std::initializer_list<U> il, Args&&... args )
    {
        ptr.reset( T( il, std::forward<Args>(args)...) );
    }

#endif // nsvp_CPP11_OR_GREATER

    // Observers:

    pointer get() const nsvp_noexcept
    {
        return ptr.get();
    }

    cloner_type & get_cloner() nsvp_noexcept
    {
        return ptr.get_cloner();
    }

    deleter_type & get_deleter() nsvp_noexcept
    {
        return ptr.get_deleter();
    }

    reference operator*() const
    {
        assert( get() != nsvp_nullptr ); return *get();
    }

    pointer operator->() const nsvp_noexcept
    {
        assert( get() != nsvp_nullptr ); return get();
    }

#if  nsvp_CPP11_OR_GREATER
    explicit operator bool() const nsvp_noexcept
    {
        return has_value();
    }
#else
private:
    typedef void (value_ptr::*safe_bool)() const;
    void this_type_does_not_support_comparisons() const {}

public:
    operator safe_bool() const nsvp_noexcept
    {
        return has_value() ? &value_ptr::this_type_does_not_support_comparisons : 0;
    }
#endif

    bool has_value() const nsvp_noexcept
    {
        return !! get();
    }

    element_type const & value() const
    {
#if nsvp_CONFIG_NO_EXCEPTIONS
        assert( has_value() );
#else
        if ( ! has_value() )
        {
            throw bad_value_access();
        }
#endif
        return *get();
    }

    element_type & value()
    {
#if nsvp_CONFIG_NO_EXCEPTIONS
        assert( has_value() );
#else
        if ( ! has_value() )
        {
            throw bad_value_access();
        }
#endif
        return *get();
    }

#if nsvp_CPP11_OR_GREATER


#else

    template< class U >
    element_type value_or( U const & v ) const
    {
        return has_value() ? value() : static_cast<element_type>( v );
    }

#endif // nsvp_CPP11_OR_GREATER

    // Modifiers:

    pointer release() nsvp_noexcept
    {
        return ptr.release();
    }

    void reset( pointer p = pointer() ) nsvp_noexcept
    {
        ptr.reset( p );
    }

    void swap( value_ptr & other ) nsvp_noexcept
    {
        ptr.swap( other.ptr );
    }

private:
    detail::compressed_ptr<T, Cloner, Deleter> ptr;
};

// Non-member functions:

#if nsvp_CPP11_OR_GREATER

template< class T >
inline value_ptr< typename std::decay<T>::type > make_value( T && v )
{
    return value_ptr< typename std::decay<T>::type >( std::forward<T>( v ) );
}

template< class T, class... Args >
inline value_ptr<T> make_value( Args&&... args )
{
    return value_ptr<T>( in_place, std::forward<Args>(args)...);
}

template< class T, class U, class... Args >
inline value_ptr<T> make_value( std::initializer_list<U> il, Args&&... args )
{
    return value_ptr<T>( in_place, il, std::forward<Args>(args)...);
}

#else

template< typename T >
inline value_ptr<T> make_value( T const & value )
{
    return value_ptr<T>( value );
}

#endif // nsvp_CPP11_OR_GREATER

// Comparison between value_ptr-s:

#if nsvp_CONFIG_COMPARE_POINTERS

// compare pointers:

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator==(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
    return lhs.get() == rhs.get();
}

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator!=(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
    return ! ( lhs == rhs );
}

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator<(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
#if nsvp_CPP11_OR_GREATER
    using P1 = typename value_ptr<T1, D1, C1>::const_pointer;
    using P2 = typename value_ptr<T2, D2, C2>::const_pointer;
    using CT = typename std::common_type<P1, P2>::type;
    return std::less<CT>()( lhs.get(), rhs.get() );
#else
    return std::less<T1 const *>()( lhs.get(), rhs.get() );
#endif
}

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator<=(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
    return !( rhs < lhs );
}

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator>(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
    return rhs < lhs;
}

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator>=(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
    return !( lhs < rhs );
}

// Comparison with std::nullptr_t:

#if nsvp_HAVE_NULLPTR

template< class T, class D, class C  >
inline bool operator==( value_ptr<T, D, C> const & lhs, std::nullptr_t ) nsvp_noexcept
{
    return ! lhs;
}

template< class T, class D, class C  >
inline bool operator==( std::nullptr_t, value_ptr<T, D, C> const & rhs ) nsvp_noexcept
{
    return ! rhs;
}

template< class T, class D, class C  >
inline bool operator!=( value_ptr<T, D, C> const & lhs, std::nullptr_t ) nsvp_noexcept
{
    return static_cast<bool>( lhs );
}

template< class T, class D, class C  >
inline bool operator!=( std::nullptr_t, value_ptr<T, D, C> const & rhs ) nsvp_noexcept
{
    return static_cast<bool>( rhs );
}

template< class T, class D, class C  >
inline bool operator<( value_ptr<T, D, C> const & lhs, std::nullptr_t )
{
    typedef typename value_ptr<T, D, C>::const_pointer P;
    return std::less<P>()( lhs.get(), nullptr );
}

template< class T, class D, class C  >
inline bool operator<( std::nullptr_t, value_ptr<T, D, C> const & rhs )
{
    typedef typename value_ptr<T, D, C>::const_pointer P;
    return std::less<P>()( nullptr, rhs.get() );
}

template< class T, class D, class C  >
inline bool operator<=( value_ptr<T, D, C> const & lhs, std::nullptr_t )
{
    return !( nullptr < lhs );
}

template< class T, class D, class C  >
inline bool operator<=( std::nullptr_t, value_ptr<T, D, C> const & rhs )
{
    return !( rhs < nullptr );
}

template< class T, class D, class C  >
inline bool operator>( value_ptr<T, D, C> const & lhs, std::nullptr_t )
{
    return nullptr < lhs;
}

template< class T, class D, class C  >
inline bool operator>( std::nullptr_t, value_ptr<T, D, C> const & rhs )
{
    return rhs < nullptr;
}

template< class T, class D, class C  >
inline bool operator>=( value_ptr<T, D, C> const & lhs, std::nullptr_t )
{
    return !( lhs < nullptr );
}

template< class T, class D, class C  >
inline bool operator>=( std::nullptr_t, value_ptr<T, D, C> const & rhs )
{
    return !( nullptr < rhs );
}

#endif // nsvp_HAVE_NULLPTR

#else  // nsvp_CONFIG_COMPARE_POINTERS

// compare content:

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator==(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
    return bool(lhs) != bool(rhs) ? false : bool(lhs) == false ? true : *lhs == *rhs;
}

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator!=(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
    return ! ( lhs == rhs );
}

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator<(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
//#if nsvp_CPP11_OR_GREATER
//    using E1 = typename value_ptr<T1, D1, C1>::element_type;
//    using E2 = typename value_ptr<T2, D2, C2>::element_type;
//    using CT = typename std::common_type<E1, E2>::type;
//    return std::less<CT>()( *lhs, *rhs );
//#else
//    return std::less<T1>()( *lhs, *rhs );
//#endif

    return (!rhs) ? false : (!lhs) ? true : *lhs < *rhs;
}

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator<=(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
    return !( rhs < lhs );
}

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator>(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
    return rhs < lhs;
}

template<
    class T1, class D1, class C1,
    class T2, class D2, class C2
>
inline bool operator>=(
    value_ptr<T1, D1, C1> const & lhs,
    value_ptr<T2, D2, C2> const & rhs )
{
    return !( lhs < rhs );
}

// compare with value:

template< class T, class C, class D >
bool operator==( value_ptr<T,C,D> const & vp, T const & value )
{
    return bool(vp) ? *vp == value : false;
}

template< class T, class C, class D >
bool operator==( T const & value, value_ptr<T,C,D> const & vp )
{
    return bool(vp) ? value == *vp : false;
}

template< class T, class C, class D >
bool operator!=( value_ptr<T,C,D> const & vp, T const & value )
{
    return bool(vp) ? *vp != value : true;
}

template< class T, class C, class D >
bool operator!=( T const & value, value_ptr<T,C,D> const & vp )
{
    return bool(vp) ? value != *vp : true;
}

template< class T, class C, class D >
bool operator<( value_ptr<T,C,D> const & vp, T const & value )
{
    return bool(vp) ? *vp < value : true;
}

template< class T, class C, class D >
bool operator<( T const & value, value_ptr<T,C,D> const & vp )
{
    return bool(vp) ? value < *vp : false;
}

template< class T, class C, class D >
bool operator<=( value_ptr<T,C,D> const & vp, T const & value )
{
    return bool(vp) ? *vp <= value : true;
}

template< class T, class C, class D >
bool operator<=( T const & value, value_ptr<T,C,D> const & vp )
{
    return bool(vp) ? value <= *vp : false;
}

template< class T, class C, class D >
bool operator>( value_ptr<T,C,D> const & vp, T const & value )
{
    return bool(vp) ? *vp > value : false;
}

template< class T, class C, class D >
bool operator>( T const & value, value_ptr<T,C,D> const & vp )
{
    return bool(vp) ? value > *vp : true;
}

template< class T, class C, class D >
bool operator>=( value_ptr<T,C,D> const & vp, T const & value )
{
    return bool(vp) ? *vp >= value : false;
}

template< class T, class C, class D >
bool operator>=( T const & value, value_ptr<T,C,D> const & vp )
{
    return bool(vp) ? value >= *vp : true;
}

#endif // nsvp_CONFIG_COMPARE_POINTERS

// swap:

template< class T, class D, class C >
inline void swap(
    value_ptr<T, D, C> & lhs,
    value_ptr<T, D, C> & rhs ) nsvp_noexcept
{
    lhs.swap( rhs );
}

} // namespace vptr

using namespace vptr;

} // namespace nonstd

#if nsvp_CPP11_OR_GREATER

// Specialize the std::hash algorithm:

namespace std
{

template< class T, class D, class C >
struct hash< nonstd::value_ptr<T, D, C> >
{
    typedef nonstd::value_ptr<T, D, C> argument_type;
    typedef size_t result_type;

    result_type operator()( argument_type const & p ) const nsvp_noexcept
    {
        return hash<typename argument_type::const_pointer>()( p.get() );
    }
};

} // namespace std

#endif // nsvp_CPP11_OR_GREATER

#if nsvp_BETWEEN( nsvp_COMPILER_MSVC_VER, 1300, 1900 )
# pragma warning( pop )
#endif

#endif // NONSTD_VALUE_PTR_LITE_HPP

#endif
