#include "scripting/WrenContext.h"
#include "core/Stream.h"
#include "core/URI.h"
#include "util/Log.h"
#include "platform/OS.h"

namespace bfc {
  namespace scripting {
    namespace impl {
      void * reallocate(void * memory, size_t newSize, void * userData) {
        if (memory == nullptr)
          return mem::alloc(newSize);
        else if (newSize != 0)
          return mem::realloc(memory, newSize);
        else
          mem::free(memory);

        return nullptr;
      }

      static const char * resolveModule(WrenVM * vm, const char * importer, const char * name) {
        std::optional<URI> uri = ((WrenContext *)wrenGetUserData(vm))->resolveModule(importer, name);
        if (!uri.has_value()) {
          return 0;
        }

        int64_t length = uri->str().length();
        char *  ret    = (char *)reallocate(0, length + 1, 0);

        mem::strcpy(ret, length + 1, uri->c_str());

        return ret;
      }

      static void loadModuleComplete(WrenVM * vm, const char * name, WrenLoadModuleResult result) {
        free(result.userData);
      }

      static WrenLoadModuleResult loadModule(WrenVM * vm, const char * name) {
        std::optional<String> content = ((WrenContext *)wrenGetUserData(vm))->loadModule(name);

        if (!content.has_value()) {
          return {0};
        }

        WrenLoadModuleResult result = {0};
        result.onComplete           = loadModuleComplete;
        result.userData             = mem::strdup(content->c_str());
        result.source               = (const char *)result.userData;
        return result;
      }

      static WrenForeignClassMethods bindForeignClass(WrenVM * vm, const char * module, const char * className) {
        auto pType = ((WrenContext *)wrenGetUserData(vm))->lookupForeignType(module, className);

        return pType == nullptr ? WrenForeignClassMethods{0} : pType->binding;
      }

      static WrenForeignMethodFn bindForeignMethod(WrenVM * vm, const char * module, const char * className, bool isStatic, const char * signature) {
        auto pMethod = ((WrenContext *)wrenGetUserData(vm))->lookupForeignMethod(module, className, isStatic, signature);
        return pMethod == nullptr ? WrenForeignMethodFn{0} : pMethod->callback;
      }

      static void write(WrenVM * vm, const char * text) {
        BFC_LOG_INFO("WrenContext", text);
      }

      static void error(WrenVM * vm, WrenErrorType type, const char * module, int line, const char * message) {
        const char * typeDesc = "Unknown";
        switch (type) {
        case WrenErrorType::WREN_ERROR_COMPILE: typeDesc = "Compile Error"; break;
        case WrenErrorType::WREN_ERROR_RUNTIME: typeDesc = "Runtime Error"; break;
        case WrenErrorType::WREN_ERROR_STACK_TRACE: typeDesc = "Stack Trace"; break;
        }

        BFC_LOG_ERROR("WrenContext", "%s: %s in %s:%d", typeDesc, message, module, line);
      }
    } // namespace impl

    WrenContext::WrenContext() {
      WrenConfiguration config;
      wrenInitConfiguration(&config);
      config.userData            = this;
      config.errorFn             = impl::error;
      config.resolveModuleFn     = impl::resolveModule;
      config.bindForeignClassFn  = impl::bindForeignClass;
      config.bindForeignMethodFn = impl::bindForeignMethod;
      config.loadModuleFn        = impl::loadModule;
      config.reallocateFn        = impl::reallocate;
      config.writeFn             = impl::write;
      m_pVM                      = wrenNewVM(&config);
    }

    WrenContext::~WrenContext() {
      wrenFreeVM(m_pVM);
      m_pVM = nullptr;
    }

    void WrenContext::interpret(URI const & moduleName, String const & code) {
      wrenInterpret(m_pVM, moduleName.c_str(), code.c_str());
    }

    void WrenContext::addForeignModule(URI const & moduleUri, wren::Module const & module) {
      m_modules.addOrSet(moduleUri, module);
    }

    wren::ForeignType const * WrenContext::lookupForeignType(URI const & moduleUri, StringView className) const {
      wren::Module const * pModule = m_modules.tryGet(moduleUri);
      if (pModule == nullptr)
        return nullptr;

      return pModule->types.tryGet(className);
    }

    wren::ForeignMethod const * WrenContext::lookupForeignMethod(URI const & moduleUri, StringView className, bool isStatic, StringView signature) const {
      wren::ForeignType const * pType = lookupForeignType(moduleUri, className);
      if (pType == nullptr)
        return nullptr;

      if (isStatic)
        return nullptr; // pType->staticMethods.tryGet(signature);

      return pType->memberMethods.tryGet(signature);
    }

    wren::Value WrenContext::getVariable(String const & variableName, std::optional<URI> const & uri) {
      char const * moduleName = uri.has_value() ? uri->c_str() : nullptr;

      if (!wrenHasModule(m_pVM, moduleName) || !wrenHasVariable(m_pVM, moduleName, variableName.c_str()))
        return {};

      wrenEnsureSlots(m_pVM, 1);
      wrenGetVariable(m_pVM, moduleName, variableName.c_str(), 0);

      return wren::Value(m_pVM, 0);
    }

    wren::Value WrenContext::getMethod(String const & methodName) {
      return { m_pVM, wrenMakeCallHandle(m_pVM, methodName.c_str()) };
    }

    std::optional<URI> WrenContext::resolveModule(URI const & importer, String const & name) {
      URI importUri = name + ".wren";
      URI cwd       = URI::File(os::getCwd()).resolveRelativeReference(importUri, !importUri.scheme().empty());
      if (uriExists(cwd)) {
        return cwd;
      }

      if (uriExists(importUri)) {
        return importUri;
      }

      URI relative = importer.withPath(importer.path().parent()).resolveRelativeReference(importUri, !importUri.scheme().empty());
      if (uriExists(relative)) {
        return relative;
      }

      return std::nullopt;
    }

    std::optional<String> WrenContext::loadModule(URI const & uri) {
      String content;
      if (!readTextURI(uri, &content))
        return std::nullopt;
      return content;
    }
  }
} // namespace bfc
