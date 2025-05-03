#pragma once

#include "../../../vendor/wren/src/include/wren.hpp"

#include "../core/URI.h"
#include "../core/Map.h"
#include "wren/Module.h"
#include "wren/Value.h"

#include <optional>
#include <functional>

namespace bfc {
  namespace scripting {
    namespace wren {
      class Value;
    }
    class WrenContext {
    public:
      // Gives some types access to the interal VM without exposing it to users of WrenContext
      class VMAccess {
      private:
        friend wren::Value;
        inline static WrenVM * get(WrenContext const * pContext) {
          return pContext->m_pVM;
        }
      };

      WrenContext();
      ~WrenContext();

      void interpret(URI const & moduleUri, String const & code);
      void addForeignModule(URI const & moduleUri, wren::Module const & module);

      wren::ForeignType const *   lookupForeignType(URI const & moduleUri, StringView className) const;
      wren::ForeignMethod const * lookupForeignMethod(URI const & moduleUri, StringView className, bool isStatic, StringView signature) const;

      wren::Value getVariable(String const & variableName, std::optional<URI> const & moduleUri = std::nullopt);
      wren::Value getMethod(String const & signature);

      virtual std::optional<URI>    resolveModule(URI const & importer, String const & name);
      virtual std::optional<String> loadModule(URI const & uri);

    private:
      Map<URI, wren::Module> m_modules;
      WrenVM *               m_pVM = nullptr;
    };
  }
}
