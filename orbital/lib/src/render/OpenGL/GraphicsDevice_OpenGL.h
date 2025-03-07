#pragma once

#include "core/Pool.h"
#include "core/String.h"
#include "math/MathTypes.h"
#include "media/Pixel.h"
#include "render/GraphicsDevice.h"

#ifdef BFC_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define GLEW_STATIC
#include "../../../vendor/glew/include/GL/glew.h"
#include "../../../vendor/glew/include/GL/wglew.h"
#endif

namespace bfc {
  namespace graphics {
    namespace impl {
      class CommandBuffer {
      public:
        using GenericCommandCallback = void (*)(uint8_t const * pCommand, GraphicsDevice * pDevice, CommandBuffer * pBuffer);

        template<typename Cmd>
        void add(Cmd const & command) {
          static_assert(std::is_trivially_copyable_v<Cmd>, "Cmd must be trivially copyable");

          m_stream.pushBack((uint8_t *)&cmd, sizeof(command));

          Command cmd;
          cmd.callback = (GenericCommandCallback)&Cmd::execute;
          cmd.size     = sizeof(Cmd);
        }

        void execute(GraphicsDevice * pDevice) {
          for (auto & command : m_commands) {
            command.callback(m_stream.storage().begin() + command.offset, pDevice, this);
          }
        }

        void reset() {
          m_commands.clear();
          m_stream.clear();
        }

        int64_t write(Span<const uint8_t> const & data) {
          if (data.begin() == 0) {
            return -1;
          }

          m_stream.seek(0, SeekOrigin_End);
          int64_t offset = m_stream.tell();
          m_stream.write(data.begin(), data.size());
          return offset;
        }

        Span<const uint8_t> read(int64_t offset, int64_t size) const {
          if (offset == -1)
            return { nullptr, size };
          else
            return {m_stream.storage().begin() + offset, size};
        }

        template<typename T>
        struct Serialized {
          int64_t offset = -1;
        };

        template<typename T>
        Serialized<T> serialize(T const & o) {
          int64_t offset = m_stream.tell();
          m_stream.write(o);
          return offset;
        }

        template<typename T>
        T deserialize(Serialized<T> const & handle) {
          m_stream.seek(handle.offset);
          bfc::Uninitialized<T> buffer;
          m_stream.read(buffer.ptr());
          return buffer.take();
        }

        template<typename T>
        T deserialize(Serialized<T> const & handle, int64_t * pSize) {
          T ret  = deserialize(handle);
          *pSize = m_stream.tell() - handle.offset;
          return ret;
        }

      private:
        struct Command {
          GenericCommandCallback callback;
          uint64_t               offset;
        };

        Vector<Command> m_commands;
        MemoryStream    m_stream;
      };
    } // namespace impl

    class GLBuffer : public Buffer {
    public:
      virtual int64_t getSize() const override;

      uint32_t        glID          = 0;
      int64_t         size          = 0;
      int64_t         allocated     = 0;
      GLenum          defaultTarget = GL_NONE;
      GLenum          glUsage       = GL_STATIC_DRAW;
      BufferUsageHint usageHint     = BufferUsageHint_Unknown;

      GLenum getBindTarget(bool read) const;
    };

    class GLVertexArray : public VertexArray {
    public:
      virtual void              setLayout(VertexInputLayout const & layout) override;
      virtual bool              setVertexBuffer(int64_t slot, BufferRef vertexBufferID) override;
      virtual bool              setIndexBuffer(BufferRef indexBufferID, DataType indexType) override;
      virtual VertexInputLayout getLayout() const override;
      virtual BufferRef         getVertexBuffer(int64_t slot) const override;
      virtual BufferRef         getIndexBuffer() const override;
      virtual DataType          getIndexType() const override;

      uint32_t glID = 0;

      DataType  indexBufferType = DataType_Unknown;
      BufferRef indexBuffer     = InvalidGraphicsResource;

      VertexInputLayout layout;
      BufferRef         vertexBuffers[MaxVertexBuffers];

      bool            rebind = false; // VAO has changed and needs to be rebound
      Vector<int64_t> activeSlots;    // Currently active attrib arrays
    };

    class GLTexture : public Texture {
    public:
      virtual TextureType        getType() const override;
      virtual Vec3i              getSize(int64_t mipLevel) const override;
      virtual bool               isDepthTexture() const override;
      virtual DepthStencilFormat getDepthStencilFormat() const override;
      virtual PixelFormat        getColourFormat() const override;

