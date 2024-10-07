#pragma once

#include "Core.h"
#include "Reflection.h"
#include "templateindex.h"
#include "typeindex.h"

namespace bfc {
  namespace detail {
    class TypeReflection;
  }
  using Type = detail::TypeReflection const *;

  class BFC_API RuntimeObject {
    /// Move construct from a T
    RuntimeObject(void * pInstance, Type reflection, bool isOwner);
  public:
    RuntimeObject() = default;

    /// Copy construct from a T
    template<typename T>
    static RuntimeObject bind(T const & instance) {
      return copy(&instance, runtimeReflect<T>());
    }

    template<typename T>
    static RuntimeObject bind(T && instance) {
      return move(&instance, runtimeReflect<T>());
    }

    template<typename T>
    static RuntimeObject bind(T * pInstance) {
      return bind(pInstance, runtimeReflect<T>());
    }

    template<typename T>
    static RuntimeObject bind(T &instance) {
      return bind(&instance);
    }

    /// Construct from opaque pointer and type information
    static RuntimeObject move(void * pInstance, Type reflection);
    static RuntimeObject copy(void const * pInstance, Type reflection);
    static RuntimeObject bind(void * pInstance, Type reflection);
    static RuntimeObject construct(Type reflection, Vector<RuntimeObject> const & argList = {});

    Vector<String> const & members();
    Vector<String> const & methods();

    // Set the value of this object.
    // returns false if there is no copy-assign operator.
    bool assign(RuntimeObject const & value);
    bool assign(RuntimeObject &&value);

    RuntimeObject call(StringView const & name, Vector<RuntimeObject> const & args = {});
    RuntimeObject get(StringView const & name);

    bfc::type_index     typeInfo() const;
    bfc::template_index templateInfo() const;

    Type reflection() const;

    void *       data();
    void const * data() const;

    bool isEmpty() const;

    template<typename T>
    bool is() const {
      return m_pData != nullptr && m_pData->type != nullptr && m_pData->type->is<T>();
    }

    template<typename T>
    T& get() {
      BFC_ASSERT(is<T>(), "RuntimeObject is not of type T");
      return *(T*)data();
    }

    template<typename T>
    T * tryGet() {
      return is<T>() ? (T*)data() : nullptr;
    }

    template<typename T>
    T const & get() const {
      BFC_ASSERT(is<T>(), "RuntimeObject is not of type T");
      return *(T const *)data();
    }

    template<typename T>
    T const * tryGet() const {
      return is<T>() ? (T const * )data() : nullptr;
    }

  private:
    struct ControlBlock {
      ControlBlock(ControlBlock const & o) = delete;
      ControlBlock(void * pInstance, Type reflection, bool isOwner);
      ControlBlock(ControlBlock && o);
      ~ControlBlock();

      Type   type      = nullptr; ///< Reflection data for the instance.
      void * pInstance = nullptr; ///< A pointer to the object instance.
      bool   isOwner   = false;   ///< Does this RuntimeObject own the instance it points to
    };

    std::shared_ptr<ControlBlock> m_pData = nullptr;
  };
} // namespace bfc
