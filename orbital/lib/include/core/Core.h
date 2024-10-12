#pragma once

#include <cstdint>
#include <limits>
#include <utility>
#include <memory>
#include <type_traits>

namespace bfc {
  enum class PlatformID {
    Windows,
    Linux,
    Mac
  };

  enum class CompilerID {
    MSVC,
    GCC,
    Clang,
    MinGW,
  };
}

// Production/Dev defines
#ifndef BFC_PRODUCTION_BUILD
#define BFC_DEV_BUILD
#endif

// Debug/Release defines
#ifdef DEBUG
#define BFC_DEBUG
#endif

#ifdef NDEBUG
#define BFC_RELEASE
#endif

#if defined(BFC_RELEASE) && defined(BFC_DEBUG)
static_assert(false, "NDEBUG and DEBUG are both defined. Please remove one.");
#endif

// Detect Platform
#if defined(_WIN64) || defined(_WIN32)
#define NOMINMAX
#define BFC_WINDOWS
#define BFC_PLATFORM ::bcf::PlatformID::Windows
#elif defined(__linux__)
#define BFC_LINUX
#define BFC_PLATFORM ::bcf::PlatformID::Linux
#elif defined(__APPLE__)
#define BFC_MAC
#define BFC_PLATFORM ::bcf::PlatformID::Mac
#endif

// Detect Compiler
#if defined(_MSC_VER)
#define BFC_MSVC
#define BFC_COMPILER ::bcf::CompilerID::MSVC
#define BFC_COMPILER_NAME "msvc"
#define BFC_FUNCTION __FUNCTION__
#define BFC_LINE __LINE__
#define BFC_FILE __FILE__
#define BFC_IMPORT __declspec(dllimport)
#define BFC_EXPORT __declspec(dllexport)
#ifdef _M_IX86
#define BFC_X86
#endif
#ifdef _M_X64
#define BFC_X64
#endif
#elif defined(__GNUC__)
#define BCF_GCC
#define BFC_COMPILER      ::bcf::CompilerID::GCC
#define BFC_COMPILER_NAME "gcc"
#define BFC_FUNCTION __FUNCSIG__
#define BFC_LINE __LINE__
#define BFC_FILE __FILE__
#define BFC_IMPORT __attribute__((dllimport))
#define BFC_EXPORT __attribute__((dllexport))
#elif defined(__clang__)
#define BCF_CLANG
#define BFC_COMPILER      ::bcf::CompilerID::Clang
#define BFC_COMPILER_NAME "clang"
#define BFC_FUNCTION __FUNCSIG__
#define BFC_LINE __LINE__
#define BFC_FILE __FILE__
#define BFC_IMPORT __attribute__((dllimport))
#define BFC_EXPORT __attribute__((dllexport))
#elif defined(__MINGW64__)
#define BCF_MINGW
#define BFC_COMPILER      ::bcf::CompilerID::MinGW
#define BFC_COMPILER_NAME "mingw"
#define BFC_FUNCTION __FUNCSIG__
#define BFC_LINE __LINE__
#define BFC_FILE __FILE__
#define BFC_IMPORT __attribute__((dllimport))
#define BFC_EXPORT __attribute__((dllexport))
#endif

#define BFC_STRINGIFY(x) #x
#define BFC_TOSTRING(x) BFC_STRINGIFY(x)

#ifdef BFC_STATIC
#define BFC_API
#else
#ifdef BFC_EXPORT_SYMBOLS
#define BFC_API BFC_EXPORT
#else
#define BFC_API BFC_IMPORT
#endif
#endif

#define BFC_UNUSED(...) (__VA_ARGS__)

#define BFC_RELASSERT(expression, messageFmt, ...) ::bfc::assertion(BFC_FILE, BFC_FUNCTION, BFC_LINE, expression, #expression, messageFmt, __VA_ARGS__)
#define BFC_RELFAIL(messageFmt, ...) ::bfc::fail(BFC_FILE, BFC_FUNCTION, BFC_LINE, messageFmt, __VA_ARGS__)

#ifdef BFC_DEBUG
#define BFC_ASSERT(expression, messageFmt, ...) BFC_RELASSERT(expression, messageFmt, __VA_ARGS__)
#else
#define BFC_ASSERT(expression, messageFmt, ...)
#endif

