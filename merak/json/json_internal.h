// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip.

#ifndef MERAK_JSON_JSON_INTERNAL_H_
#define MERAK_JSON_JSON_INTERNAL_H_

/*!\file merak/json.h
    \brief common definitions and configuration

    \see MERAK_JSON_CONFIG
 */

/*! \defgroup MERAK_JSON_CONFIG RapidJSON configuration
    \brief Configuration macros for library features

    Some RapidJSON features are configurable to adapt the library to a wide
    variety of platforms, environments and usage scenarios.  Most of the
    features can be configured in terms of overridden or predefined
    preprocessor macros at compile-time.

    Some additional customization is available in the \ref MERAK_JSON_ERRORS APIs.

    \note These macros should be given on the compiler command-line
          (where applicable)  to avoid inconsistent values when compiling
          different translation units of a single application.
 */

#include <cstdlib>  // malloc(), realloc(), free(), size_t
#include <cstring>  // memset(), memcpy(), memmove(), memcmp()

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_VERSION_STRING
//
// ALWAYS synchronize the following 3 macros with corresponding variables in /CMakeLists.txt.
//

//!@cond MERAK_JSON_HIDDEN_FROM_DOXYGEN
// token stringification
#define MERAK_JSON_STRINGIFY(x) MERAK_JSON_DO_STRINGIFY(x)
#define MERAK_JSON_DO_STRINGIFY(x) #x

// token concatenation
#define MERAK_JSON_JOIN(X, Y) MERAK_JSON_DO_JOIN(X, Y)
#define MERAK_JSON_DO_JOIN(X, Y) MERAK_JSON_DO_JOIN2(X, Y)
#define MERAK_JSON_DO_JOIN2(X, Y) X##Y
//!@endcond

/*! \def MERAK_JSON_MAJOR_VERSION
    \ingroup MERAK_JSON_CONFIG
    \brief Major version of RapidJSON in integer.
*/
/*! \def MERAK_JSON_MINOR_VERSION
    \ingroup MERAK_JSON_CONFIG
    \brief Minor version of RapidJSON in integer.
*/
/*! \def MERAK_JSON_PATCH_VERSION
    \ingroup MERAK_JSON_CONFIG
    \brief Patch version of RapidJSON in integer.
*/
/*! \def MERAK_JSON_VERSION_STRING
    \ingroup MERAK_JSON_CONFIG
    \brief Version of RapidJSON in "<major>.<minor>.<patch>" string format.
*/
#define MERAK_JSON_MAJOR_VERSION 1
#define MERAK_JSON_MINOR_VERSION 1
#define MERAK_JSON_PATCH_VERSION 0
#define MERAK_JSON_VERSION_STRING \
    MERAK_JSON_STRINGIFY(MERAK_JSON_MAJOR_VERSION.MERAK_JSON_MINOR_VERSION.MERAK_JSON_PATCH_VERSION)

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_NAMESPACE_(BEGIN|END)
/*! \def merak::json
    \ingroup MERAK_JSON_CONFIG
    \brief   provide custom rapidjson namespace

    In order to avoid symbol clashes and/or "One Definition Rule" errors
    between multiple inclusions of (different versions of) RapidJSON in
    a single binary, users can customize the name of the main RapidJSON
    namespace.

    In case of a single nesting level, defining \c merak::json
    to a custom name (e.g. \c MyRapidJSON) is sufficient.  If multiple
    levels are needed, both \ref namespace merak::json { and \ref
    }  // namespace merak::json need to be defined as well:

    \code
    // in some .cpp file
    #define merak::json my::rapidjson
    #define namespace merak::json { namespace my { namespace merak::json {
    #define }  // namespace merak::json   } }
    #include <merak/json/..."
    \endcode

    \see rapidjson
 */
/*! \def namespace merak::json {
    \ingroup MERAK_JSON_CONFIG
    \brief   provide custom rapidjson namespace (opening expression)
    \see merak::json
*/
/*! \def }  // namespace merak::json
    \ingroup MERAK_JSON_CONFIG
    \brief   provide custom rapidjson namespace (closing expression)
    \see merak::json
*/

