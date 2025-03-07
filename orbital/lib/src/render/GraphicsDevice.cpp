#include "render/GraphicsDevice.h"
#include "core/Map.h"

namespace bfc {
  static Vector<Pair<String, GraphicsDeviceFactory>> g_devices;

  namespace graphics {
    StateManager::StateManager() {
      set(State::EnableBlend{true}, State::EnableStencilTest{false}, State::EnableScissorTest{false}, State::EnableDepthRead{true},
          State::EnableDepthWrite{true}, State::DepthRange{0.0f, 1.0f}, State::DepthFunc{ComparisonFunction_Less},
          State::BlendFunc{BlendFunction_SourceAlpha, BlendFunction_OneMinusSourceAlpha}, State::BlendEq{BlendEquation_Add}, State::ColourWrite{true},
          State::ColourFactor{1.0f});
    }

    bool StateManager::beginGroup() {
      if (m_groupStart.has_value())
        return false;
      
      m_groupStart = m_stack.size();
      return true;
    }

    bool StateManager::endGroup() {
      if (!m_groupStart.has_value())
        return false;

      m_groups.pushBack(m_stack.size() - m_groupStart.value());
      m_groupStart.reset();
      return true;
    }

    void StateManager::push(Span<const State> const & states) {
      BFC_ASSERT(beginGroup(), "push cannot be called between beginGroup/endGroup. Use set() instead");
      set(states);
      BFC_ASSERT(endGroup(), "Failed to end the group");
    }

    void StateManager::push(State const & state) {
      beginGroup();
      set(state);
      endGroup();
    }

    void StateManager::pop() {
      BFC_ASSERT(m_groups.size() > 0, "No groups have been pushed");
      BFC_ASSERT(!m_groupStart.has_value(), "Cannot pop() between beginGroup/endGroup calls");

      int64_t count = m_groups.popBack();
      while (count-- > 0) {
        State previous = m_stack.popBack();
        m_state[previous.index()] = previous;
        m_changes.pushBack(previous);
      }
    }

    void StateManager::set(Span<const State> const & states) {
      if (m_groupStart.has_value()) {
        for (auto & state : states) {
          if (!m_state[state.index()].is<std::monostate>()) {
            m_stack.pushBack(m_state[state.index()]);
          }
        }
      }

      for (auto & state : states)
        m_state[state.index()] = state;
      m_changes.pushBack(states);
    }

    void StateManager::set(State const & state) {
      if (m_groupStart.has_value()) {
        if (!m_state[state.index()].is<std::monostate>()) {
          m_stack.pushBack(m_state[state.index()]);
        }
      }

      m_state[state.index()] = state;
      m_changes.pushBack(state);
    }

    void StateManager::apply() {
      for (auto & state : m_changes)
        apply(state);

      m_changes.clear();
    }
  }

  int64_t getDepthStencilFormatStride(DepthStencilFormat const & type) {
    switch (type) {
    case DepthStencilFormat_D24S8: return 4;
    case DepthStencilFormat_D32: return 4;
    }
    return 0;
  }

  int64_t getDataTypeSize(DataType const & type) {
    switch (type) {
    case DataType_Bool: return sizeof(bool);
    case DataType_UInt8: return sizeof(uint8_t);
    case DataType_Int8: return sizeof(int8_t);
    case DataType_UInt16: return sizeof(uint16_t);
    case DataType_Int16: return sizeof(int16_t);
    case DataType_Int32: return sizeof(int32_t);
    case DataType_UInt32: return sizeof(uint32_t);
    case DataType_Float32: return sizeof(float);
    case DataType_Float64: return sizeof(double);
    }
    return 0;
  }

  BFC_API bool registerGraphicsDevice(StringView const & name, GraphicsDeviceFactory Factory) {
    for (Pair<String, GraphicsDeviceFactory> & item : g_devices) {
      if (item.first == name) {
        return false;
      }
    }

    g_devices.pushBack({name, Factory});
    return true;
  }

  BFC_API Ref<GraphicsDevice> createGraphicsDevice(StringView const & name) {
    for (Pair<String, GraphicsDeviceFactory> & item : g_devices) {
      if (item.first == name) {
        return item.second();
      }
    }

    return nullptr;
  }

  BFC_API int64_t getGraphicsDeviceCount() {
    return g_devices.size();
  }

  BFC_API StringView getGraphicsDeviceName(int64_t index) {
    return g_devices[index].first;
  }
} // namespace bfc

