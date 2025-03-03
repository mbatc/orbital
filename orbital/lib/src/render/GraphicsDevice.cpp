#include "render/GraphicsDevice.h"
#include "core/Map.h"

namespace bfc {
  Vector<Pair<String, GraphicsDeviceFactory>> g_devices;

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