///////////////////////////////////////////////////////////////////////////////
// __cplusplus macro

//!@cond MERAK_JSON_HIDDEN_FROM_DOXYGEN

#if defined(_MSC_VER)
#define MERAK_JSON_CPLUSPLUS _MSVC_LANG
#else
#define MERAK_JSON_CPLUSPLUS __cplusplus
#endif

//!@endcond

#include <string>

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_USE_MEMBERSMAP

/*! \def MERAK_JSON_USE_MEMBERSMAP
    \ingroup MERAK_JSON_CONFIG
    \brief Enable RapidJSON support for object members handling in a \c std::multimap

    By defining this preprocessor symbol to \c 1, \ref merak::json::GenericValue object
    members are stored in a \c std::multimap for faster lookup and deletion times, a
    trade off with a slightly slower insertion time and a small object allocat(or)ed
    memory overhead.

    \hideinitializer
*/
#ifndef MERAK_JSON_USE_MEMBERSMAP
#define MERAK_JSON_USE_MEMBERSMAP 0 // not by default
#endif

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_NO_INT64DEFINE

/*! \def MERAK_JSON_NO_INT64DEFINE
    \ingroup MERAK_JSON_CONFIG
    \brief Use external 64-bit integer types.

    RapidJSON requires the 64-bit integer types \c int64_t and  \c uint64_t types
    to be available at global scope.

    If users have their own definition, define MERAK_JSON_NO_INT64DEFINE to
    prevent RapidJSON from defining its own types.
*/
#ifndef MERAK_JSON_NO_INT64DEFINE
//!@cond MERAK_JSON_HIDDEN_FROM_DOXYGEN
#if defined(_MSC_VER) && (_MSC_VER < 1800) // Visual Studio 2013
#include <merak/json/msinttypes/stdint.h>
#include <merak/json/msinttypes/inttypes.h>
#else
// Other compilers should have this.
#include <stdint.h>
#include <inttypes.h>
#endif
//!@endcond
#ifdef MERAK_JSON_DOXYGEN_RUNNING
#define MERAK_JSON_NO_INT64DEFINE
#endif
#endif // MERAK_JSON_NO_INT64TYPEDEF

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_FORCEINLINE

#ifndef MERAK_JSON_FORCEINLINE
//!@cond MERAK_JSON_HIDDEN_FROM_DOXYGEN
#if defined(_MSC_VER) && defined(NDEBUG)
#define MERAK_JSON_FORCEINLINE __forceinline
#elif defined(__GNUC__) && __GNUC__ >= 4 && defined(NDEBUG)
#define MERAK_JSON_FORCEINLINE __attribute__((always_inline))
#else
#define MERAK_JSON_FORCEINLINE
#endif
//!@endcond
#endif // MERAK_JSON_FORCEINLINE

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_ENDIAN
#define MERAK_JSON_LITTLEENDIAN  0   //!< Little endian machine
#define MERAK_JSON_BIGENDIAN     1   //!< Big endian machine