      TextureType        type            = TextureType_Unknown;
      uint32_t           glID            = 0;
      Vec3i              size            = Vec3i(0);
      PixelFormat        format          = PixelFormat_Unknown;
      DepthStencilFormat depthStencilFmt = DepthStencilFormat_Unknown;
    };

    class GLSampler : public Sampler {
    public:
      virtual void setSamplerMinFilter(FilterMode filter, FilterMode mipFilter) override;
      virtual void setSamplerMagFilter(FilterMode filter, FilterMode mipFilter) override;

      virtual void setSamplerMinLOD(float level) override;
      virtual void setSamplerMaxLOD(float level) override;

      virtual void setSamplerWrapU(WrapMode mode) override;
      virtual void setSamplerWrapV(WrapMode mode) override;
      virtual void setSamplerWrapW(WrapMode mode) override;

      uint32_t glID    = 0;

      bool     changed = false;

      FilterMode minFilter    = FilterMode_Linear;
      FilterMode magFilter    = FilterMode_Linear;
      FilterMode minMipFilter = FilterMode_None;
      FilterMode magMipFilter = FilterMode_None;

      float minLOD = -1000.0f;
      float maxLOD = 1000.0f;

      Vector3<WrapMode> wrapMode = {WrapMode_Repeat, WrapMode_Repeat, WrapMode_Repeat};
    };

    class GLProgram : public Program {
    public:
      struct Attribute {
        String    name;
        DataClass cls;
        DataType  type;
        int64_t   width, height;

        int32_t glLoc = -1;
      };

      struct Uniform {
        String    name;
        DataClass cls;
        DataType  type;
        int64_t   width, height;

        int32_t glLoc = -1;
      };

      struct Buffer {
        String  name;
        int64_t size;

        bool    isStorageBuffer = false;
        int32_t bufferIndex     = -1;
        int32_t bindPoint       = -1;
      };

      struct Texture {
        String      name;
        TextureType type;

        int32_t bindPoint = -1;
        int32_t glLoc     = -1;
      };

      virtual void                      addShader(ShaderType type, std::optional<ShaderDesc> desc) override;
      virtual std::optional<ShaderDesc> getShader(ShaderType type) const override;
      virtual bool                      compile(String * pError = nullptr) override;

      virtual int64_t getAttributeCount() const override;
      virtual int64_t getUniformCount() const override;
      virtual int64_t getBufferCount() const override;
      virtual int64_t getTextureCount() const override;

      virtual void getAttributeDesc(int64_t uniformIndex, ProgramAttributeDesc * pDesc) const override;
      virtual void getUniformDesc(int64_t uniformIndex, ProgramUniformDesc * pDesc) const override;
      virtual void getTextureDesc(int64_t textureIndex, ProgramTextureDesc * pDesc) const override;
      virtual void getBufferDesc(int64_t bufferIndex, ProgramBufferDesc * pDesc) const override;

      static void reflect(uint32_t glID, Vector<Attribute> * pAttributes, Vector<Uniform> * pUniforms, Vector<Texture> * pTextures, Vector<Buffer> * pBuffers);

      uint32_t   glID = 0;
      std::optional<ShaderDesc> shaders[ShaderType_Count];

      Vector<Attribute> attributes;
      Vector<Uniform>   uniforms;
      Vector<Buffer>    buffers;
      Vector<Texture>   textures;
    };

    class GLRenderTarget : public RenderTarget {
    public:
      virtual RenderTargetType getType() const override;
      virtual Vec2i            getSize() const override;
      virtual bool             attachWindow(platform::Window * pWindow, DepthStencilFormat depthStencilFormat) override;
      virtual void             attachColour(TextureRef textureID, int64_t slot, int64_t mipLevel, int64_t layer) override;
      virtual void             setReadAttachment(int64_t slot) override;
      virtual void             attachDepth(TextureRef textureID, int64_t mipLevel, int64_t layer) override;

      struct Attachment {
        int64_t    layer    = 0; // Level/Face for 3D textures
        int64_t    mipLevel = 0;
        TextureRef texture  = InvalidGraphicsResource;
      };

      RenderTargetType type = RenderTargetType_Unknown;

      struct Texture {
        bool       rebind = false;
        uint32_t   glID   = 0;
        Attachment depth;
        Attachment colour[MaxColourAttachments];
        int64_t    colourReadAttachment = 0;
      } textures;

