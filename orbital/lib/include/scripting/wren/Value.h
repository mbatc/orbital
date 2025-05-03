#pragma once

#include "../../../vendor/wren/src/include/wren.hpp"
#include "../../../vendor/wren/src/include/wren_ext.hpp"

#include <stdint.h>

#include "../../core/StringView.h"
#include "../../core/Map.h"

#include "Slot.h"

namespace bfc {
  namespace scripting {
    namespace wren {
      class Field;
      class MethodSequence;
      class MethodIterator;

      class Value {
        friend MethodSequence;
      public:
        Value() = default;

        /// Create a value from a slot in the VM.
        Value(WrenVM * pVM, int32_t slot);
        /// Set the value to an existing handle.
        Value(WrenVM * pVM, WrenHandle * pHandle);
        Value(Value const & o);
        Value(Value && o) noexcept;
        Value & operator=(Value const & o);
        Value & operator=(Value && o) noexcept;

        ~Value();

        void reset();
        bool empty() const;
        bool null() const;

        template<typename T>
        auto get() const -> typename SlotTraits<T>::return_t {
          if (m_pHandle == nullptr) {
            return std::nullopt;
          }
          wrenSetSlotHandle(m_pVM, 0, m_pHandle);
          return Slot<T>::get(m_pVM, 0);
        }

        template<typename... Args>
        Value call(Value const & method, Args &&... args) const {
          if (empty() || method.empty())
            return {};

          return call(m_pVM, method, *this, std::forward<Args>(args)...);
        }

        template<typename... Args>
        Value call(String const & signature, Args &&... args) const {
          if (empty())
            return {};

          return call(Value{m_pVM, wrenMakeCallHandle(m_pVM, signature.c_str())}, std::forward<Args>(args)...);
        }

        /// Get the internal handle to the value
        WrenHandle * handle() const;

        /// Get the type of the value stored.
        WrenType type() const;

        template<typename T>
        explicit operator T() const & {
          return get<T>().value();
        }

        // Implicit cast for r-values
        template<typename T>
        operator T() && {
          return get<T>().value();
        }

        /// Access the value with `key`.
        /// If this Value is a Map,  FieldProxy<T> will resolve to the value at `key`.
        /// If this Value is a List, FieldProxy<T> will resolve to the value at the index `key`.
        ///   * A runtime error is thrown if `key` is not an index.
        /// If this Value is an Object or Class, FieldProxy<T> will resolve to a Method, getter, or setter call.
        template<typename T>
        Field operator[](T &&key) const {
          if (empty())
            return Field();

          wrenEnsureSlots(m_pVM, 1);
          Slot<std::decay_t<T>>::set(m_pVM, 0, std::forward<T>(key));
          Value wrenKey(m_pVM, 0);
          return Field(m_pVM, *this, wrenKey);
        }

        MethodSequence methods() const;

        int64_t count() const {
          switch (type()) {
          case WREN_TYPE_LIST: wrenSetSlotHandle(m_pVM, 0, m_pHandle); return wrenGetListCount(m_pVM, 0);
          case WREN_TYPE_MAP: wrenSetSlotHandle(m_pVM, 0, m_pHandle); return wrenGetMapCount(m_pVM, 0);
          }
          return 0;
        }

      private:
        template<size_t... Indices, typename... Args>
        static void pushArguments(WrenVM * pVM, std::index_sequence<Indices...>, Args &&... args) {
          wrenEnsureSlots(pVM, sizeof...(Args));

          (Slot<std::decay_t<Args>>::set(pVM, Indices, std::forward<Args>(args)), ...);
        }

        template<size_t... Indices, typename... Args>
        static void unpackAndPushArguments(WrenVM * pVM, Args &&... args) {
          wrenEnsureSlots(pVM, sizeof...(Args));

          pushArguments(pVM, std::index_sequence_for<Args...>{}, UnpackForSlot<std::decay_t<Args>>::unpack(std::forward<Args>(args))...);
        }

        template<typename... Args>
        static Value call(WrenVM * pVM, Value const & method, Value const & receiver, Args &&... args) {
          unpackAndPushArguments(pVM, receiver, std::forward<Args>(args)...);

          WrenInterpretResult result = wrenCall(pVM, method.handle());

          if (result != WREN_RESULT_SUCCESS)
            return Value();

          return Value(pVM, 0);
        }

        mutable std::optional<WrenType> m_typeCache;

        WrenVM *     m_pVM     = nullptr;
        WrenHandle * m_pHandle = nullptr;
      };

      /// Specialization to get/retreive a Value from a slot
      template<>
      struct Slot<Value> {
        inline static std::optional<Value> get(WrenVM * pVM, int32_t idx) {
          return Value(pVM, idx);
        }