//! Endianness of the machine.
/*!
    \def MERAK_JSON_ENDIAN
    \ingroup MERAK_JSON_CONFIG

    GCC 4.6 provided macro for detecting endianness of the target machine. But other
    compilers may not have this. User can define MERAK_JSON_ENDIAN to either
    \ref MERAK_JSON_LITTLEENDIAN or \ref MERAK_JSON_BIGENDIAN.

    Default detection implemented with reference to
    \li https://gcc.gnu.org/onlinedocs/gcc-4.6.0/cpp/Common-Predefined-Macros.html
    \li http://www.boost.org/doc/libs/1_42_0/boost/detail/endian.hpp
*/
#ifndef MERAK_JSON_ENDIAN
// Detect with GCC 4.6's macro
#  ifdef __BYTE_ORDER__
#    if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#      define MERAK_JSON_ENDIAN MERAK_JSON_LITTLEENDIAN
#    elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#      define MERAK_JSON_ENDIAN MERAK_JSON_BIGENDIAN
#    else
#      error Unknown machine endianness detected. User needs to define MERAK_JSON_ENDIAN.
#    endif // __BYTE_ORDER__
// Detect with GLIBC's endian.h
#  elif defined(__GLIBC__)
#    include <endian.h>
#    if (__BYTE_ORDER == __LITTLE_ENDIAN)
#      define MERAK_JSON_ENDIAN MERAK_JSON_LITTLEENDIAN
#    elif (__BYTE_ORDER == __BIG_ENDIAN)
#      define MERAK_JSON_ENDIAN MERAK_JSON_BIGENDIAN
#    else
#      error Unknown machine endianness detected. User needs to define MERAK_JSON_ENDIAN.
#   endif // __GLIBC__
// Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro
#  elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#    define MERAK_JSON_ENDIAN MERAK_JSON_LITTLEENDIAN
#  elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#    define MERAK_JSON_ENDIAN MERAK_JSON_BIGENDIAN
// Detect with architecture macros
#  elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || defined(__powerpc__) || defined(__ppc__) || defined(__ppc64__) || defined(__hpux) || defined(__hppa) || defined(_MIPSEB) || defined(_POWER) || defined(__s390__)
#    define MERAK_JSON_ENDIAN MERAK_JSON_BIGENDIAN
#  elif defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__) || defined(_M_IX86) || defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || defined(__bfin__)
#    define MERAK_JSON_ENDIAN MERAK_JSON_LITTLEENDIAN
#  elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#    define MERAK_JSON_ENDIAN MERAK_JSON_LITTLEENDIAN
#  elif defined(MERAK_JSON_DOXYGEN_RUNNING)
#    define MERAK_JSON_ENDIAN
#  else
#    error Unknown machine endianness detected. User needs to define MERAK_JSON_ENDIAN.
#  endif
#endif // MERAK_JSON_ENDIAN

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_64BIT

//! Whether using 64-bit architecture
#ifndef MERAK_JSON_64BIT
#if defined(__LP64__) || (defined(__x86_64__) && defined(__ILP32__)) || defined(_WIN64) || defined(__EMSCRIPTEN__)
#define MERAK_JSON_64BIT 1
#else
#define MERAK_JSON_64BIT 0
#endif
#endif // MERAK_JSON_64BIT

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_ALIGN

//! Data alignment of the machine.
/*! \ingroup MERAK_JSON_CONFIG
    \param x pointer to align

    Some machines require strict data alignment. The default is 8 bytes.
    User can customize by defining the MERAK_JSON_ALIGN function macro.
*/
#ifndef MERAK_JSON_ALIGN
#define MERAK_JSON_ALIGN(x) (((x) + static_cast<size_t>(7u)) & ~static_cast<size_t>(7u))
#endif

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_UINT64_C2

//! Construct a 64-bit literal by a pair of 32-bit integer.
/*!
    64-bit literal with or without ULL suffix is prone to compiler warnings.
    UINT64_C() is C macro which cause compilation problems.
    Use this macro to define 64-bit constants by a pair of 32-bit integer.
*/
#ifndef MERAK_JSON_UINT64_C2
#define MERAK_JSON_UINT64_C2(high32, low32) ((static_cast<uint64_t>(high32) << 32) | static_cast<uint64_t>(low32))
#endif

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_48BITPOINTER_OPTIMIZATION

//! Use only lower 48-bit address for some pointers.
/*!
    \ingroup MERAK_JSON_CONFIG

    This optimization uses the fact that current X86-64 architecture only implement lower 48-bit virtual address.
    The higher 16-bit can be used for storing other data.
    \c GenericValue uses this optimization to reduce its size form 24 bytes to 16 bytes in 64-bit architecture.
*/
#ifndef MERAK_JSON_48BITPOINTER_OPTIMIZATION
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define MERAK_JSON_48BITPOINTER_OPTIMIZATION 1
#else
#define MERAK_JSON_48BITPOINTER_OPTIMIZATION 0
#endif
#endif // MERAK_JSON_48BITPOINTER_OPTIMIZATION

