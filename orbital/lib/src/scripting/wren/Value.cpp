#include "scripting/wren/Value.h"


namespace bfc {
  namespace scripting {
    namespace wren {
      Value::Value(WrenVM * pVM, int32_t slot)
        : m_pVM(pVM)
        , m_pHandle(wrenGetSlotHandle(pVM, slot))
        , m_typeCache(wrenGetSlotType(pVM, slot)) {}

      Value::Value(WrenVM * pVM, WrenHandle * pHandle)
        : m_pVM(pVM)
        , m_pHandle(pHandle) {}

      Value::Value(Value const & o) {
        if (o.empty())
          return;

        wrenEnsureSlots(o.m_pVM, 1);
        // Move o's value into the slot
        wrenSetSlotHandle(o.m_pVM, 0, o.m_pHandle);
        // Create a new handle to o's value.
        m_pHandle = wrenGetSlotHandle(o.m_pVM, 0);
        m_pVM     = o.m_pVM;
        m_typeCache = o.m_typeCache;
      }

      Value & Value::operator=(Value const & o) {
        if (o.empty()) {
          reset();
        }

        wrenEnsureSlots(o.m_pVM, 1);
        // Move o's value into the slot
        wrenSetSlotHandle(o.m_pVM, 0, o.m_pHandle);
        // Create a new handle to o's value.
        m_pHandle = wrenGetSlotHandle(o.m_pVM, 0);
        m_pVM       = o.m_pVM;
        m_typeCache = o.m_typeCache;
        return *this;
      }

      Value & Value::operator=(Value && o) noexcept {
        std::swap(o.m_pHandle, m_pHandle);
        std::swap(o.m_pVM, m_pVM);
        std::swap(o.m_typeCache, m_typeCache);
        return *this;
      }

      Value::Value(Value && o) noexcept {
        if (o.empty()) {
          reset();
        }
        std::swap(o.m_pHandle, m_pHandle);
        std::swap(o.m_pVM, m_pVM);
        std::swap(o.m_typeCache, m_typeCache);
      }

      Value::~Value() {
        reset();
      }

      void Value::reset() {
        if (empty())
          return;

        wrenReleaseHandle(m_pVM, m_pHandle);
        m_pVM     = nullptr;
        m_pHandle = nullptr;
        m_typeCache.reset();
      }

      bool Value::empty() const {
        return m_pHandle == nullptr || m_pVM == nullptr;
      }

      bool Value::null() const {
        return type() == WREN_TYPE_NULL;
      }

      WrenHandle * Value::handle() const {
        return m_pHandle;
      }

      WrenType Value::type() const {
        if (!m_typeCache.has_value()) {
          wrenSetSlotHandle(m_pVM, 0, m_pHandle);

          m_typeCache = wrenGetSlotType(m_pVM, 0);
        }

        return m_typeCache.value();
      }

      MethodSequence Value::methods() const {
        return { *this };
      }

      MethodIterator MethodSequence::begin() const {
        return ++MethodIterator(self.m_pVM, self.m_pHandle, -1);
      }

      MethodIterator MethodSequence::end() const {
        return MethodIterator();
      }
    } // namespace wren
  } // namespace scripting
} // namespace bfc
