#pragma once

#include "../../../vendor/wren/src/include/wren.hpp"
#include "../../core/String.h"

#include <functional>
#include <optional>

namespace bfc {
  namespace scripting {
    namespace wren {
      template<typename T>
      struct Slot {
        static auto get(WrenVM * pVM, int32_t idx) {
          if constexpr (std::is_same_v<T, bool>) {
            std::optional<bool> ret;
            if (wrenGetSlotType(pVM, idx) == WREN_TYPE_BOOL)
              ret = wrenGetSlotBool(pVM, idx);
            return ret;
          } else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
            std::optional<T> ret;
            if (wrenGetSlotType(pVM, idx) == WREN_TYPE_NUM)
              ret = (T)wrenGetSlotDouble(pVM, idx);
            return ret;
          } else if constexpr (std::is_same_v<T, StringView> || std::is_same_v<T, String> || std::is_same_v<T, std::string> ||
                               std::is_same_v<T, char const *>) {
            std::optional<T> ret;
            if (wrenGetSlotType(pVM, idx) == WREN_TYPE_STRING)
              ret = (T)wrenGetSlotString(pVM, idx);
            return ret;
          } else {
            std::optional<std::reference_wrapper<T>> ret;
            if (wrenGetSlotType(pVM, idx) == WREN_TYPE_FOREIGN)
              ret = *(T *)wrenGetSlotForeign(pVM, idx);
            return ret;
          }
        }

        static void set(WrenVM * pVM, int32_t idx, T const & value) {
          if constexpr (std::is_same_v<T, bool>) {
            wrenSetSlotBool(pVM, idx, value);
          } else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
            wrenSetSlotDouble(pVM, idx, (double)value);
          } else if constexpr (std::is_same_v<T, char const *> || std::is_same_v<T, char *>) {
            wrenSetSlotString(pVM, idx, value);
          } else if constexpr (std::is_same_v<T, String> || std::is_same_v<T, std::string>) {
            wrenSetSlotString(pVM, idx, value.c_str());
          } else if constexpr (std::is_same_v<T, StringView>) {
            wrenSetSlotString(pVM, idx, String(value).c_str());
          } else {
            static_assert(false, "Not implemented");
          }
        }
      };

      /// Specialization to get/set a nullptr
      template<>
      struct Slot<std::nullptr_t> {
        static std::optional<std::nullptr_t> get(WrenVM * pVM, int32_t idx) {
          if (wrenGetSlotType(pVM, idx) == WREN_TYPE_NULL)
            return nullptr;
          else
            return std::nullopt;
        }

        static void set(WrenVM * pVM, int32_t idx, std::nullptr_t const & value) {
          wrenSetSlotNull(pVM, idx);
        }
      };

      /// Specialization to get/set a shared_ptr
      // template<typename T>
      // struct Slot<std::shared_ptr<T>> {
      //   static std::optional<std::shared_ptr<T>> get(WrenVM * pVM, int32_t idx) {
      //     return Slot<T>::get(pVM, idx);
      //     return Value(pVM, idx);
      //   }
      // 
      //   static void set(WrenVM * pVM, int32_t idx, std::shared_ptr<T> const & value) {
      //     if (value == nullptr)
      //       wrenSetSlotNull(pVM, idx);
      //     else
      //       Slot<T>::set(pVM, idx, *value);
      //   }
      // };

      template<typename T>
      struct SlotTraits {
        using return_t           = decltype(Slot<T>::get(0, 0));
        using return_unwrapped_t = std::decay_t<decltype(Slot<T>::get(0, 0).value())>;
      };

      template<typename T>
      struct UnpackForSlot {
        inline static T const & unpack(T const & o) {
          return o;
        }

        inline static auto unpack(T &&o) {
          return std::forward<T>(o);
        }
      };
    } // namespace wren
  } // namespace scripting
} // namespace bfc
