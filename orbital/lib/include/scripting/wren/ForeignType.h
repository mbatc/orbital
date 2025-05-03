#pragma once

#include "../../../vendor/wren/src/include/wren.hpp"

#include "ForeignMethod.h"

#include <functional>
#include <optional>

namespace bfc {
  namespace scripting {
    namespace wren {
      class ForeignType {
        template<int64_t N>
        static String getParameterList() {
          if constexpr (N == 0) {
            return "";
          } else {
            return String::join(Array<String, N>("_"), ",");
          }
        }

      public:
        ForeignType() = default;

        template<typename T>
        ForeignType & bind() {
          return bind(reflect<T>());
        }

        template<typename T>
        void allocate() {
          binding.allocate = [](WrenVM * pVM) { T * pInstance = (T *)wrenSetSlotNewForeign(pVM, 0, 0, sizeof(T)); };
          binding.finalize = [](void * data) { mem::destruct<T>((T *)data); };
        }

        template<typename T, typename... Members>
        ForeignType & bind(Reflection<T, Members...> const & staticReflection) {
          return bind(staticReflection, std::make_integer_sequence<int64_t, sizeof...(Members)>{});
        }

        template<typename T, typename... Members, int64_t... Indices>
        ForeignType & bind(Reflection<T, Members...> const & staticReflection, std::integer_sequence<int64_t, Indices...>) {
          using ReflectionT = Reflection<T, Members...>;

          allocate<T>();

          (bind(staticReflection.getAttribute<Indices>()), ...);
          typeInfo     = staticReflection.typeInfo();
          templateInfo = staticReflection.templateInfo();

          return *this;
        }

        template<typename Member, Member Address>
        ForeignType & bind(bfc::Pair<ReflectMember<Member, Address>, char const *> const & member) {
          return bind(member.first, member.second);
        }

        template<typename Member, Member Address>
        ForeignType & bind(ReflectMember<Member, Address> member, char const * name) {
          using interface_t = ReflectMember<Member, Address>;
          if constexpr (interface_t::attributeType == ReflAttributeType_Method) {
            String signature = String::format("%s(%s)", name, getParameterList<interface_t::arg_list_size>());
            memberMethods.add(signature, ForeignMethod::function(member));
          } else if constexpr (interface_t::attributeType == ReflAttributeType_Member) {
            // Add getter
            memberMethods.add(String::format("%s", name), ForeignMethod::getter(member));
            memberMethods.add(String::format("%s=(_)", name), ForeignMethod::setter(member));
          } else {
            static_assert(false, "member type is not supported");
          }

          return *this;
        }

        template<typename T, typename... Args>
        ForeignType & bind(bfc::Pair<ReflectConstructor<T, Args...>, char const *> const & ctor) {
          return bind(ctor.first);
        }

        template<typename T, typename... Args>
        ForeignType & bind(ReflectConstructor<T, Args...> const & ctor) {
          memberMethods.add(String::format("init new(%s)", getParameterList<sizeof...(Args)>()), ForeignMethod::constructor(ctor));
          return *this;
        }

        WrenForeignClassMethods binding;
        Map<String, ForeignMethod>     memberMethods;

        type_index     typeInfo     = TypeID<void>();
        template_index templateInfo = templateid<void>();
      };

      namespace detail {
        template<typename T>
        class ForeignTypeBuilder {
        public:
          ForeignTypeBuilder() {
            type.allocate<T>();
          }

          ForeignTypeBuilder& reflect() {
            type.reflect<T>();
            return *this;
          }

          template<typename... Args>
          ForeignTypeBuilder & construct() {
            type.bind(ReflectConstructor<T, Args...>{});
            return *this;
          }

          template<typename Member, Member Address>
          ForeignTypeBuilder & member(bfc::Pair<ReflectMember<Member, Address>, char const *> const & member) {
            static_assert(std::is_same_v<typename member_type<Member>::container, T>, "Type mismatch");
            type.bind(member);
            return *this;
          }

          operator ForeignType const & () {
            return type;
          }

          operator ForeignType () && {
            return std::move(type);
          }

          ForeignType type;
        };
      }

      template<typename T>
      detail::ForeignTypeBuilder<T> newForeignType() {
        return detail::ForeignTypeBuilder<T>{};
      }
    } // namespace wren
  } // namespace scripting
} // namespace bfc