      struct Window {
        HWND               hWnd        = 0;
        HDC                hDC         = 0;
        DepthStencilFormat depthFormat = DepthStencilFormat_Unknown;
      } window;
    };

    class BFC_API StateManager_OpenGL : public StateManager {
    public:
      virtual void apply(State const & state) override;
    };

    class BFC_API GLBufferDownload : public BufferDownload {
    public:
      virtual bool wait(std::optional<Timestamp> const & timeout) override {
        if (timeout.has_value())
          return complete.wait_for(std::chrono::microseconds{(int64_t)timeout->micros()}) == std::future_status::ready;

        complete.wait();
        return true;
      }

      virtual Vector<uint8_t> take() override {
        return std::move(storage);
      }

      virtual Span<const uint8_t> view() const override {
        return storage;
      }

      Vector<uint8_t>   storage;
      std::future<void> complete;
    };

    class BFC_API GLTextureDownload : public TextureDownload {
    public:
      virtual bool wait(std::optional<Timestamp> const & timeout) override {
        if (timeout.has_value())
          return complete.wait_for((std::chrono::microseconds)timeout.value()) == std::future_status::ready;

        complete.wait();
        return true;
      }

      virtual media::Surface view() const override {
      
      }

      virtual std::tuple<media::Surface, Vector<uint8_t>> take() override {
        
      }

      Vector<uint8_t>   storage;
      std::future<bool> complete;
    };

    class BFC_API CommandList_OpenGL : public CommandList {
    public:
      CommandList_OpenGL(GraphicsDevice * pDevice, RenderTargetRef defaultTarget, VertexArrayRef emptyVertexArray, uint32_t lastTextureUnit);

      // Pipeline state
      virtual void bindProgram(ProgramRef programID) override;
      virtual void bindVertexArray(VertexArrayRef vertexArrayID) override;
      virtual void bindTexture(TextureRef textureID, int64_t textureUnit) override;
      virtual void bindSampler(SamplerRef samplerID, int64_t textureUnit) override;
      virtual void bindUniformBuffer(BufferRef bufferID, int64_t bindPoint, int64_t offset = 0, int64_t size = 0) override;
      virtual void bindShaderStorageBuffer(BufferRef bufferID, int64_t bindPoint, int64_t offset = 0, int64_t size = 0) override;
      virtual void bindRenderTarget(RenderTargetRef renderTargetID, MapAccess renderTargetAccess = MapAccess_ReadWrite) override;
      virtual void bindScreen(MapAccess renderTargetAccess = MapAccess_ReadWrite) override;

      virtual void setState(Span<const State> const & state) override;
      virtual void pushState(Span<const State> const & state) override;
      virtual void popState() override;

      // Buffers
      virtual bool upload(BufferRef bufferID, int64_t size, void const * pData = nullptr) override;

      /// Map a buffer to client memory.
      /// @param bufferID The buffer to map.
      /// @param access   Type of access needed (read, write, read/write).
      /// @returns A pointer to the mapped buffer.
      virtual std::future<void*> map(BufferRef bufferID, MapAccess access = MapAccess_ReadWrite) override;

      /// Map part of a buffer to client memory.
      /// @param bufferID The buffer to map.
      /// @param offset   The offset into the buffer to map (in bytes).
      /// @param size     The number of bytes to map, starting at `offset`.
      /// @param access   Type of access needed (read, write, read/write).
      /// @returns A pointer to the mapped buffer.
      virtual std::future<void*> map(BufferRef bufferID, int64_t offset, int64_t size, MapAccess access = MapAccess_ReadWrite) override;

      virtual void unmap(BufferRef bufferID) override;

      virtual void download(BufferRef bufferID, BufferDownloadRef pDownload, int64_t offset, int64_t size) override;

      // Textures
      virtual bool uploadTexture(TextureRef textureID, DepthStencilFormat format, Vec3i size) override;
      virtual bool uploadTexture(TextureRef textureID, media::Surface const & src) override;
      virtual bool uploadTextureSubData(TextureRef textureID, media::Surface const & src, Vec3i offset) override;
      virtual void generateMipMaps(TextureRef textureID) override;
      virtual bool downloadTexture(TextureRef textureID, TextureDownloadRef pDownload) override;

