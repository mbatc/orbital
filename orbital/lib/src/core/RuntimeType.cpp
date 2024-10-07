#include "core/RuntimeType.h"

namespace bfc {
  namespace detail {
    Vector<String> const & TypeReflection::members() const {
      return m_variableNames;
    }

    Vector<String> const & TypeReflection::methods() const {
      return m_functionNames;
    }

    int64_t TypeReflection::getConstructorCount() const {
      return m_constructors.size();
    }

    bool TypeReflection::hasMember(StringView const & name) const {
      return m_variables.contains(name);
    }

    bool TypeReflection::hasMethod(StringView const & name) const {
      return m_functions.contains(name);
    }

    bool TypeReflection::hasConstructor(Span<bfc::type_index> const & types) const {
      return getConstructor(types) != nullptr;
    }

    MemberReflection const * TypeReflection::getMember(StringView const & name) const {
      return m_variables.tryGet(name);
    }

    MethodReflection const * TypeReflection::getMethod(StringView const & name) const {
      return m_functions.tryGet(name);
    }

    ConstructorReflection const * TypeReflection::getConstructor(Span<bfc::type_index> const & types) const {
      for (ConstructorReflection const & ctor : m_constructors)
        if (ctor.getArgList() == types)
          return &ctor;
      return nullptr;
    }

    ConstructorReflection const * TypeReflection::getConstructor(int64_t index) const {
      return &m_constructors[index];
    }

    ConstructorReflection * TypeReflection::getDefaultConstructor() const{
      return m_pDefaultConstructor;
    }

    ConstructorReflection * TypeReflection::getMoveConstructor() const {
      return m_pMoveConstructor;
    }

    ConstructorReflection * TypeReflection::getCopyConstructor() const {
      return m_pCopyConstructor;
    }

    int64_t TypeReflection::sizeOf() const {
      return m_size;
    }

    type_index const & TypeReflection::typeInfo() const {
      return m_typeInfo;
    }

    template_index const & TypeReflection::templateInfo() const {
      return m_templateInfo;
    }

    void * TypeReflection::allocate(int64_t count) const {
      return mem::alloc(sizeOf() * count);
    }

    Type TypeReflection::innerType(int64_t i) const {
      return i < m_innerTypes.size() ? m_innerTypes[i] : nullptr;
    }

    int64_t TypeReflection::innerTypeCount() const {
      return m_innerTypes.size();
    }

    void TypeReflection::destruct(void * pInstance, int64_t count) const {
      return m_destructor(pInstance, count);
    }

    bool TypeReflection::copyAssign(void * pDst, void const * pSrc) const {
      if (m_copyAssign == nullptr) {
        return false;
      }

      m_copyAssign(pDst, pSrc);
      return true;
    }

    bool TypeReflection::moveAssign(void * pDst, void * pSrc) const {
      if (m_moveAssign == nullptr) {
        return false;
      }

      m_moveAssign(pDst, pSrc);
      return true;
    }

    bool TypeReflection::isDefaultReflection() const {
      return m_isDefault;
    }
  } // namespace detail

  type_index const & MethodReflection::returnTypeInfo() const {
    return m_returnType;
  }

  type_index MethodReflection::paramTypeInfo(int64_t const i) const {
    return i >= 0 && i < m_argTypes.size() ? m_argTypes[i] : TypeID<void>();
  }

  int64_t MethodReflection::paramCount() const {
    return m_argTypes.size();
  }

  type_index const & MethodReflection::typeInfo() const {
    return m_funcType;
  }

  bool MemberReflection::is(type_index const & type) const {
    return false;
  }

  void * MemberReflection::getPtr(void * pInstance) const {
    return (uint8_t *)pInstance + m_offset;
  }

  Type MemberReflection::reflection() const {
    return m_reflection;
  }
} // namespace bfc