#ifdef BFC_DEBUG
#define BFC_FAIL(messageFmt, ...) BFC_RELFAIL(messageFmt, __VA_ARGS__)
#else
#define BFC_FAIL(messageFmt, ...)
#endif

// Platform/Architecture strings
#ifdef BFC_WINDOWS
#define BFC_PLATFORM_NAME "windows"
#if defined(BFC_X64)
#define BFC_ARCH_NAME "x86_64"
#elif defined(BFC_X86)
#define BFC_ARCH_NAME "x86"
#endif
#endif

#ifdef BFC_LINUX
#define BFC_PLATFORM_NAME "linux"
#if defined(BFC_X64)
#define BFC_ARCH_NAME "x86_64"
#elif defined(BFC_X86)
#define BFC_ARCH_NAME "x86"
#endif
#endif

#ifdef BFC_MAC
#define BFC_PLATFORM_NAME "osx"
#if defined(BFC_X64)
#define BFC_ARCH_NAME "x86_64"
#elif defined(BFC_X86)
#define BFC_ARCH_NAME "x86"
#endif
#endif

#define BFC_DEBUG_NAME "debug"
#define BFC_RELEASE_NAME "release"

#if defined(BFC_RELEASE)
#define BFC_CONFIG_NAME BFC_RELEASE_NAME
#elif defined(BFC_DEBUG)
#define BFC_CONFIG_NAME BFC_DEBUG_NAME
#endif

#define BFC_BIN_PREFIX BFC_CONFIG_NAME "/" BFC_PLATFORM_NAME "/" BFC_ARCH_NAME

namespace bfc {
  template <typename T>
  using Ref = std::shared_ptr<T>;

  template<typename T>
  using WeakRef = std::weak_ptr<T>;

  template <typename T, typename... Args>
  Ref<T> NewRef(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }

  template <typename T>
  struct type_identity {
    using type = T;
  };

  template <typename T>
  using type_identity_t = typename type_identity<T>::type;

  static constexpr int64_t npos = std::numeric_limits<int64_t>::max();

  template<typename T>
  struct enable_bitwise_operators : std::false_type {};

  template<typename T, std::enable_if_t<enable_bitwise_operators<T>::value>* = 0>
  T operator|(T const& lhs, T const& rhs) {
    using U = std::underlying_type_t<T>;
    return T((U)lhs | (U)rhs);
  }

  template <typename T, std::enable_if_t<enable_bitwise_operators<T>::value>* = 0>
  T operator&(T const& lhs, T const& rhs) {
    using U = std::underlying_type_t<T>;
    return T((U)lhs & (U)rhs);
  }

  template <typename T, std::enable_if_t<enable_bitwise_operators<T>::value>* = 0>
  T operator^(T const& lhs, T const& rhs) {
    using U = std::underlying_type_t<T>;
    return T((U)lhs ^ (U)rhs);
  }

  template <typename T, std::enable_if_t<enable_bitwise_operators<T>::value>* = 0>
  T& operator|=(T& lhs, T const& rhs) {
    lhs = lhs | rhs;
    return lhs;
  }

  template <typename T, std::enable_if_t<enable_bitwise_operators<T>::value>* = 0>
  T& operator&=(T& lhs, T const& rhs) {
    lhs = lhs & rhs;
    return lhs;
  }

  template <typename T, std::enable_if_t<enable_bitwise_operators<T>::value>* = 0>
  T& operator^=(T &lhs, T const& rhs) {
    lhs = lhs ^ rhs;
    return lhs;
  }

  template<typename T>
  constexpr uint64_t hash(T const& o) {
    return (uint64_t)std::hash<T>{}(o);
  }

  constexpr uint64_t hashCombine(uint64_t a, uint64_t b) {
    return a ^ (b + 0x9e3779b9 + (a << 6) + (b >> 2));
  }

  template<typename T, typename... Args>
  constexpr uint64_t hash(T const& first, Args const&... rest){
    return hashCombine(hash(first), hash(rest...));
  }

  template<typename T, int64_t N>
  constexpr uint64_t hash(T const (&arr)[N]) {
    uint64_t seed = 0;
    for (T const & o : arr) {
      seed = hashCombine(seed, hash(o));
    }
    return seed;
  }

