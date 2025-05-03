#pragma once

#include "Map.h"
#include "Pair.h"
#include "String.h"
#include "templateindex.h"
#include "typeindex.h"

#include <stdint.h>
#include <tuple>
#include <optional>

namespace bfc {
  enum ReflAttributeType { ReflAttributeType_Unknown, ReflAttributeType_Ctor, ReflAttributeType_Method, ReflAttributeType_Member, ReflAttributeType_Count };

  namespace detail {
    template<typename T, typename... Args>
    struct ReflCtor {};

    template<typename Type, Type Value>
    struct ReflMember {};

    template<typename Type>
    struct ReflAttribute {
      inline static constexpr ReflAttributeType attributeType = ReflAttributeType_Unknown;
    };

    template<typename Type, Type Address, typename ArgList>
    struct ReflMethodAttributeImpl {};

    template<typename Type, Type Address, typename...Args>
    struct ReflMethodAttributeImpl<Type, Address, arg_list<Args...>> : member_type<Type> {
      using class_type                                        = typename member_type<Type>::container;
      using type                                              = Type;
      inline static constexpr ReflAttributeType attributeType = ReflAttributeType_Method;

      inline static constexpr size_t arg_list_size = member_type<Type>::arg_list_size;

      static auto call(typename member_type<Type>::container & instance, Args... args) {
        return (instance.*Address)(std::forward<Args>(args)...);
      }
    };

    template<typename Type, Type Address>
    struct ReflMethodAttribute : ReflMethodAttributeImpl<Type, Address, typename member_type<Type>::arg_list> {};

    template<typename Type, Type Address>
    struct ReflMemberAttribute : member_type<Type> {
      using class_type                                        = typename member_type<Type>::container;
      using value_type                                        = typename member_type<Type>::type;
      inline static constexpr ReflAttributeType attributeType = ReflAttributeType_Member;

      inline static constexpr size_t arg_list_size = 1;

      static value_type const & get(class_type const & instance) {
        return instance.*Address;
      }

      static value_type & get(class_type & instance) {
        return instance.*Address;
      }

      template<typename U = Type, std::enable_if_t<!std::is_const_v<typename member_type<U>::type>> * = 0>
      static auto & set(class_type & instance, value_type const & value) {
        return instance.*Address = value;
      }
    };

    template<typename Type, Type Address>
    struct ReflAttribute<ReflMember<Type, Address>>
      : std::conditional_t<member_type<Type>::isMethod, ReflMethodAttribute<Type, Address>, ReflMemberAttribute<Type, Address>> {
    };

    template<typename T, typename... Args>
    struct ReflAttribute<ReflCtor<T, Args...>> {
      inline static constexpr ReflAttributeType attributeType = ReflAttributeType_Ctor;

      using class_type = T;
      using type       = ReflCtor<T, Args...>;

      inline static constexpr size_t arg_list_size = sizeof...(Args);

      static void call(T & instance, Args... args) {
        mem::construct(&instance, std::forward<Args>(args)...);
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

  template<typename Method, Method Address>
  using ReflectMember = detail::ReflAttribute<detail::ReflMember<Method, Address>>;

  template<typename T, typename... Args>
  using ReflectConstructor = detail::ReflAttribute<detail::ReflCtor<T, Args...>>;

  template<typename ClassT, typename... Members>
  class Reflection {
    using DataType = std::tuple<Pair<detail::ReflAttribute<Members>, char const *>...>;

  public:
    using Type = std::decay_t<ClassT>;

    template<int64_t N>
    using InterfaceOf = decltype(std::get<N>(std::declval<DataType>()).first);
    template<int64_t N>
    using TypeOf   = std::decay_t<typename InterfaceOf<N>::type>;
    using IndicesT = std::make_integer_sequence<int64_t, sizeof...(Members)>;

    template<int64_t N>
    inline static constexpr auto attributeType = InterfaceOf<N>::attributeType;
    template<int64_t N>
    inline static constexpr bool isMethod = attributeType<N> == ReflAttributeType_Method;
    template<int64_t N>
    inline static constexpr bool isMember = attributeType<N> == ReflAttributeType_Member;
    template<int64_t N>
    inline static constexpr bool isConstructor         = attributeType<N> == ReflAttributeType_Ctor;
    inline static constexpr bool hasCopyConstructor    = std::is_copy_constructible_v<Type>;
    inline static constexpr bool hasMoveConstructor    = std::is_move_constructible_v<Type>;
    inline static constexpr bool hasDefaultConstructor = std::is_default_constructible_v<Type>;
    inline static constexpr bool hasCopyAssign         = std::is_copy_assignable_v<Type>;
    inline static constexpr bool hasMoveAssign         = std::is_move_assignable_v<Type>;
    inline static constexpr bool hasInnerTypes         = inner_types<ClassT>::valid;
    inline static constexpr auto innerTypes            = detail::reflectInnerTypes<ClassT>();

    inline constexpr Reflection(Pair<detail::ReflAttribute<Members>, char const *> const &... args)
      : m_attributes(args...)
    {}

    static inline constexpr int64_t size() {
      return sizeof...(Members);
    }

    template<int64_t N>
    inline constexpr auto getAttribute() const {
      return std::get<N>(m_attributes);
    }

    template<int64_t N>
    inline constexpr auto const & get(Type const * pSelf) const {
      static_assert(isMember<N>, "Member at `N` is not a variable type. Use `call` instead?.");

      return InterfaceOf<N>::get(*pSelf);
    }

    template<int64_t N>
    inline constexpr auto & get(Type * pSelf) const {
      static_assert(isMember<N>, "Member at `N` is not a variable type. Use `call` instead?.");

      return InterfaceOf<N>::get(*pSelf);
    }

    template<int64_t N, typename... Args>
    inline constexpr auto call(Type * pSelf, Args... args) const {
      static_assert(isMethod<N>, "Member at `N` is not a variable type. Use `get` instead.");

      return InterfaceOf<N>::call(*pSelf, std::forward<Args>(args)...);
    }

    template<int64_t N, typename... Args>
    inline constexpr void construct(Type * pSelf, Args... args) const {
      static_assert(isConstructor<N>, "Member at `N` is not a constructor type. Use `get` or `call` instead.");

      InterfaceOf<N>::call(*pSelf, args...);
    }

    template<int64_t N>
    inline constexpr char const * name() const {
      return std::get<N>(m_attributes).second;
    }

    template<int64_t N, typename T>
    inline static constexpr bool is() {
      return std::is_same_v<TypeOf<N>, T(Type::*)>;
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

  private:
    template<typename Visitor, int64_t... Indices>
    inline void _visit(Type * pSelf, Visitor visitor, std::integer_sequence<int64_t, Indices...>) const {
      (_tryVisit<Indices>(pSelf, visitor), ...);
    }

    template<int64_t Index, typename Visitor>
    inline void _tryVisit(Type * pSelf, Visitor visitor) const {
      visitor(name<Index>(), get<Index>(pSelf));
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
  inline constexpr auto makeReflection(Pair<detail::ReflAttribute<Members>, char const *> const &... args) {
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

#define BFC_REFLECT(MyType, a)                          ::bfc::makePair(::bfc::ReflectMember<decltype(&MyType::a), &MyType::a>{}, #a)
#define BFC_REFLECT_OVERLOAD(MyType, a, Return, Params) ::bfc::makePair(::bfc::ReflectMember<Return(MyType::*) Params, &MyType::a>{}, #a)
#define BFC_REFLECT_CTOR(MyType, ...)                   ::bfc::makePair(::bfc::ReflectConstructor<MyType, __VA_ARGS__>{}, "MyType ctor")
