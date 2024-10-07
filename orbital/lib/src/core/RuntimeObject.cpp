#include "core/RuntimeType.h"
#include "core/RuntimeObject.h"

namespace bfc {
  RuntimeObject::RuntimeObject(void* pInstance, Type reflection, bool isOwner) {
    m_pData = std::make_shared<ControlBlock>(pInstance, reflection, isOwner);
  }

  /// Const pointers
  RuntimeObject RuntimeObject::move(void * pInstance, Type reflection) {
    if (reflection->getMoveConstructor() == nullptr) {
      return RuntimeObject();
    }

    void * pNewInstance = reflection->allocate();
    if (reflection->getMoveConstructor()->invoke(pNewInstance, {RuntimeObject((void *)pInstance, reflection, false)})) {
      return RuntimeObject(pNewInstance, reflection, true);
    }

    mem::free(pNewInstance);
    return RuntimeObject();
  }

  RuntimeObject RuntimeObject::copy(void const* pInstance, Type reflection) {
    if (reflection->getCopyConstructor() == nullptr) {
      return RuntimeObject();
    }

    void * pNewInstance = reflection->allocate();
    if (reflection->getCopyConstructor()->invoke(pNewInstance, { RuntimeObject((void*)pInstance, reflection, false) })) {
      return RuntimeObject(pNewInstance, reflection, true);
    }
    mem::free(pNewInstance);
    return RuntimeObject();
  }

  RuntimeObject RuntimeObject::bind(void* pInstance, Type reflection) {
    return RuntimeObject(pInstance, reflection, false);
  }

  RuntimeObject RuntimeObject::construct(Type reflection, Vector<RuntimeObject> const& argList) {
    Vector<type_index> argTypes;
    argTypes.reserve(argList.size());
    void *                        pInstance    = reflection->allocate();
    for (int64_t i = 0; i < reflection->getConstructorCount(); ++i) {
      ConstructorReflection const * pConstructor = reflection->getConstructor(i);
      if (pConstructor->invoke(pInstance, argList)) {
        return RuntimeObject(pInstance, reflection, true);
      }
    }

    mem::free(pInstance);
    return RuntimeObject();
  }

  Vector<String> const & RuntimeObject::members() {
    BFC_ASSERT(!isEmpty(), "RuntimeObject is null");
    return reflection()->members();
  }

  Vector<String> const & RuntimeObject::methods() {
    BFC_ASSERT(!isEmpty(), "RuntimeObject is null");
    return reflection()->methods();
  }

  RuntimeObject RuntimeObject::call(StringView const & name, Vector<RuntimeObject> const & args) {
    BFC_ASSERT(!isEmpty(), "RuntimeObject is null");
    MethodReflection const * pMethod = m_pData->type->getMethod(name);
    if (pMethod == nullptr) {
      BFC_ASSERT(false, "Instance of type %s has no method %s", m_pData->type->typeInfo().name(), String(name).c_str());
      return RuntimeObject();
    }
    return pMethod->invoke(m_pData->pInstance, args);
  }

  RuntimeObject RuntimeObject::get(StringView const & name) {
    BFC_ASSERT(!isEmpty(), "RuntimeObject is null");
    MemberReflection const * pMember = m_pData->type->getMember(name);
    if (pMember == nullptr) {
      BFC_ASSERT(false, "Instance of type %s has no member %s", m_pData->type->typeInfo().name(), String(name).c_str());
      return RuntimeObject();
    }

    return RuntimeObject::bind(pMember->getPtr(m_pData->pInstance), pMember->reflection());
  }

  bool RuntimeObject::assign(RuntimeObject const & value) {
    BFC_ASSERT(!isEmpty(), "RuntimeObject is null");
    return typeInfo() == value.typeInfo() && m_pData->type->copyAssign(data(), value.data());
  }

  bool RuntimeObject::assign(RuntimeObject && value) {
    BFC_ASSERT(!isEmpty(), "RuntimeObject is null");
    return typeInfo() == value.typeInfo() && m_pData->type->moveAssign(data(), value.data());
  }

  type_index RuntimeObject::typeInfo() const {
    return m_pData != nullptr ? m_pData->type->typeInfo() : TypeID<void>();
  }

  template_index RuntimeObject::templateInfo() const {
    return m_pData != nullptr ? m_pData->type->templateInfo() : templateid<void>();
  }
  
  Type RuntimeObject::reflection() const {
    return m_pData != nullptr ? m_pData->type : nullptr;
  }

  void* RuntimeObject::data() {
    return m_pData != nullptr ? m_pData->pInstance : nullptr;
  }

  void const * RuntimeObject::data() const {
    return m_pData != nullptr ? m_pData->pInstance : nullptr;
  }

  bool RuntimeObject::isEmpty() const {
    return m_pData == nullptr || m_pData->pInstance == nullptr;
  }

  RuntimeObject::ControlBlock::ControlBlock(void * pInstance, Type reflection, bool isOwner) 
    : pInstance(pInstance)
    , isOwner(isOwner)
    , type(reflection)
  {}

  RuntimeObject::ControlBlock::ControlBlock(ControlBlock && o) {
    std::swap(isOwner, o.isOwner);
    std::swap(pInstance, o.pInstance);
    std::swap(type, o.type);
  }

  RuntimeObject::ControlBlock::~ControlBlock() {
    if (isOwner && pInstance != nullptr) {
      type->destruct(pInstance);
      mem::free(pInstance);
      pInstance = nullptr;
    }
    type = nullptr;
    isOwner = false;
  }
} // namespace bfc