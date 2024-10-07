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

  int64_t GraphicsDevice::getReferences(GraphicsResource resourceID) {
    switch (resourceID.type) {
    case GraphicsResourceType_Buffer: return getBufferManager()->getBufferReferences(resourceID);
    case GraphicsResourceType_VertexArray: return getBufferManager()->getVertexArrayReferences(resourceID);
    case GraphicsResourceType_Texture: return getTextureManager()->getTextureReferences(resourceID);
    case GraphicsResourceType_Sampler: return getTextureManager()->getSamplerReferences(resourceID);
    case GraphicsResourceType_Shader: return getShaderManager()->getShaderReferences(resourceID);
    case GraphicsResourceType_Program: return getShaderManager()->getProgramReferences(resourceID);
    case GraphicsResourceType_RenderTarget: return getRenderTargetManager()->getRenderTargetReferences(resourceID);
    default: return -1;
    }
  }

  GraphicsResource GraphicsDevice::refResource(GraphicsResource resourceID) {
    switch (resourceID.type) {
    case GraphicsResourceType_Buffer: getBufferManager()->refBuffer(resourceID); break;
    case GraphicsResourceType_VertexArray: getBufferManager()->refVertexArray(resourceID); break;
    case GraphicsResourceType_Texture: getTextureManager()->refTexture(resourceID); break;
    case GraphicsResourceType_Sampler: getTextureManager()->refSampler(resourceID); break;
    case GraphicsResourceType_Shader: getShaderManager()->refShader(resourceID); break;
    case GraphicsResourceType_Program: getShaderManager()->refProgram(resourceID); break;
    case GraphicsResourceType_RenderTarget: getRenderTargetManager()->refRenderTarget(resourceID); break;
    default: return GraphicsResource();
    }
    return resourceID;
  }

  void GraphicsDevice::releaseResource(GraphicsResource * pResourceID) {
    switch (pResourceID->type) {
    case GraphicsResourceType_Buffer: getBufferManager()->releaseBuffer(pResourceID); break;
    case GraphicsResourceType_VertexArray: getBufferManager()->releaseVertexArray(pResourceID); break;
    case GraphicsResourceType_Texture: getTextureManager()->releaseTexture(pResourceID); break;
    case GraphicsResourceType_Sampler: getTextureManager()->releaseSampler(pResourceID); break;
    case GraphicsResourceType_Shader: getShaderManager()->releaseShader(pResourceID); break;
    case GraphicsResourceType_Program: getShaderManager()->releaseProgram(pResourceID); break;
    case GraphicsResourceType_RenderTarget: getRenderTargetManager()->releaseRenderTarget(pResourceID); break;
    }
  }

  ManagedGraphicsResource::ManagedGraphicsResource(GraphicsDevice * pDevice, GraphicsResource resource) {
    m_pDevice  = pDevice;
    m_resource = resource;
  }

  ManagedGraphicsResource::ManagedGraphicsResource(ManagedGraphicsResource const & o) {
    if (o.m_pDevice != nullptr) {
      m_resource = o.m_pDevice->refResource(o.m_resource);
      m_pDevice  = o.m_pDevice;
    }
  }

  ManagedGraphicsResource::ManagedGraphicsResource(ManagedGraphicsResource && o) {
    m_resource = o.takeResource(&m_pDevice);
  }

  ManagedGraphicsResource::~ManagedGraphicsResource() {
    release();
  }

  ManagedGraphicsResource & ManagedGraphicsResource::operator=(ManagedGraphicsResource const & o) {
    set(o.m_pDevice, o.m_resource);
    return *this;
  }

  ManagedGraphicsResource & ManagedGraphicsResource::operator=(ManagedGraphicsResource && o) {
    std::swap(o.m_pDevice, m_pDevice);
    std::swap(o.m_resource, m_resource);
    return *this;
  }

  bool ManagedGraphicsResource::hasResource() const {
    return m_resource != InvalidGraphicsResource;
  }

  void ManagedGraphicsResource::release() {
    if (m_pDevice != nullptr)
      m_pDevice->releaseResource(&m_resource);
    m_pDevice = nullptr;
  }

  GraphicsResource ManagedGraphicsResource::takeResource(GraphicsDevice ** ppDevice) {
    if (ppDevice != nullptr)
      *ppDevice = m_pDevice;
    m_pDevice = nullptr;
    return m_resource.take();
  }

  GraphicsResource ManagedGraphicsResource::getResource() const {
    return m_resource;
  }

  GraphicsDevice * ManagedGraphicsResource::getDevice() const {
    return m_pDevice;
  }

  GraphicsResourceType ManagedGraphicsResource::getType() const {
    return m_resource.type;
  }

  int64_t ManagedGraphicsResource::getReferences() const {
    return m_pDevice == nullptr ? -1 : m_pDevice->getReferences(m_resource);
  }

  ManagedGraphicsResource::operator GraphicsResource() const {
    return m_resource;
  }

  void ManagedGraphicsResource::set(GraphicsDevice * pDevice, GraphicsResource resource) {
    if (pDevice != nullptr)
      pDevice->refResource(resource);
    if (m_pDevice != nullptr)
      m_pDevice->releaseResource(&m_resource);

    m_pDevice  = pDevice;
    m_resource = resource;
  }
} // namespace bfc
