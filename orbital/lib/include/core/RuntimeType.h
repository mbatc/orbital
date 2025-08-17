#pragma once

#include "Core.h"
#include "Reflect.h"
#include "RuntimeObject.h"
#include <optional>

namespace bfc {
  namespace detail {
    class TypeReflection;
  }
  using Type = detail::TypeReflection const *;

  class BFC_API MethodReflection {
  public:
    MethodReflection() = default;

    template<typename Class, typename ReturnT, typename... Args>
    inline MethodReflection(ReturnT (Class::*pFunc)(Args...)) {
      std::function<ReturnT(void *, Args...)> caller = [pFunc](void * pInstance, Args... args) -> ReturnT {
        return (((Class *)pInstance)->*pFunc)(args...);
      };

      m_storage         = [caller]() mutable { return &caller; };
      m_funcType        = TypeID<ReturnT(Args...)>();
      m_argTypes        = {TypeID<Args>()...};
      m_returnType      = {TypeID<ReturnT>()};
      m_unpackAndInvoke = getUnpackAndInvokeImpl<Class, ReturnT, Args...>(std::make_integer_sequence<int64_t, sizeof...(Args)>{});
    }

    template<typename Class, typename ReturnT, typename... Args>
    inline MethodReflection(ReturnT (Class::*pFunc)(Args...) const) {
      std::function<ReturnT(void *, Args...)> caller = [pFunc](void * pInstance, Args... args) -> ReturnT {
        return (((Class *)pInstance)->*pFunc)(args...);
      };

      m_storage         = [caller]() mutable { return &caller; };
      m_funcType        = TypeID<ReturnT(Args...)>();
      m_argTypes        = {TypeID<Args>()...};
      m_returnType      = {TypeID<ReturnT>()};
      m_unpackAndInvoke = getUnpackAndInvokeImpl<Class, ReturnT, Args...>(std::make_integer_sequence<int64_t, sizeof...(Args)>{});
    }

    template<typename FunctorT>
    inline bool is() const {
      return m_funcType == TypeID<FunctorT>();
    }

    RuntimeObject invoke(void * pInstance, Vector<RuntimeObject> const & args) const {
      return m_unpackAndInvoke(this, pInstance, args.getView());
    }

    bfc::type_index const & returnTypeInfo() const;

    bfc::type_index paramTypeInfo(int64_t const i) const;

    int64_t paramCount() const;

    bfc::type_index const & typeInfo() const;

  private:
    /// Call the function without validating argument types.
    template<typename ReturnT, typename... Args>
    inline ReturnT invokeUnchecked(void * pInstance, Args... args) const {
      return (*(std::function<ReturnT(void *, Args...)> *)m_storage())(pInstance, args...);
    }

    template<typename Class, typename ReturnT, typename... Args, int64_t... Indices>
    std::function<RuntimeObject(MethodReflection const *, void *, Span<RuntimeObject>)>
    getUnpackAndInvokeImpl(std::integer_sequence<int64_t, Indices...>) const {
      return [](MethodReflection const * pSelf, void * pInstance, Span<RuntimeObject> argList) {
        // Check arg count is correct
        if (argList.size() != sizeof...(Args))
          return RuntimeObject();

        // Check types are correct
        if (((argList[Indices].typeInfo() != TypeID<Args>()) || ...))
          return RuntimeObject();

        // Unpack arguments and invoke method
        if constexpr (std::is_void_v<ReturnT>) {
          pSelf->invokeUnchecked<ReturnT>(pInstance, argList[Indices].get<std::remove_reference_t<Args>>()...);
          return RuntimeObject();
        } else {
          return RuntimeObject::bind(pSelf->invokeUnchecked<ReturnT>(pInstance, argList[Indices].get<std::remove_reference_t<Args>>()...));
        }
      };
    }

    std::function<void *()> m_storage; /// Returns an opaque pointer to the function wrapper implementation
    bfc::type_index         m_funcType = TypeID<void>();

    bfc::type_index         m_returnType = TypeID<void>();
    Vector<bfc::type_index> m_argTypes;

    // Wrapper that unpacks the list of RuntimeObject's into an argument list and invokes the method
    std::function<RuntimeObject(MethodReflection const *, void *, Span<RuntimeObject>)> m_unpackAndInvoke;
  };

  class BFC_API MemberReflection {
  public:
    MemberReflection() = default;

    template<typename Class, typename MemberT>
    inline MemberReflection(MemberT Class::*pMember) {
      m_offset = (int64_t)&(((Class *)0)->*pMember);
      m_reflection = runtimeReflect<std::decay_t<MemberT>>();
    }

