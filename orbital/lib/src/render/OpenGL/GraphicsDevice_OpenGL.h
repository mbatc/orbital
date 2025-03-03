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
  namespace impl {
    class CommandBuffer {
    public:
      using GenericCommandCallback = void (*)(uint8_t * pCommand, CommandBuffer * pBuffer);

      template<typename Cmd>
      void Add(Cmd const & command) {
        static_assert(std::is_trivially_copyable_v<Cmd>, "Cmd must be trivially copyable");

        data.pushBack((uint8_t *)&cmd, sizeof(command));

        Command cmd;
        cmd.callback = (GenericCommandCallback)&Cmd::Execute;
        cmd.size     = sizeof(Cmd);
      }

      void Execute() {
        uint64_t dataOffset = 0;
        for (auto& command : m_commands) {
          command.callback(m_data.begin() + dataOffset, this);
          dataOffset += command.size;
        }
      }

      void Reset() {
        m_commands.clear();
        m_data.clear();
      }

    private:
      struct Command {
        GenericCommandCallback callback;
        uint16_t size;
      };

      Vector<Command> m_commands;
      Vector<uint8_t> m_data;
    };
  }

  namespace graphics {
    class BufferManager_OpenGL : public BufferManager {
    public:
      struct GLBuffer : public GraphicsResource {
        GLBuffer() : GraphicsResource(GraphicsResourceType_Buffer) {}

        uint32_t        glID          = 0;
        int64_t         size          = 0;
        int64_t         allocated     = 0;
        GLenum          defaultTarget = GL_NONE;
        GLenum          glUsage       = GL_STATIC_DRAW;
        BufferUsageHint usageHint     = BufferUsageHint_Unknown;

        GLenum getBindTarget(bool read) const;
      };

      struct GLVertexArray : public GraphicsResource {
        GLVertexArray()
          : GraphicsResource(GraphicsResourceType_VertexArray) {}

        uint32_t glID = 0;

        DataType         indexBufferType = DataType_Unknown;
        GraphicsResourceRef indexBuffer     = InvalidGraphicsResource;

        VertexInputLayout layout;
        GraphicsResourceRef vertexBuffers[MaxVertexBuffers];

        bool            rebind = false; // VAO has changed and needs to be rebound
        Vector<int64_t> activeSlots;    // Currently active attrib arrays
      };

      virtual GraphicsResourceRef createBuffer(BufferUsageHint usageHint = BufferUsageHint_Unknown) override;
      virtual int64_t             getBufferReferences(GraphicsResourceRef bufferID) override;
      virtual int64_t             getSize(GraphicsResourceRef bufferID) override;

      virtual GraphicsResourceRef createVertexArray() override;
      virtual void                setLayout(GraphicsResourceRef vaID, VertexInputLayout const & layout) override;
      virtual bool                setVertexBuffer(GraphicsResourceRef vaID, int64_t slot, GraphicsResourceRef vertexBufferID) override;
      virtual bool                setIndexBuffer(GraphicsResourceRef vaID, GraphicsResourceRef indexBufferID, DataType indexType) override;
      virtual VertexInputLayout   getLayout(GraphicsResourceRef vaID) override;
      virtual GraphicsResourceRef getVertexBuffer(GraphicsResourceRef vaID, int64_t slot) override;
      virtual GraphicsResourceRef getIndexBuffer(GraphicsResourceRef vaID) override;
      virtual DataType            getIndexType(GraphicsResourceRef vaID) override;

      GLBuffer &      getBuffer(GraphicsResourceRef bufferID);
      GLVertexArray & getVertexArray(GraphicsResourceRef bufferID);

      RefPool<GLBuffer>      buffers;
      RefPool<GLVertexArray> vertexArrays;
    };

    class TextureManager_OpenGL : public TextureManager {
    public:
      struct GLTexture : public GraphicsResource {
        GLTexture()
          : GraphicsResource(GraphicsResourceType_Texture) {}

        TextureType        type            = TextureType_Unknown;
        uint32_t           glID            = 0;
        Vec3i              size            = Vec3i(0);
        PixelFormat        format          = PixelFormat_Unknown;
        DepthStencilFormat depthStencilFmt = DepthStencilFormat_Unknown;
      };

      struct GLSampler : public GraphicsResource {
        GLSampler()
          : GraphicsResource(GraphicsResourceType_Sampler) {}

        uint32_t glID = 0;

        FilterMode minFilter    = FilterMode_Linear;
        FilterMode magFilter    = FilterMode_Linear;
        FilterMode minMipFilter = FilterMode_None;
        FilterMode magMipFilter = FilterMode_None;

        float minLOD = -1000.0f;
        float maxLOD = 1000.0f;

        Vector3<WrapMode> wrapMode = { WrapMode_Repeat, WrapMode_Repeat, WrapMode_Repeat };
      };

      virtual GraphicsResourceRef   createTexture(TextureType type) override;
      virtual TextureType        getType(GraphicsResourceRef textureID) override;
      virtual Vec3i              getSize(GraphicsResourceRef textureID, int64_t mipLevel) override;
      virtual bool               isDepthTexture(GraphicsResourceRef textureID) override;
      virtual DepthStencilFormat getDepthStencilFormat(GraphicsResourceRef textureID) override;
      virtual PixelFormat        getColourFormat(GraphicsResourceRef textureID) override;

      virtual GraphicsResourceRef createSampler() override;

      virtual void setSamplerMinFilter(GraphicsResourceRef samplerID, FilterMode filter, FilterMode mipFilter) override;
      virtual void setSamplerMagFilter(GraphicsResourceRef samplerID, FilterMode filter, FilterMode mipFilter) override;

      virtual void setSamplerMinLOD(GraphicsResourceRef samplerID, float level) override;
      virtual void setSamplerMaxLOD(GraphicsResourceRef samplerID, float level) override;

      virtual void setSamplerWrapU(GraphicsResourceRef samplerID, WrapMode mode) override;
      virtual void setSamplerWrapV(GraphicsResourceRef samplerID, WrapMode mode) override;
      virtual void setSamplerWrapW(GraphicsResourceRef samplerID, WrapMode mode) override;

      GLTexture & getTexture(GraphicsResourceRef textureID);
      GLSampler & getSampler(GraphicsResourceRef samplerID);

      RefPool<GLTexture> textures;
      RefPool<GLSampler> samplers;
    };

    class ShaderManager_OpenGL : public ShaderManager {
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

      struct GLProgram : public GraphicsResource {
        GLProgram()
          : GraphicsResource(GraphicsResourceType_Program) {}

        uint32_t         glID = 0;
        GraphicsResourceRef shaders[ShaderType_Count];

        Vector<Attribute> attributes;
        Vector<Uniform>   uniforms;
        Vector<Buffer>    buffers;
        Vector<Texture>   textures;
      };

      struct GLShader : public GraphicsResource {
        GLShader()
          : GraphicsResource(GraphicsResourceType_Shader) {}

        ShaderType type   = ShaderType_Unknown;
        uint32_t   glID   = 0;
        String     source = ""; // Shader source
        String     file   = ""; // Shader file
      };

      virtual GraphicsResourceRef createShader(ShaderType type) override;
      virtual ShaderType       getType(GraphicsResourceRef shaderID) override;
      virtual void             setSource(GraphicsResourceRef shaderID, StringView src) override;
      virtual void             setFile(GraphicsResourceRef shaderID, StringView path) override;
      virtual bool             compile(GraphicsResourceRef shaderID, String * pError) override;

      virtual GraphicsResourceRef createProgram() override;
      virtual void             addShader(GraphicsResourceRef programID, GraphicsResourceRef shaderID) override;
      virtual GraphicsResourceRef getShader(GraphicsResourceRef programID, ShaderType shaderID) override;
      virtual bool             linkProgram(GraphicsResourceRef programID, String * pError) override;

      virtual void    setUniform(GraphicsResourceRef programID, int64_t uniformIndex, void const * pBuffer) override;
      virtual void    setBufferBinding(GraphicsResourceRef programID, int64_t bufferIndex, int64_t bindPoint) override;
      virtual void    setTextureBinding(GraphicsResourceRef programID, int64_t textureIndex, int64_t bindPoint) override;
      virtual void    getUniform(GraphicsResourceRef programID, int64_t uniformIndex, void * pBuffer, ProgramUniformDesc * pDesc) override;
      virtual int64_t getBufferBinding(GraphicsResourceRef programID, int64_t bufferIndex) override;
      virtual int64_t getTextureBinding(GraphicsResourceRef programID, int64_t bufferIndex) override;

      virtual int64_t getAttributeCount(GraphicsResourceRef programID) override;
      virtual int64_t getUniformCount(GraphicsResourceRef programID) override;
      virtual int64_t getBufferCount(GraphicsResourceRef programID) override;
      virtual int64_t getTextureCount(GraphicsResourceRef programID) override;
      virtual void    getAttributeDesc(GraphicsResourceRef programID, int64_t uniformIndex, ProgramAttributeDesc * pDesc) override;
      virtual void    getUniformDesc(GraphicsResourceRef programID, int64_t uniformIndex, ProgramUniformDesc * pDesc) override;
      virtual void    getTextureDesc(GraphicsResourceRef programID, int64_t textureIndex, ProgramTextureDesc * pDesc) override;
      virtual void    getBufferDesc(GraphicsResourceRef programID, int64_t bufferIndex, ProgramBufferDesc * pDesc) override;

      void reflect(uint32_t glID, Vector<Attribute> * pAttributes, Vector<Uniform> * pUniforms, Vector<Texture> * pTextures, Vector<Buffer> * pBuffers);

      GLShader &  getShader(GraphicsResourceRef shaderID);
      GLProgram & getProgram(GraphicsResourceRef programID);

      RefPool<GLShader>  shaders;
      RefPool<GLProgram> programs;
    };

    class BFC_API RenderTargetManager_OpenGL : public RenderTargetManager {
    public:
      RenderTargetManager_OpenGL(TextureManager * pTextures);

      struct Attachment {
        int64_t          layer    = 0; // Level/Face for 3D textures
        int64_t          mipLevel = 0;
        GraphicsResourceRef texture  = InvalidGraphicsResource;
      };

      struct GLRenderTarget : public GraphicsResource {
        GLRenderTarget()
          : GraphicsResource(GraphicsResourceType_RenderTarget) {}

        RenderTargetType type = RenderTargetType_Unknown;

        struct {
          bool       rebind = false;
          uint32_t   glID   = 0;
          Attachment depth;
          Attachment colour[MaxColourAttachments];
          int64_t    colourReadAttachment = 0;
        } textures;

        struct {
          HWND               hWnd        = 0;
          HDC                hDC         = 0;
          DepthStencilFormat depthFormat = DepthStencilFormat_Unknown;
        } window;
      };

      virtual GraphicsResourceRef createRenderTarget(RenderTargetType type) override;

      virtual RenderTargetType getType(GraphicsResourceRef renderTargetID) override;
      virtual Vec2i            getSize(GraphicsResourceRef renderTargetID) override;
      virtual bool             attachWindow(GraphicsResourceRef renderTargetID, platform::Window * pWindow, DepthStencilFormat depthStencilFormat) override;
      virtual void attachColour(GraphicsResourceRef renderTargetID, GraphicsResourceRef textureID, int64_t slot, int64_t mipLevel, int64_t layer) override;
      virtual void setReadAttachment(GraphicsResourceRef renderTargetID, int64_t slot) override;
      virtual void attachDepth(GraphicsResourceRef renderTargetID, GraphicsResourceRef textureID, int64_t mipLevel, int64_t layer) override;

      GLRenderTarget & getRenderTarget(GraphicsResourceRef renderTargetID);

      TextureManager *        m_pTextures = nullptr;
      RefPool<GLRenderTarget> renderTargets;
    };

    class BFC_API StateManager_OpenGL : public StateManager {
    public:
      virtual void setFeatureEnabled(GraphicsState state, bool enabled) override;
      virtual void setViewport(Vec2i position, Vec2i size) override;
      virtual void setScissor(Vec2i position, Vec2i size) override;
      virtual void setDepthRange(float min, float max) override;
      virtual void setDepthFunction(ComparisonFunction function) override;
      virtual void setBlendEquation(BlendEquation colourAndAlpha, int64_t colourAttachment) override;
      virtual void setBlendEquation(BlendEquation colour, BlendEquation alpha, int64_t colourAttachment) override;
      virtual void setBlendFunction(BlendFunction sourceFactor, BlendFunction destFactor, int64_t colourAttachment) override;
      virtual void setBlendFunction(BlendFunction sourceColourFactor, BlendFunction sourceAlphaFactor, BlendFunction destColourFactor,
                                    BlendFunction destAlphaFactor, int64_t colourAttachment) override;
      virtual void setColourWriteEnabled(bool red, bool green, bool blue, bool alpha) override;
      virtual void setColourFactor(float red, float green, float blue, float alpha) override;
    };

    class BFC_API CommandList_OpenGL : public CommandList {
    public:
      // Pipeline state
      virtual void bindProgram(GraphicsResourceRef programID) override;
      virtual void bindVertexArray(GraphicsResourceRef vertexArrayID)                                                             override;
      virtual void bindTexture(GraphicsResourceRef textureID, int64_t textureUnit)                                                override;
      virtual void bindSampler(GraphicsResourceRef samplerID, int64_t textureUnit)                                                override;
      virtual void bindUniformBuffer(GraphicsResourceRef bufferID, int64_t bindPoint, int64_t offset = 0, int64_t size = 0)       override;
      virtual void bindShaderStorageBuffer(GraphicsResourceRef bufferID, int64_t bindPoint, int64_t offset = 0, int64_t size = 0) override;
      virtual void bindRenderTarget(GraphicsResourceRef renderTargetID, MapAccess renderTargetAccess = MapAccess_ReadWrite)       override;
      virtual void bindScreen(MapAccess renderTargetAccess = MapAccess_ReadWrite)                                              override;

      virtual void setState() override;
      virtual void pushState() override;
      virtual void popState()  override;

      // Buffers
      virtual bool upload(GraphicsResourceRef bufferID, int64_t size, void const * pData = nullptr) override;

      /// Map a buffer to client memory.
      /// @param bufferID The buffer to map.
      /// @param access   Type of access needed (read, write, read/write).
      /// @returns A pointer to the mapped buffer.
      virtual void * map(GraphicsResourceRef bufferID, MapAccess access = MapAccess_ReadWrite) override;

      /// Map part of a buffer to client memory.
      /// @param bufferID The buffer to map.
      /// @param offset   The offset into the buffer to map (in bytes).
      /// @param size     The number of bytes to map, starting at `offset`.
      /// @param access   Type of access needed (read, write, read/write).
      /// @returns A pointer to the mapped buffer.
      virtual void * map(GraphicsResourceRef bufferID, int64_t offset, int64_t size, MapAccess access = MapAccess_ReadWrite) override;

      virtual void unmap(GraphicsResourceRef bufferID) override;

      virtual int64_t download(GraphicsResourceRef bufferID, void * pDst, int64_t offset = 0, int64_t size = 0) override;

      // Textures
      virtual bool uploadTexture(GraphicsResourceRef textureID, DepthStencilFormat format, Vec3i size)           override;
      virtual bool uploadTexture(GraphicsResourceRef textureID, media::Surface const & src)                      override;
      virtual bool uploadTextureSubData(GraphicsResourceRef textureID, media::Surface const & src, Vec3i offset) override;
      virtual void generateMipMaps(GraphicsResourceRef textureID)                                                override;
      virtual bool downloadTexture(GraphicsResourceRef textureID, media::Surface * pDest)                        override;

      // Rendering commands
      virtual void clear(RGBAu8 colour) override;
      virtual void swap()               override;

      virtual void draw(int64_t elementCount = std::numeric_limits<int64_t>::max(), int64_t elementOffset = 0, PrimitiveType primType = PrimitiveType_Triangle,
                        int64_t instanceCount = 1) override;
      virtual void drawIndexed(int64_t elementCount = std::numeric_limits<int64_t>::max(), int64_t elementOffset = 0,
                               PrimitiveType primType = PrimitiveType_Triangle, int64_t instanceCount = 1) override;

    private:
      template<typename Cmd>
      void Add(Cmd const & cmd) {
        m_commandBuffer.Add(cmd);
      }

      void Track(GraphicsResourceRef const & resource) {
        m_trackedResources.pushBack(resource);
      }

      void Clear()
      {
        m_trackedResources.clear();
        m_commandBuffer.Reset();
      }

      int64_t  m_indexCount  = -1;
      int64_t  m_vertexCount = -1;
      DataType m_vaIndexType = DataType_Unknown;

      int64_t m_lastTextureUnit = 0;

      GraphicsResourceRef m_activeTarget       = InvalidGraphicsResource;
      GraphicsResourceRef m_activeWindowTarget = InvalidGraphicsResource;

      impl::CommandBuffer         m_commandBuffer;
      Vector<GraphicsResourceRef> m_trackedResources;
      GraphicsDevice *            m_pDevice;
    };
  } // namespace graphics

  class GraphicsDevice_OpenGL : public GraphicsDevice {
  public:
    GraphicsDevice_OpenGL();

    virtual bool init(platform::Window * pWindow) override;
    virtual void destroy() override;

    virtual graphics::BufferManager *       getBufferManager() override;
    virtual graphics::TextureManager *      getTextureManager() override;
    virtual graphics::ShaderManager *       getShaderManager() override;
    virtual graphics::RenderTargetManager * getRenderTargetManager() override;
    virtual graphics::StateManager *        getStateManager() override;

  private:
    graphics::BufferManager_OpenGL       m_buffers;
    graphics::TextureManager_OpenGL      m_textures;
    graphics::ShaderManager_OpenGL       m_shaders;
    graphics::RenderTargetManager_OpenGL m_renderTargets;
    graphics::StateManager_OpenGL        m_stateManager;

    HGLRC               m_hGLRC            = 0;
    GraphicsResourceRef m_defaultTarget    = InvalidGraphicsResource;
    uint32_t            m_emptyVertexArray = 0;
  };
} // namespace bfc
