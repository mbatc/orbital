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
      for (auto & state : states) {
        _set(state);
      }
      BFC_ASSERT(endGroup(), "Failed to end the group");
    }

    void StateManager::push(State const & state) {
      return _push(state);
    }

    void StateManager::_push(State const & state) {
      BFC_ASSERT(beginGroup(), "push cannot be called between beginGroup/endGroup. Use set() instead");
      _set(state);
      BFC_ASSERT(endGroup(), "Failed to end the group");
    }

    void StateManager::pop() {
      BFC_ASSERT(m_groups.size() > 0, "No groups have been pushed");
      BFC_ASSERT(!m_groupStart.has_value(), "Cannot pop() between beginGroup/endGroup calls");

      int64_t count = m_groups.popBack();
      while (count-- > 0) {
        State previous            = m_stack.popBack();
        m_state[previous.index()] = previous;
        m_changes.pushBack(previous);
      }
    }

    void StateManager::set(State const & state) {
      return _set(state);
    }

    void StateManager::_set(State const & state) {
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

    graphics::BufferRef CommandList::createBuffer(BufferUsageHint usageHint) {
      return getDevice()->createBuffer();
    }

    graphics::VertexArrayRef CommandList::createVertexArray() {
      return getDevice()->createVertexArray();
    }

    graphics::ProgramRef CommandList::createProgram() {
      return getDevice()->createProgram();
    }

    graphics::TextureRef CommandList::createTexture(TextureType type) {
      return getDevice()->createTexture(type);
    }

    graphics::SamplerRef CommandList::createSampler() {
      return getDevice()->createSampler();
    }

    graphics::RenderTargetRef CommandList::createRenderTarget(RenderTargetType type) {
      return getDevice()->createRenderTarget(type);
    }

    void Program::setSource(Map<ShaderType, String> const & sources) {
      for (int64_t i = 0; i < ShaderType_Count; ++i) {
        ShaderType type = ShaderType(i);
        if (sources.contains(type))
          setShader(type, ShaderDesc{
                            std::nullopt,
                            sources[type],
                          });
        else
          setShader(type, std::nullopt);
      }
    }

    void Program::setFiles(Map<ShaderType, URI> const & files) {
      for (int64_t i = 0; i < ShaderType_Count; ++i) {
        ShaderType type = ShaderType(i);
        if (files.contains(type))
          setShader(type, ShaderDesc{
                            files[type],
                            std::nullopt,
                          });
        else
          setShader(type, std::nullopt);
      }
    }

    void CommandList::setUniform(StringView const & name, Mat4 value) {
      return setUniform(name, &value, sizeof(value));
    }

    void CommandList::setUniform(StringView const & name, float value) {
      return setUniform(name, &value, sizeof(value));
    }

    void CommandList::setUniform(StringView const & name, Vec2 value) {
      return setUniform(name, &value, sizeof(value));
    }

    void CommandList::setUniform(StringView const & name, Vec3 value) {
      return setUniform(name, &value, sizeof(value));
    }

    void CommandList::setUniform(StringView const & name, Vec4 value) {
      return setUniform(name, &value, sizeof(value));
    }

    void CommandList::setUniform(StringView const & name, Vec2i value) {
      return setUniform(name, &value, sizeof(value));
    }

    void CommandList::setUniform(StringView const & name, Vec3i value) {
      return setUniform(name, &value, sizeof(value));
    }

    void CommandList::setUniform(StringView const & name, Vec4i value) {
      return setUniform(name, &value, sizeof(value));
    }

    void CommandList::setUniform(StringView const & name, int32_t value) {
      return setUniform(name, &value, sizeof(value));
    }

    void loadTexture(CommandList * pCmdList, TextureRef * pTexture, TextureType const & type, media::Surface const & surface) {
      if (*pTexture == nullptr || (*pTexture)->getType() != type)
        *pTexture = pCmdList->createTexture(type);
      pCmdList->uploadTexture(*pTexture, surface);
    }

    void loadTexture(CommandList * pCmdList, TextureRef * pTexture, TextureType const & type, Vec3i const & size, PixelFormat const & format,
                     void const * pPixels, int64_t rowPitch) {
      media::Surface surface;
      surface.pBuffer = (void *)pPixels;
      surface.format  = format;
      surface.size    = size;
      surface.pitch   = rowPitch;
      return loadTexture(pCmdList, pTexture, type, surface);
    }

    void loadTexture(CommandList * pCmdList, TextureRef * pTexture, TextureType const & type, Vec3i const & size, DepthStencilFormat const & depthFormat) {
      if (*pTexture == nullptr || (*pTexture)->getType() != type)
        *pTexture = pCmdList->createTexture(type);
      pCmdList->uploadTexture(*pTexture, depthFormat, size);
    }

    void loadTexture2D(CommandList * pCmdList, TextureRef * pTexture, media::Surface const & surface) {
      loadTexture(pCmdList, pTexture, TextureType_2D, surface);
    }

    bool loadTexture2D(CommandList * pCmdList, TextureRef * pTexture, URI const & path) {
      media::Surface surface;
      if (!loadSurface(path, &surface)) {
        return false;
      }
      loadTexture2D(pCmdList, pTexture, surface);
      return true;
    }

    void loadTexture2D(CommandList * pCmdList, TextureRef * pTexture, Vec2i const & size, PixelFormat const & format, void const * pPixels, int64_t rowPitch) {
      loadTexture(pCmdList, pTexture, TextureType_2D, {size, 1}, format, pPixels, rowPitch);
    }

    void loadTexture2D(CommandList * pCmdList, TextureRef * pTexture, Vec2i const & size, DepthStencilFormat const & depthFormat) {
      loadTexture(pCmdList, pTexture, TextureType_2D, {size, 1}, depthFormat);
    }

    bool loadTextureSub2D(CommandList * pCmdList, TextureRef * pTexture, media::Surface const & surface, Vec2i offset) {
      if (*pTexture == nullptr)
        return false;
      pCmdList->uploadTextureSubData(*pTexture, surface, {offset, 0});
      return true;
    }

    void loadTexture2DArray(CommandList * pCmdList, TextureRef * pTexture, media::Surface const & surface) {
      loadTexture(pCmdList, pTexture, TextureType_2DArray, surface);
    }

    void loadTexture2DArray(CommandList * pCmdList, TextureRef * pTexture, Vec3i const & size, PixelFormat const & format, void const * pPixels,
                            int64_t rowPitch) {
      loadTexture(pCmdList, pTexture, TextureType_2DArray, size, format, pPixels, rowPitch);
    }

    void loadTexture2DArray(CommandList * pCmdList, TextureRef * pTexture, Vec3i const & size, DepthStencilFormat const & depthFormat) {
      loadTexture(pCmdList, pTexture, TextureType_2DArray, size, depthFormat);
    }

    void loadTexture3D(CommandList * pCmdList, TextureRef * pTexture, media::Surface const & surface) {
      loadTexture(pCmdList, pTexture, TextureType_3D, surface);
    }

    void loadTexture3D(CommandList * pCmdList, TextureRef * pTexture, Vec3i const & size, PixelFormat const & format, void const * pPixels, int64_t rowPitch) {
      loadTexture(pCmdList, pTexture, TextureType_3D, size, format, pPixels, rowPitch);
    }

    void loadTexture3D(CommandList * pCmdList, TextureRef * pTexture, Vec3i const & size, DepthStencilFormat const & depthFormat) {
      loadTexture(pCmdList, pTexture, TextureType_3D, size, depthFormat);
    }

    void loadTextureCubeMap(CommandList * pCmdList, TextureRef * pTexture, media::Surface const & surface) {
      BFC_ASSERT(surface.size.z == CubeMapFace_Count, "Surface must have a depth of 6");

      loadTexture(pCmdList, pTexture, TextureType_CubeMap, surface);
    }

    void loadTextureCubeMap(CommandList * pCmdList, TextureRef * pTexture, Vec2i const & size, PixelFormat const & format, void const * pPixels,
                            int64_t rowPitch) {
      loadTexture(pCmdList, pTexture, TextureType_CubeMap, {size, CubeMapFace_Count}, format, pPixels, rowPitch);
    }

    void loadTextureCubeMap(CommandList * pCmdList, TextureRef * pTexture, Vec2i const & size, DepthStencilFormat const & depthFormat) {
      loadTexture(pCmdList, pTexture, TextureType_CubeMap, {size, CubeMapFace_Count}, depthFormat);
    }
  } // namespace graphics

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

