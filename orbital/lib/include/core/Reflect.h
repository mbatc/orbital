#pragma once

#include "Map.h"
#include "Pair.h"
#include "String.h"
#include "templateindex.h"
#include "typeindex.h"

#include <stdint.h>
#include <tuple>
#include <optional>

#define BFC_REFLECT(MyType, a)        ::bfc::makePair(&MyType::a, #a)
#define BFC_REFLECT_OVERLOAD(MyType, a, Return, Params) ::bfc::makePair(static_cast<Return(MyType::*) Params>(&MyType::a), #a)
#define BFC_REFLECT_CTOR(MyType, ...) ::bfc::makePair(bfc::detail::CtorReflection<__VA_ARGS__>{}, "MyType ctor");

namespace bfc {
  namespace detail {
    template<typename T, typename... Args>
    struct CtorReflection {
      using ArgList = arg_list<Args...>;

      void operator()(T * pInstance, Args... args) const {
        mem::construct(pInstance, args...);
      }
    };

    template<typename Type>
    static inline constexpr auto getReflectionOrDefault() {
      if constexpr (has_reflect_v<Type>) {
        return reflect<Type>();
      } else {
        return Reflection<Type>{};
      }
    }

    template<typename... Types>
    static inline constexpr auto reflectInnerTypesImpl(arg_list<Types...>) {
      return std::make_tuple(getReflectionOrDefault<Types>()...);
    }

    template<typename ClassT>
    typename static inline constexpr auto deduceAndReflectInnerTypes() {
      using InnerArgList = typename inner_types<ClassT>::type;
      return reflectInnerTypesImpl(InnerArgList{});
    }

    template<typename ClassT>
    static inline constexpr auto reflectInnerTypes() {
      if constexpr (inner_types<ClassT>::valid) {
        return deduceAndReflectInnerTypes<ClassT>();
      } else {
        return std::tuple<>{};
      }
    }
  } // namespace detail

  template<typename T>
  struct is_ctorreflection : std::false_type {};

  template<typename... Args>
  struct is_ctorreflection<detail::CtorReflection<Args...>> : std::true_type {};

  template<typename T>
  inline static constexpr bool is_ctorreflection_v = is_ctorreflection<T>::value;

  template<typename ClassT, typename... Members>
  class Reflection {
    using DataType = std::tuple<Pair<Members, char const *>...>;

  public:
    using Type = std::decay_t<ClassT>;
    template<int64_t N>
    using TypeOf   = std::decay_t<decltype(std::get<N>(std::declval<DataType>()).first)>;
    using IndicesT = std::make_integer_sequence<int64_t, sizeof...(Members)>;

    inline static constexpr bool hasInnerTypes = inner_types<ClassT>::valid;
    inline static constexpr auto innerTypes    = detail::reflectInnerTypes<ClassT>();

    template<typename... Members>
    inline constexpr Reflection(Pair<Members, char const *> const &... args)
      : m_attributes(args...)
    {}

    static inline constexpr int64_t size() {
      return sizeof...(Members);
    }

    template<int64_t N>
    inline constexpr auto getAttribute() const {
      return std::get<N>(m_attributes).first;
    }

    template<int64_t N>
    inline constexpr auto const & get(Type const * pSelf) const {
      if constexpr (isMember<N>()) {
        auto member = std::get<N>(m_attributes).first;
        return pSelf->*member;
      } else {
        static_assert(false, "Member at `N` is not a variable type. Use `call` instead?.");
      }
    }

    template<int64_t N>
    inline constexpr auto & get(Type * pSelf) const {
      if constexpr (isMember<N>()) {
        auto member = std::get<N>(m_attributes).first;
        return pSelf->*member;
      } else {
        static_assert(false, "Member at `N` is not a variable type. Use `call` instead?.");
      }
    }

