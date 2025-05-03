#pragma once

#include "../../../vendor/wren/src/include/wren.hpp"

#include "../../core/Reflect.h"

#include "Slot.h"

#include <functional>
#include <optional>

namespace bfc {
  namespace scripting {
    namespace wren {
      struct ForeignMethod {
        template<typename T, typename... Args>
        static ForeignMethod constructor(ReflectConstructor<T, Args...> const &) {
          ForeignMethod ret;
          ret.callback = [](WrenVM * pVM) {
            using interface_t = ReflectConstructor<T, Args...>;
            using class_t     = typename interface_t::class_type;
            tryInvoke(pVM, interface_t::call);
          };
          return ret;
        }

        template<typename Member, Member Address>
        static ForeignMethod function(ReflectMember<Member, Address> const &) {
          ForeignMethod ret;
          ret.callback = [](WrenVM * pVM) {
            using interface_t = ReflectMember<Member, Address>;
            using class_t     = typename interface_t::class_type;

            tryInvoke(pVM, interface_t::call);
          };
          return ret;
        }

        template<typename Member, Member Address>
        static ForeignMethod getter(ReflectMember<Member, Address> const &) {
          ForeignMethod ret;
          ret.callback = [](WrenVM * pVM) {
            using interface_t = ReflectMember<Member, Address>;
            using class_t     = typename interface_t::class_type;

            tryInvoke(pVM, [](class_t & instance) { return interface_t::get(instance); });
          };
          return ret;
        }

        template<typename Member, Member Address>
        static ForeignMethod setter(ReflectMember<Member, Address> const &) {
          ForeignMethod ret;
          ret.callback = [](WrenVM * pVM) {
            using interface_t = ReflectMember<Member, Address>;
            using class_t     = typename interface_t::class_type;
            using value_t     = typename interface_t::value_type;

            tryInvoke(pVM, [](class_t & instance, value_t const & value) { return interface_t::set(instance, value); });
          };
          return ret;
        }

        WrenForeignMethodFn callback;

      private:
        template<typename Func>
        static bool tryInvoke(WrenVM * pVM, Func f) {
          return tryInvoke(pVM, std::forward<Func>(f), function_type<Func>::ArgList{});
        }

        template<typename Func, typename... Args>
        static bool tryInvoke(WrenVM * pVM, Func f, arg_list<Args...>) {
          return tryInvoke(pVM, std::forward<Func>(f), arg_list<Args...>{}, std::index_sequence_for<Args...>{});
        }

        template<typename Func, typename... Args, size_t... Indices>
        static bool tryInvoke(WrenVM * pVM, Func f, arg_list<Args...>, std::index_sequence<Indices...>) {
          return tryInvoke(pVM, std::forward<Func>(f), Slot<std::decay_t<Args>>::get(pVM, Indices)...);
        }

        template<typename Func, typename... Args>
        static bool tryInvoke(WrenVM * pVM, Func f, std::optional<Args> const &... args) {
          using return_t = typename function_type<Func>::Ret;

          if ((!args.has_value() || ...)) {
            return false; // Fail if any of the arguments were missing
          }

          if constexpr (!std::is_same_v<return_t, void>) {
            Slot<return_t>::set(pVM, 0, f(args.value()...));
          } else {
            f(args.value()...);
          }

          return true;
        }
      };
    } // namespace wren
  } // namespace scripting
} // namespace bfc
