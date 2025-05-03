#pragma once

#include "../../../vendor/wren/src/include/wren.hpp"

#include "ForeignType.h"

#include <functional>
#include <optional>

namespace bfc {
  namespace scripting {
    namespace wren {
      struct Module {
        template<typename T>
        ForeignType & exportType(StringView const & name) {
          types.addOrSet(name, Type::fromReflection(reflect<T>()));
        }

        ForeignType & add(StringView const & name) {
          return types.addOrSet(name, {});
        }

        Map<String, ForeignType> types;
      };
    } // namespace wren
  } // namespace scripting
} // namespace bfc