#if MERAK_JSON_48BITPOINTER_OPTIMIZATION == 1
#if MERAK_JSON_64BIT != 1
#error MERAK_JSON_48BITPOINTER_OPTIMIZATION can only be set to 1 when MERAK_JSON_64BIT=1
#endif
#define MERAK_JSON_SETPOINTER(type, p, x) (p = reinterpret_cast<type *>((reinterpret_cast<uintptr_t>(p) & static_cast<uintptr_t>(MERAK_JSON_UINT64_C2(0xFFFF0000, 0x00000000))) | reinterpret_cast<uintptr_t>(reinterpret_cast<const void*>(x))))
#define MERAK_JSON_GETPOINTER(type, p) (reinterpret_cast<type *>(reinterpret_cast<uintptr_t>(p) & static_cast<uintptr_t>(MERAK_JSON_UINT64_C2(0x0000FFFF, 0xFFFFFFFF))))
#else
#define MERAK_JSON_SETPOINTER(type, p, x) (p = (x))
#define MERAK_JSON_GETPOINTER(type, p) (p)
#endif

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_SSE2/MERAK_JSON_SSE42/MERAK_JSON_NEON/MERAK_JSON_SIMD

/*! \def MERAK_JSON_SIMD
    \ingroup MERAK_JSON_CONFIG
    \brief Enable SSE2/SSE4.2/Neon optimization.

    RapidJSON supports optimized implementations for some parsing operations
    based on the SSE2, SSE4.2 or NEon SIMD extensions on modern Intel
    or ARM compatible processors.

    To enable these optimizations, three different symbols can be defined;
    \code
    // Enable SSE2 optimization.
    #define MERAK_JSON_SSE2

    // Enable SSE4.2 optimization.
    #define MERAK_JSON_SSE42
    \endcode

    // Enable ARM Neon optimization.
    #define MERAK_JSON_NEON
    \endcode

    \c MERAK_JSON_SSE42 takes precedence over SSE2, if both are defined.

    If any of these symbols is defined, RapidJSON defines the macro
    \c MERAK_JSON_SIMD to indicate the availability of the optimized code.
*/
#if defined(MERAK_JSON_SSE2) || defined(MERAK_JSON_SSE42) \
    || defined(MERAK_JSON_NEON) || defined(MERAK_JSON_DOXYGEN_RUNNING)
#define MERAK_JSON_SIMD
#endif

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_NO_SIZETYPEDEFINE

#ifndef MERAK_JSON_NO_SIZETYPEDEFINE
/*! \def MERAK_JSON_NO_SIZETYPEDEFINE
    \ingroup MERAK_JSON_CONFIG
    \brief User-provided \c SizeType definition.

    In order to avoid using 32-bit size types for indexing strings and arrays,
    define this preprocessor symbol and provide the type merak::json::SizeType
    before including RapidJSON:
    \code
    #define MERAK_JSON_NO_SIZETYPEDEFINE
    namespace merak::json { typedef ::std::size_t SizeType; }
    #include <merak/json/..."
    \endcode

    \see merak::json::SizeType
*/
#ifdef MERAK_JSON_DOXYGEN_RUNNING
#define MERAK_JSON_NO_SIZETYPEDEFINE
#endif
namespace merak::json {
//! size type (for string lengths, array sizes, etc.)
/*! RapidJSON uses 32-bit array/string indices even on 64-bit platforms,
    instead of using \c size_t. Users may override the SizeType by defining
    \ref MERAK_JSON_NO_SIZETYPEDEFINE.
*/
typedef unsigned SizeType;
}  // namespace merak::json
#endif

// always import std::size_t to rapidjson namespace
namespace merak::json {
using std::size_t;
}  // namespace merak::json

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_ASSERT