  template<typename T, int64_t N>
  constexpr uint64_t arraySize(T const (&arr)[N]) {
    BFC_UNUSED(arr);
    return N;
  }

  namespace math {
    template<typename T>
    constexpr auto min(T const & a, T const & b) {
      return a < b ? a : b;
    }

    template<typename T, typename... Args>
    constexpr auto min(T const & a, T const & b, Args const &... next) {
      return a < b ? min(a, next...) : min(b, next...);
    }

    template<typename T>
    constexpr auto max(T const & a, T const & b) {
      return a > b ? a : b;
    }

    template<typename T, typename... Args>
    constexpr auto max(T const & a, T const & b, Args const &... next) {
      return a > b ? max(a, next...) : max(b, next...);
    }
  } // namespace math

  // Some type trait helpers

  template<typename T>
  struct sfinae_true : std::true_type {};

  template<typename T>
  struct sfinae_false : std::false_type {};

  template<typename... Args>
  struct arg_list {
    inline static constexpr int64_t size = sizeof...(Args);
  };

  template<bool IsConst, typename T>
  struct conditional_const {};
  template<typename T>
  struct conditional_const<true, T> : std::add_const<T> {};
  template<typename T>
  struct conditional_const<false, T> : std::remove_const<T> {};

  template<bool IsConst, typename T>
  using conditional_const_t = typename conditional_const<IsConst, T>::type;

  // Test if a type is a function.
  template<typename T>
  struct is_function : std::false_type {};

  template<typename ReturnT, typename... Args>
  struct is_function<ReturnT (*)(Args...)> : std::true_type {};

  template<typename ReturnT, typename... Args>
  struct is_function<ReturnT(Args...)> : std::true_type {};

  template<typename ClassT, typename ReturnT, typename... Args>
  struct is_function<ReturnT (ClassT::*)(Args...)> : std::true_type {};

  template<typename ClassT, typename ReturnT, typename... Args>
  struct is_function<ReturnT (ClassT::*)(Args...) const> : std::true_type {};

  template<typename T>
  inline constexpr bool is_function_v = is_function<T>::value;

  // Get details of a function type.
  template<typename T>
  struct function_type {
    using Ret     = void;
    using Func    = void;
    using ArgList = void;
  };

  template<typename ReturnT, typename... Args>
  struct function_type<ReturnT (*)(Args...)> {
    using Ret     = ReturnT;
    using ArgList = arg_list<Args...>;
    using Func    = ReturnT(Args...);
  };

  template<typename ReturnT, typename... Args>
  struct function_type<ReturnT(Args...)> {
    using Ret     = ReturnT;
    using ArgList = arg_list<Args...>;
    using Func    = ReturnT(Args...);
  };

  template<typename ClassT, typename ReturnT, typename... Args>
  struct function_type<ReturnT (ClassT::*)(Args...)> {
    using Ret     = ReturnT;
    using ArgList = arg_list<Args...>;
    using Func    = ReturnT(Args...);
  };

  template<typename ClassT, typename ReturnT, typename... Args>
  struct function_type<ReturnT (ClassT::*)(Args...) const> {
    using Ret     = ReturnT;
    using ArgList = arg_list<Args...>;
    using Func    = ReturnT(Args...);
  };

  template<typename T>
  struct inner_types {
    inline static constexpr bool valid = false;
  };

  template<template<typename...> typename Outer, typename ...Inner>
  struct inner_types<Outer<Inner...>> {
    inline static constexpr bool valid = true;

    using type = arg_list<Inner...>;
  };

  BFC_API bool assertion(char const * file, char const * function, int64_t line, bool condition, char const * expression, char const * message, ...);
  BFC_API void fail(char const * file, char const * function, int64_t line, char const * message, ...);
}

#define BFC_DEFINE_MEMBER_CHECK(member)                                                                                         \
  template <typename T, typename V = bool>                                                                                      \
  struct has_field_ ## member : std::false_type {};                                                                                   \
  template <typename T>                                                                                                         \
  struct has_field_ ## member <T, typename std::enable_if<!std::is_same<decltype(std::declval<T>().member), void>::value, bool>::type> \
      : std::true_type { typedef decltype(std::declval<T>().member) type; };

#define BFC_HAS_MEMBER(C, member)   has_field_##member<C>::value
#define BFC_OFFSET_OF(Type, member) offsetof(Type, member)