        inline static void set(WrenVM * pVM, int32_t idx, Value const & value) {
          wrenSetSlotHandle(pVM, idx, value.handle());
        }
      };

      class MethodSequence {
      public:
        Value self;

        MethodIterator begin() const;
        MethodIterator end() const;
      };

      enum MethodType {
        MethodType_Unknown = -1,
        MethodType_Constructor,
        MethodType_Method,
        MethodType_Getter,
        MethodType_Setter,
        MethodType_Count,
      };

      class Field {
      public:
        Field() = default;

        Field(WrenVM * pVM, Value const & self, Value const & key)
          : m_self(self)
          , m_key(key)
          , m_pVM(pVM) {
        }
        
        bool empty() const {
          return m_pVM == nullptr || m_self.empty() || m_key.empty();
        }

        template<typename... Args>
        Value operator()(Args &&... args) const {
          if (empty())
            return {};

          Value * pMethodHandle = m_signatures.tryGet(sizeof...(Args));

          if (pMethodHandle == nullptr) {
            // Cache the call signature for sizeof...(Args)
            auto methodName = m_key.get<const char *>();
            if (!methodName.has_value())
              return {}; // Key is not valid
            m_signatures.add(sizeof...(Args), Value{m_pVM, wrenMakeCallHandle(m_pVM, getMethodSignature<sizeof...(Args)>(methodName.value()).c_str())});

            pMethodHandle = m_signatures.tryGet(sizeof...(Args));
          }

          if (pMethodHandle == nullptr)
            return {};

          // Invoke on the receiver
          return m_self.call(*pMethodHandle, std::forward<Args>(args)...);
        }

        template<typename T>
        Value operator=(T const & value) const {
          return set(value);
        }

        operator Value() const {
          return get();
        }

        template<typename T>
        explicit operator T() const & {
          return get().get<T>().value();
        }

        // Implicit cast for r-values
        template<typename T>
        operator T() && {
          return get().get<T>().value();
        }

        /// Invoke getter
        Value get() const {
          if (empty())
            return {};

          switch (m_self.type()) {
          case WREN_TYPE_FOREIGN:
          case WREN_TYPE_UNKNOWN:
            if (m_getter.empty()) {
              // Cache the call signature for the getter
              auto methodName = m_key.get<const char *>();
              if (!methodName.has_value())
                return {}; // Key is not valid
              m_getter = {m_pVM, wrenMakeCallHandle(m_pVM, methodName.value())};
            }
            return m_self.call(m_getter);
          case WREN_TYPE_LIST: {
            auto elementIndex = m_key.get<int32_t>();
            if (!elementIndex.has_value())
              return {};

            wrenEnsureSlots(m_pVM, 2);
            Slot<Value>::set(m_pVM, 0, m_self);
            wrenGetListElement(m_pVM, 0, elementIndex.value(), 1);
            return Value(m_pVM, 1);
          }
          case WREN_TYPE_MAP: {
            wrenEnsureSlots(m_pVM, 3);
            Slot<Value>::set(m_pVM, 0, m_self);
            Slot<Value>::set(m_pVM, 1, m_key);
            wrenGetMapValue(m_pVM, 0, 1, 2);
            return Value(m_pVM, 2);
          }
          }

          return {};
        }

        /// Invoke getter
        template<typename T>
        std::optional<T> get() const {
          return get().get<T>();
        }

        /// Invoke setter
        template<typename T>
        Value set(T const & value) const {
          if (empty())
            return {};

          switch (m_self.type()) {
          case WREN_TYPE_UNKNOWN:
          case WREN_TYPE_LIST:
          }
          switch (m_self.type()) {
          case WREN_TYPE_FOREIGN:
          case WREN_TYPE_UNKNOWN:
            if (m_setter.empty()) {
              // Cache the call signature for the setter
              auto methodName = m_key.get<const char *>();
              if (!methodName.has_value())
                return {}; // Key is not valid
              m_setter = {m_pVM, wrenMakeCallHandle(m_pVM, String::format("%s=(_)", methodName.value()).c_str())};
            }

            return m_self.call(m_setter, value);
          case WREN_TYPE_LIST: {
            auto elementIndex = m_key.get<int32_t>();
            if (!elementIndex.has_value())
              return {};

            wrenEnsureSlots(m_pVM, 2);
            Slot<Value>::set(m_pVM, 0, m_self);
            Slot<T>::set(m_pVM, 1, value);
            wrenSetListElement(m_pVM, 0, elementIndex.value(), 1);
            return m_self;
          }
          case WREN_TYPE_MAP: {
            wrenEnsureSlots(m_pVM, 3);
            Slot<Value>::set(m_pVM, 0, m_self);
            Slot<Value>::set(m_pVM, 1, m_key);
            Slot<T>::set(m_pVM, 2, value);
            wrenSetMapValue(m_pVM, 0, 1, 2);
            return Value(m_pVM, 2, wrenGetSlotType(m_pVM, 2));
          }
          }

          return {};
        }