//! Assertion.
/*! \ingroup MERAK_JSON_CONFIG
    By default, rapidjson uses C \c assert() for internal assertions.
    User can override it by defining MERAK_JSON_ASSERT(x) macro.

    \note Parsing errors are handled and can be customized by the
          \ref MERAK_JSON_ERRORS APIs.
*/
#ifndef MERAK_JSON_ASSERT
#include <cassert>
#define MERAK_JSON_ASSERT(x) assert(x)
#endif // MERAK_JSON_ASSERT

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_STATIC_ASSERT

// Prefer C++11 static_assert, if available
#ifndef MERAK_JSON_STATIC_ASSERT
#define MERAK_JSON_STATIC_ASSERT(x) \
   static_assert(x, MERAK_JSON_STRINGIFY(x))
#endif // MERAK_JSON_STATIC_ASSERT

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_LIKELY, MERAK_JSON_UNLIKELY

//! Compiler branching hint for expression with high probability to be true.
/*!
    \ingroup MERAK_JSON_CONFIG
    \param x Boolean expression likely to be true.
*/
#ifndef MERAK_JSON_LIKELY
#if defined(__GNUC__) || defined(__clang__)
#define MERAK_JSON_LIKELY(x) __builtin_expect(!!(x), 1)
#else
#define MERAK_JSON_LIKELY(x) (x)
#endif
#endif

//! Compiler branching hint for expression with low probability to be true.
/*!
    \ingroup MERAK_JSON_CONFIG
    \param x Boolean expression unlikely to be true.
*/
#ifndef MERAK_JSON_UNLIKELY
#if defined(__GNUC__) || defined(__clang__)
#define MERAK_JSON_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define MERAK_JSON_UNLIKELY(x) (x)
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// Helpers

//!@cond MERAK_JSON_HIDDEN_FROM_DOXYGEN

#define MERAK_JSON_MULTILINEMACRO_BEGIN do {
#define MERAK_JSON_MULTILINEMACRO_END \
} while((void)0, 0)

// adopted from Boost
#define MERAK_JSON_VERSION_CODE(x,y,z) \
  (((x)*100000) + ((y)*100) + (z))

#if defined(__has_builtin)
#define MERAK_JSON_HAS_BUILTIN(x) __has_builtin(x)
#else
#define MERAK_JSON_HAS_BUILTIN(x) 0
#endif

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_DIAG_PUSH/POP, MERAK_JSON_DIAG_OFF

#if defined(__GNUC__)
#define MERAK_JSON_GNUC \
    MERAK_JSON_VERSION_CODE(__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__)
#endif

#if defined(__clang__) || (defined(MERAK_JSON_GNUC) && MERAK_JSON_GNUC >= MERAK_JSON_VERSION_CODE(4,2,0))

#define MERAK_JSON_PRAGMA(x) _Pragma(MERAK_JSON_STRINGIFY(x))
#define MERAK_JSON_DIAG_PRAGMA(x) MERAK_JSON_PRAGMA(GCC diagnostic x)
#define MERAK_JSON_DIAG_OFF(x) \
    MERAK_JSON_DIAG_PRAGMA(ignored MERAK_JSON_STRINGIFY(MERAK_JSON_JOIN(-W,x)))

// push/pop support in Clang and GCC>=4.6
#if defined(__clang__) || (defined(MERAK_JSON_GNUC) && MERAK_JSON_GNUC >= MERAK_JSON_VERSION_CODE(4,6,0))
#define MERAK_JSON_DIAG_PUSH MERAK_JSON_DIAG_PRAGMA(push)
#define MERAK_JSON_DIAG_POP  MERAK_JSON_DIAG_PRAGMA(pop)
#else // GCC >= 4.2, < 4.6
#define MERAK_JSON_DIAG_PUSH /* ignored */
#define MERAK_JSON_DIAG_POP /* ignored */
#endif

#elif defined(_MSC_VER)

// pragma (MSVC specific)
#define MERAK_JSON_PRAGMA(x) __pragma(x)
#define MERAK_JSON_DIAG_PRAGMA(x) MERAK_JSON_PRAGMA(warning(x))