    bool is(bfc::type_index const & type) const;
    void * getPtr(void * pInstance) const;
    Type reflection() const;

    template<typename T>
    inline bool is() const {
      return is(TypeID<T>());
    }

  private:
    int64_t m_offset     = 0;
    Type    m_reflection = nullptr;
  };

  class BFC_API ConstructorReflection {
  public:
    template<typename T, typename... Args>
    ConstructorReflection(ReflectConstructor<T, Args...> ctor) {
      m_unpackAndInvoke = getUnpackAndInvokeImpl(ctor, std::make_integer_sequence<int64_t, sizeof...(Args)>{});
    }

    template<typename... Args>
    bool is() const {
      return m_funcType == TypeID<AllocatorT<Args...>>();
    }
  
    inline bool invoke(void * pInstance, Vector<RuntimeObject> const & argList) const {
      return m_unpackAndInvoke(pInstance, argList.getView());
    }

    Vector<bfc::type_index> const & getArgList() const {
      return m_argList;
    }

  private:
    template<typename T, typename... Args, int64_t... Indices>
    std::function<bool(void *, Span<RuntimeObject>)> getUnpackAndInvokeImpl(ReflectConstructor<T, Args...>,
                                                                            std::integer_sequence<int64_t, Indices...>) const {
      return [](void * pInstance, Span<RuntimeObject> argList) {
        // Check arg count is correct
        if (argList.size() != sizeof...(Args))
          return false;
        // Check types are correct
        if (((argList[Indices].typeInfo() != TypeID<Args>()) || ...))
          return false;
        // Unpack arguments and invoke method
        ReflectConstructor<T, Args...>::call((T *)pInstance, static_cast<Args>(argList[Indices].get<std::remove_reference_t<Args>>())...);
        return true;
      };
    }

    Vector<bfc::type_index> m_argList;

    std::function<bool(void *, Span<RuntimeObject>)> m_unpackAndInvoke;
  };

  namespace detail {
    class BFC_API TypeReflection {
      /// Build runtime reflection information from static reflection data.
      template<typename ClassT, typename... Members>
      TypeReflection(Reflection<ClassT, Members...> const & staticReflection) {
        m_typeInfo     = staticReflection.typeInfo();
        m_templateInfo = staticReflection.templateInfo();
        m_size         = staticReflection.sizeOf();
        m_isDefault    = !has_reflect_v<ClassT>;
        build(staticReflection, std::make_integer_sequence<int64_t, staticReflection.size()>{});
      }

      template<typename ClassT, typename... Members, int64_t... Indices>
      void build(Reflection<ClassT, Members...> const & staticReflection, std::integer_sequence<int64_t, Indices...>) {
        using ReflectionT = Reflection<ClassT, Members...>;
        m_functions.clear();
        m_functionNames.clear();
        m_variables.clear();
        m_variableNames.clear();
        m_constructors.clear();

        (add<ReflectionT::TypeOf<Indices>>(staticReflection.name<Indices>(), staticReflection.getAttribute<Indices>()), ...);

        // Add default/copy/move constructors (if applicable)
        m_constructors.reserve(m_constructors.size() + 3); // Make sure m_constructors doesn't reallocate so pointers below remain valid
        if constexpr (staticReflection.hasDefaultConstructor) {
          m_constructors.pushBack(ReflectConstructor<ClassT>{});
          m_pDefaultConstructor =  &m_constructors.back();
        }

        if constexpr (staticReflection.hasCopyConstructor) {
          m_constructors.pushBack(ReflectConstructor<ClassT, ClassT const &>{});
          m_pCopyConstructor = &m_constructors.back();
        }

        if constexpr (staticReflection.hasMoveConstructor) {
          m_constructors.pushBack(ReflectConstructor<ClassT, ClassT &&>{});
          m_pMoveConstructor = &m_constructors.back();
        }

        if constexpr (staticReflection.hasCopyAssign) {
          m_copyAssign = [](void * pDst, void const * pSrc) { *(ClassT *)pDst = *(ClassT const *)pSrc; };
        }

        if constexpr (staticReflection.hasMoveAssign) {
          m_moveAssign = [](void * pDst, void * pSrc) { *(ClassT *)pDst = std::move(*(ClassT *)pSrc); };
        }

        if constexpr (staticReflection.hasInnerTypes) {
          m_innerTypes = buildInnerTypes(staticReflection.innerTypes);
        }

        m_destructor = [](void * pInstance, int64_t count) { mem::destruct<ClassT>((ClassT *)pInstance, count); };
      }

      template<typename... Reflections>
      Vector<Type> buildInnerTypes(std::tuple<Reflections...> const& refl) {
        return { runtimeReflect<Reflections::Type>()...};
      }