    template<int64_t N, typename... Args>
    inline constexpr auto call(Type * pSelf, Args... args) const {
      if constexpr (isMethod<N>()) {
        auto func = std::get<N>(m_attributes).first;
        return (pSelf->*func)(args...);
      } else {
        static_assert(false, "Member at `N` is not a variable type. Use `get` instead.");
      }
    }

    template<int64_t N, typename... Args>
    inline constexpr void construct(Type * pSelf, Args... args) const {
      if constexpr (isConstructor<N>()) {
        auto ctor = std::get<N>(m_attributes).first;
        ctor(pSelf, args...);
      } else {
        static_assert(false, "Member at `N` is not a constructor type. Use `get` or `call` instead.");
      }
    }

    template<int64_t N>
    inline constexpr char const * name() const {
      return std::get<N>(m_attributes).second;
    }

    template<int64_t N, typename T>
    inline static constexpr bool is() {
      return std::is_same<TypeOf<N>, T(Type::*)>::value;
    }

    template<typename T>
    inline void forEach(Type * pSelf, std::function<void(char const *, T &)> const & callback) const {
      _forEach<T>(pSelf, callback, IndicesT{});
    }

    template<typename Visitor>
    inline void visit(Type * pSelf, Visitor visitor) const {
      _visit(pSelf, visitor, IndicesT{});
    }

    inline static bfc::type_index typeInfo() {
      return TypeID<Type>();
    }

    inline static bfc::template_index templateInfo() {
      return templateid<Type>();
    }

    static inline constexpr int64_t sizeOf() {
      return sizeof(Type);
    }

    template<int64_t N>
    static inline constexpr bool isMember() {
      return !isMethod<N>() && !isConstructor<N>();
    }

    template<int64_t N>
    static inline constexpr bool isMethod() {
      return bfc::is_function_v<TypeOf<N>>;
    }

    template<int64_t N>
    static inline constexpr bool isConstructor() {
      return bfc::is_ctorreflection_v<TypeOf<N>>;
    }

    static inline constexpr bool hasCopyConstructor() {
      return std::is_copy_constructible_v<Type>;
    }

    static inline constexpr bool hasMoveConstructor() {
      return std::is_move_constructible_v<Type>;
    }

    static inline constexpr bool hasDefaultConstructor() {
      return std::is_default_constructible_v<Type>;
    }

    static inline constexpr bool hasCopyAssign() {
      return std::is_copy_assignable_v<Type>;
    }

    static inline constexpr bool hasMoveAssign() {
      return std::is_move_assignable_v<Type>;
    }

  private:
    template<typename Visitor, int64_t... Indices>
    inline void _visit(Type * pSelf, Visitor visitor, std::integer_sequence<int64_t, Indices...>) const {
      (_tryVisit<Indices>(pSelf, visitor), ...);
    }

    template<int64_t Index>
    inline void _tryVisit(Type * pSelf, ...) const {
      BFC_UNUSED(pSelf);
    }

    template<typename T, int64_t... Indices>
    inline void _forEach(Type * pSelf, std::function<void(char const *, T &)> const & callback, std::integer_sequence<int64_t, Indices...>) const {
      (_tryForwardMember<Indices, T>(pSelf, callback), ...);
    }

    template<int64_t Index, typename T>
    inline void _tryForwardMember(Type * pSelf, std::function<void(char const *, T &)> const & callback) const {
      if constexpr (is<Index, T>()) {
        callback(name<Index>(), get<Index>(pSelf));
      } else {
        BFC_UNUSED(callback);
      }
    }

    DataType m_attributes;

  };

  template<typename T, typename... Members>
  inline constexpr auto makeReflection(Pair<Members, char const *> const &... args) {
    return Reflection<T, Members...>(args...);
  }

  template<typename T>
  struct Reflect {
    static inline constexpr auto get() {}
  };

  template<typename T>
  inline constexpr auto reflect() {
    return Reflect<T>::get();
  }

  template<typename T>
  inline constexpr bool has_reflect_v = !std::is_same<void, decltype(::bfc::reflect<T>())>::value;
} // namespace bfc