#define MERAK_JSON_DIAG_OFF(x) MERAK_JSON_DIAG_PRAGMA(disable: x)
#define MERAK_JSON_DIAG_PUSH MERAK_JSON_DIAG_PRAGMA(push)
#define MERAK_JSON_DIAG_POP  MERAK_JSON_DIAG_PRAGMA(pop)

#else

#define MERAK_JSON_DIAG_OFF(x) /* ignored */
#define MERAK_JSON_DIAG_PUSH   /* ignored */
#define MERAK_JSON_DIAG_POP    /* ignored */

#endif // MERAK_JSON_DIAG_*

///////////////////////////////////////////////////////////////////////////////
// C++11 features

#include <utility> // std::move

#ifndef MERAK_JSON_HAS_CXX11_NOEXCEPT
#define MERAK_JSON_HAS_CXX11_NOEXCEPT 1
#endif
#ifndef MERAK_JSON_NOEXCEPT
#if MERAK_JSON_HAS_CXX11_NOEXCEPT
#define MERAK_JSON_NOEXCEPT noexcept
#else
#define MERAK_JSON_NOEXCEPT throw()
#endif // MERAK_JSON_HAS_CXX11_NOEXCEPT
#endif

//!@endcond

//! Assertion (in non-throwing contexts).
 /*! \ingroup MERAK_JSON_CONFIG
    Some functions provide a \c noexcept guarantee, if the compiler supports it.
    In these cases, the \ref MERAK_JSON_ASSERT macro cannot be overridden to
    throw an exception.  This macro adds a separate customization point for
    such cases.

    Defaults to C \c assert() (as \ref MERAK_JSON_ASSERT), if \c noexcept is
    supported, and to \ref MERAK_JSON_ASSERT otherwise.
 */

///////////////////////////////////////////////////////////////////////////////
// MERAK_JSON_NOEXCEPT_ASSERT

#ifndef MERAK_JSON_NOEXCEPT_ASSERT
#ifdef MERAK_JSON_ASSERT_THROWS
#include <cassert>
#define MERAK_JSON_NOEXCEPT_ASSERT(x) assert(x)
#else
#define MERAK_JSON_NOEXCEPT_ASSERT(x) MERAK_JSON_ASSERT(x)
#endif // MERAK_JSON_ASSERT_THROWS
#endif // MERAK_JSON_NOEXCEPT_ASSERT

///////////////////////////////////////////////////////////////////////////////
// malloc/realloc/free

#ifndef MERAK_JSON_MALLOC
///! customization point for global \c malloc
#define MERAK_JSON_MALLOC(size) std::malloc(size)
#endif
#ifndef MERAK_JSON_REALLOC
///! customization point for global \c realloc
#define MERAK_JSON_REALLOC(ptr, new_size) std::realloc(ptr, new_size)
#endif
#ifndef MERAK_JSON_FREE
///! customization point for global \c free
#define MERAK_JSON_FREE(ptr) std::free(ptr)
#endif

///////////////////////////////////////////////////////////////////////////////
// new/delete

#ifndef MERAK_JSON_NEW
///! customization point for global \c new
#define MERAK_JSON_NEW(TypeName) new TypeName
#endif
#ifndef MERAK_JSON_DELETE
///! customization point for global \c delete
#define MERAK_JSON_DELETE(x) delete x
#endif

///////////////////////////////////////////////////////////////////////////////
// Type

/*! \namespace merak::json
    \brief main RapidJSON namespace
    \see merak::json
*/
namespace merak::json {

//! Type of JSON value
enum Type {
    kNullType = 0,      //!< null
    kFalseType = 1,     //!< false
    kTrueType = 2,      //!< true
    kObjectType = 3,    //!< object
    kArrayType = 4,     //!< array
    kStringType = 5,    //!< string
    kNumberType = 6     //!< number
};

}  // namespace merak::json

#endif // MERAK_JSON_JSON_INTERNAL_H_