        /// Access the value with `key`.
        /// If this Value is a Map,  FieldProxy<T> will resolve to the value at `key`.
        /// If this Value is a List, FieldProxy<T> will resolve to the value at the index `key`.
        ///   * A runtime error is thrown if `key` is not an index.
        /// If this Value is an Object or Class, FieldProxy<T> will resolve to a Method, getter, or setter call.
        template<typename T>
        Field operator[](T const & key) const {
          return get()[key];
        }

        int64_t count() const {
          return get().count();
        }

      private:
        WrenVM * m_pVM;

        Value m_self;
        Value m_key;

        mutable Value               m_setter;
        mutable Value               m_getter;
        mutable Map<int32_t, Value> m_signatures;

        template<size_t N>
        inline static String getMethodSignature(String const & methodName) {
          if constexpr (N == 0) {
            return String::format("%s()", methodName);
          } else {
            return String::format("%s(%s)", methodName, String::join(Array<StringView, N>("_"), ","));
          }
        }
      };

      /// Specialization to get/retreive a Value from a slot
      template<>
      struct Slot<Field> {
        inline static void set(WrenVM * pVM, int32_t idx, Field const & value) {
          Slot<Value>::set(pVM, idx, value);
        }
      };

      /// We need to unpack the field before actually assigning function parameters to a slot.
      /// This is because resolving a field may involve manipulating slots or calling methods.
      template<>
      struct UnpackForSlot<Field> {
        inline static Value unpack(Field const & o) {
          return o;
        }
      };

      class MethodDesc {
        WrenVM * m_pVM = nullptr;

      public:
        MethodDesc(WrenVM * pVM, StringView const & symbol)
          : m_pVM(pVM)
          , type(type)
          , symbol(symbol){
          type = MethodType_Unknown;
          signature          = symbol;

          if (symbol.startsWith("init ")) {
            signature = symbol.substr(5); // Skip 'init '
            type      = MethodType_Constructor;
          }

          int64_t parenStart = signature.find("(");
          if (type == MethodType_Unknown) {
            if (parenStart == npos) {
              type = MethodType_Getter;
            } else if (symbol[parenStart - 1] == '=') {
              type = MethodType_Setter;
            } else {
              type = MethodType_Method;
            }
          }

          if (parenStart != npos) {
            for (auto & c : signature.substr(parenStart + 1))
              numParams += c == '_' ? 1 : 0;

            name = signature.substr(0, parenStart);
          } else {
            name = signature;
          }
        }

        String     name;
        String     signature;
        String     symbol;
        MethodType type      = MethodType_Unknown;
        int64_t    numParams = 0;

        Value handle() const {
          return Value(m_pVM, wrenMakeCallHandle(m_pVM, signature.c_str()));
        }
      };

      class MethodIterator {
      public:
        MethodIterator()
          : MethodIterator(nullptr, nullptr, -1) {}
        MethodIterator(WrenVM * pVM, WrenHandle * classHandle, int32_t symbolIndex)
          : m_pVM(pVM)
          , m_pClass(classHandle)
          , m_symbolIndex(symbolIndex) {}

        bool operator!=(MethodIterator const & o) const {
          return !operator==(o);
        }

        bool operator==(MethodIterator const & o) const {
          return m_symbolIndex == o.m_symbolIndex;
        }

        MethodIterator & operator++() {
          if (m_pClass == nullptr || m_pVM == nullptr)
            return *this;

          wrenEnsureSlots(m_pVM, 2);
          wrenSetSlotHandle(m_pVM, 0, m_pClass);
          wrenSetSlotDouble(m_pVM, 1, (double)m_symbolIndex);

          if (wrenGetClassNextMethod(m_pVM, 0, 1))
            m_symbolIndex = (int32_t)wrenGetSlotDouble(m_pVM, 1);
          else
            m_symbolIndex = -1; // Invalid symbol index

          return *this;
        }

        MethodIterator operator++(int) {
          MethodIterator prev = *this;
                         operator++();
          return prev;
        }

        MethodDesc operator*() const {
          BFC_ASSERT(m_symbolIndex != -1, "Symbol index is invalid");

          wrenSetSlotDouble(m_pVM, 0, (double)m_symbolIndex);

          return { m_pVM, wrenGetMethodSymbol(m_pVM, 0) };
        }

      private:
        WrenVM *     m_pVM         = nullptr;
        WrenHandle * m_pClass      = nullptr;
        int32_t      m_symbolIndex = 0;
      };
    }
  }
}