      template<typename T>
      auto add(char const * name, T member) {
        if constexpr (is_function_v<T>) {
          m_functions.add(name, member);
          m_functionNames.pushBack(name);
        } else {
          m_variables.add(name, member);
          m_variableNames.pushBack(name);
        }
      }

      template<typename ClassT, typename... Args>
      void add(char const *, ReflectConstructor<ClassT, Args...> ctor) {
        m_constructors.pushBack(ctor);
      }

      TypeReflection(TypeReflection const &) = delete;
      TypeReflection(TypeReflection &&)      = delete;

    public:
      template<typename T>
      static TypeReflection const & get() {
        static_assert(!std::is_reference_v<T>, "T cannot be a reference.");

        if constexpr (has_reflect_v<T>) {
          static TypeReflection refl(reflect<T>());
          return refl;
        } else {
          static TypeReflection refl(Reflection<T>{});
          return refl;
        }
      }

      /// Get names of the reflected variables.
      Vector<String> const & members() const;
      /// Get names of the reflected functions.
      Vector<String> const & methods() const;
      /// Get the number of constructors
      int64_t getConstructorCount() const;

      /// Check if a variable is in the reflected attributes.
      bool hasMember(StringView const & name) const;
      /// Check if a function is in the reflected attributes.
      bool hasMethod(StringView const & name) const;
      /// Check if the reflection has a constructor that takes the types specified.
      bool hasConstructor(Span<bfc::type_index> const & types) const;

      /// Get reflection data for a member of this type.
      MemberReflection const * getMember(StringView const & name) const;
      /// Get reflection data for a method in this type.
      MethodReflection const * getMethod(StringView const & name) const;
      /// Get a constructor that takes the specified types.
      ConstructorReflection const * getConstructor(Span<bfc::type_index> const & types) const;
      /// Get a constructor that takes the specified types.
      ConstructorReflection const * getConstructor(int64_t index) const;

      /// Get a pointer to the default constructor (nullptr if not applicable).
      ConstructorReflection * getDefaultConstructor() const;
      /// Get a pointer to the move constructor (nullptr if not applicable).
      ConstructorReflection * getMoveConstructor() const;
      /// Get a pointer to the copy constructor (nullptr if not applicable).
      ConstructorReflection * getCopyConstructor() const;

      /// Size of the type in bytes
      int64_t sizeOf() const;
      /// Get the type info for this type.
      bfc::type_index const & typeInfo() const;
      /// Get the template info for this type.
      bfc::template_index const & templateInfo() const;
      /// Allocate a buffer for an object of this type.
      void * allocate(int64_t count = 1) const;

      /// Get the type info for an inner parameter of a template class instance.
      Type innerType(int64_t count = 0) const;
      /// Get the number of inner types for a template class instance.
      int64_t innerTypeCount() const;

      /// Invoke the destructor on an instance of this type.
      void destruct(void * pInstance, int64_t count = 1) const;
      /// Copy assign from pSrc to pDst.
      /// @returns false if this type has no copy assign operator.
      bool copyAssign(void * pDst, void const * pSrc) const;
      /// Move assign from pSrc to pDst.
      /// @returns false if this type has no move assign operator.
      bool moveAssign(void * pDst, void * pSrc) const;

      template<typename T>
      inline bool is() const {
        return typeInfo() == TypeID<T>();
      }

      /// Check if this is the default reflection data.
      /// If true, no reflect() function is implemented for this type.
      bool isDefaultReflection() const;

    private:
      Map<String, MemberReflection> m_variables;
      Map<String, MethodReflection> m_functions;

      // Default constructors. If null, the type does not have the constructor (it is deleted).
      // These are reflected automatically.
      ConstructorReflection * m_pDefaultConstructor;
      ConstructorReflection * m_pCopyConstructor;
      ConstructorReflection * m_pMoveConstructor;
      // Additional reflected constructors
      Vector<ConstructorReflection> m_constructors;
      Vector<String> m_variableNames;
      Vector<String> m_functionNames;

      // Inner types of a template class instance
      Vector<Type>   m_innerTypes;

      std::function<void(void *, void const *)> m_copyAssign;
      std::function<void(void *, void *)> m_moveAssign;

      std::function<void(void *, int64_t)> m_destructor;

      int64_t             m_size = 0;

      bfc::type_index m_typeInfo         = TypeID<void>();
      bfc::template_index m_templateInfo = templateid<void>();

      bool                m_isDefault    = false;
    };
  } // namespace detail

  template<typename T>
  inline Type runtimeReflect() {
    return &detail::TypeReflection::get<T>();
  }
}
