#include "util/Delegate.h"

namespace bfc {
  Ref<DelegateHandle> DelegateCollectionBase::createHandle(int64_t index) {
    return NewRef<DelegateHandle>(DelegateHandle(this, index));
  }
  
  DelegateHandle::DelegateHandle(DelegateHandle && o)
    : m_pCollection(o.m_pCollection)
    , m_index(o.m_index) {
    o.m_pCollection = nullptr;
    o.m_index       = -1;
  }

  DelegateHandle & DelegateHandle::operator=(DelegateHandle && o) {
    std::swap(m_pCollection, o.m_pCollection);
    std::swap(m_index, o.m_index);
    return *this;
  }

  DelegateHandle ::~DelegateHandle() {
    if (m_pCollection != nullptr) {
      m_pCollection->remove(m_index);
    }
  }

  DelegateHandle ::DelegateHandle(DelegateCollectionBase * pCollection, int64_t index)
    : m_pCollection(pCollection)
    , m_index(index) {}
} // namespace bfc