      // Shaders
      virtual void    setUniform(int64_t uniformIndex, void const * pBuffer, int64_t size) override;
      virtual void    setBufferBinding(int64_t bufferIndex, int64_t bindPoint) override;
      virtual void    setTextureBinding(int64_t textureIndex, int64_t bindPoint) override;
      // virtual void    getUniform(int64_t uniformIndex, void * pBuffer, ProgramUniformDesc * pDesc) override;
      // virtual int64_t getBufferBinding(int64_t bufferIndex) override;
      // virtual int64_t getTextureBinding(int64_t bufferIndex) override;

      // Rendering commands
      virtual void clear(RGBAu8 colour) override;
      virtual void swap() override;

      virtual void draw(int64_t elementCount = std::numeric_limits<int64_t>::max(), int64_t elementOffset = 0, PrimitiveType primType = PrimitiveType_Triangle,
                        int64_t instanceCount = 1) override;
      virtual void drawIndexed(int64_t elementCount = std::numeric_limits<int64_t>::max(), int64_t elementOffset = 0,
                               PrimitiveType primType = PrimitiveType_Triangle, int64_t instanceCount = 1) override;

      /// Get the graphics device that created this command list.
      virtual GraphicsDevice * getDevice() const override;
    private:
      template<typename Cmd>
      void add(Cmd const & cmd) {
        m_commandBuffer.Add(cmd);
      }

      void track(Ref<void> const & resource) {
        if (resource != nullptr) {
          m_trackedResources.pushBack(resource);
        }
      }

      void clear() {
        m_trackedResources.clear();
        m_commandBuffer.reset();
      }

      GraphicsDevice * m_pDevice;

      int64_t  m_indexCount  = -1;
      int64_t  m_vertexCount = -1;
      DataType m_vaIndexType = DataType_Unknown;

      RenderTargetRef m_activeWindowTarget = InvalidGraphicsResource;
      RenderTargetRef m_defaultTarget      = InvalidGraphicsResource;

      VertexArrayRef m_emptyVertexArray = nullptr;
      uint32_t       m_lastTextureUnit  = 0;

      ProgramRef m_boundProgram = nullptr;

      impl::CommandBuffer m_commandBuffer;
      Vector<Ref<void>>   m_trackedResources;
      GraphicsDevice *    m_pDevice;
    };
  } // namespace graphics

  class GraphicsDevice_OpenGL : public GraphicsDevice {
  public:
    GraphicsDevice_OpenGL();

    virtual bool init(platform::Window * pWindow) override;
    virtual void destroy() override;

    virtual std::unique_ptr<graphics::CommandList> createCommandList() override;
    virtual graphics::BufferRef       createBuffer(BufferUsageHint usageHint) override;
    virtual graphics::VertexArrayRef  createVertexArray() override;
    virtual graphics::ProgramRef      createProgram() override;
    virtual graphics::TextureRef      createTexture(TextureType type) override;
    virtual graphics::SamplerRef      createSampler() override;
    virtual graphics::RenderTargetRef createRenderTarget(RenderTargetType type) override;

    virtual graphics::StateManager * getStateManager() override;

    virtual uint64_t submit(std::unique_ptr<graphics::CommandList> const & pCommandList) override;

    virtual bool wait(uint64_t handle, std::optional<Timestamp> const & timeout = std::nullopt) override;

    const graphics::VertexArrayRef getEmptyVertexArray() const {
      return m_emptyVertexArray;
    }

    const graphics::RenderTargetRef getDefaultRenderTarget() const {
      return m_defaultTarget;
    }

    const uint32_t getReservedTextureUnit() const {
      return m_lastTextureUnit;
    }

    const HGLRC getGLRC() const {
      return m_hGLRC;
    }

  private:
    void RenderThread();

    std::thread m_renderThread;
    bool        m_running = false;

    uint64_t                m_nextCommandListID = 0;
    uint64_t                m_commandListFence  = 0;
    std::mutex              m_fenceLock;
    std::condition_variable m_fenceNotifier;

    std::mutex                                     m_queueLock;
    std::condition_variable                        m_queueNotifier;
    Vector<std::unique_ptr<graphics::CommandList>> m_commandListQueue;

    graphics::StateManager_OpenGL m_stateManager;

    HGLRC                     m_hGLRC            = 0;
    graphics::RenderTargetRef m_defaultTarget    = InvalidGraphicsResource;
    graphics::VertexArrayRef  m_emptyVertexArray = InvalidGraphicsResource0;

    uint32_t m_lastTextureUnit = 0;
  };
} // namespace bfc
