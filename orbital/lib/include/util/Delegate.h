#pragma once

#include <functional>
#include "../core/Pool.h"

namespace bfc {
  class DelegateCollectionBase;
  class BFC_API DelegateHandle {
    friend DelegateCollectionBase;
  public:
    DelegateHandle() = default;
    DelegateHandle(DelegateHandle && o);
    DelegateHandle & operator=(DelegateHandle && o);
    ~DelegateHandle();

  private:
    DelegateHandle(DelegateHandle const & o) = delete;
    DelegateHandle(DelegateCollectionBase * pCollection, int64_t index);

    DelegateCollectionBase * m_pCollection = nullptr;
    int64_t                  m_index       = -1;
  };

  using DelegatePtr = Ref<DelegateHandle>;

  class BFC_API DelegateCollectionBase {
  protected:
    friend DelegateHandle;

    virtual void remove(int64_t index) = 0;
    DelegatePtr  createHandle(int64_t index);
  };

  template<typename ReturnT, typename... Args>
  class DelegateCollection : public DelegateCollectionBase {
  public:
    using CallbackType = std::function<ReturnT(Args...)>;

    /// Invokes all callbacks in the collection.
    auto invoke(Args... args) {
      if constexpr (std::is_void_v<ReturnT>) {
        for (CallbackType & callback : m_callback) {
          callback(args...);
        }
      } else {
        Vector<ReturnT> ret;
        ret.reserve(m_callback.size());
        for (CallbackType & callback : m_callback) {
          ret.pushBack(callback(args...));
        }
        return ret;
      }
    }

    /// Add a callback to the collection.
    /// A shared_ptr to a DelegateHandle is returned.
    /// The callback is removed when the shared_ptr is destroyed.
    DelegatePtr add(CallbackType const & func) {
      return DelegateCollectionBase::createHandle(m_callback.emplace(func));
    }

  protected:
    virtual void remove(int64_t index) {
      m_callback.erase(index);
    }

  private:
    Pool<CallbackType> m_callback;
  };
}
