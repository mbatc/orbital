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
    class BufferManager_OpenGL : public BufferManager {
    public:
      struct GLBuffer {
        uint32_t        glID          = 0;
        int64_t         size          = 0;
        int64_t         allocated     = 0;
        GLenum          defaultTarget = GL_NONE;
        GLenum          glUsage       = GL_STATIC_DRAW;
        BufferUsageHint usageHint     = BufferUsageHint_Unknown;

        GLenum getBindTarget(bool read) const;
      };

      struct GLVertexArray {
        uint32_t glID = 0;

        DataType         indexBufferType = DataType_Unknown;
        GraphicsResource indexBuffer     = InvalidGraphicsResource;

        VertexInputLayout layout;
        GraphicsResource  vertexBuffers[MaxVertexBuffers];

        bool            rebind = false; // VAO has changed and needs to be rebound
        Vector<int64_t> activeSlots;    // Currently active attrib arrays
      };

      virtual GraphicsResource createBuffer(BufferUsageHint usageHint = BufferUsageHint_Unknown) override;
      virtual GraphicsResource refBuffer(GraphicsResource bufferID) override;
      virtual void             releaseBuffer(GraphicsResource * pResource) override;
      virtual int64_t          getBufferReferences(GraphicsResource bufferID) override;
      virtual int64_t          getSize(GraphicsResource bufferID) override;
      virtual void *           map(GraphicsResource bufferID, MapAccess access) override;
      virtual void *           map(GraphicsResource bufferID, int64_t offset, int64_t size, MapAccess access) override;
      virtual void             unmap(GraphicsResource bufferID) override;
      virtual bool             upload(GraphicsResource bufferID, int64_t size, void const * pData = nullptr) override;
      virtual int64_t          download(GraphicsResource bufferID, void * pDst, int64_t offset = 0, int64_t size = 0) override;

      virtual GraphicsResource  createVertexArray() override;
      virtual GraphicsResource  refVertexArray(GraphicsResource vaID) override;
      virtual void              releaseVertexArray(GraphicsResource * pResource) override;
      virtual int64_t           getVertexArrayReferences(GraphicsResource vaID) override;
      virtual void              setLayout(GraphicsResource vaID, VertexInputLayout const & layout) override;
      virtual bool              setVertexBuffer(GraphicsResource vaID, int64_t slot, GraphicsResource vertexBufferID) override;
      virtual bool              setIndexBuffer(GraphicsResource vaID, GraphicsResource indexBufferID, DataType indexType) override;
      virtual VertexInputLayout getLayout(GraphicsResource vaID) override;
      virtual GraphicsResource  getVertexBuffer(GraphicsResource vaID, int64_t slot) override;
      virtual GraphicsResource  getIndexBuffer(GraphicsResource vaID) override;
      virtual DataType          getIndexType(GraphicsResource vaID) override;

      GLBuffer &      getBuffer(GraphicsResource bufferID);
      GLVertexArray & getVertexArray(GraphicsResource bufferID);

      RefPool<GLBuffer>      buffers;
      RefPool<GLVertexArray> vertexArrays;
    };

    class TextureManager_OpenGL : public TextureManager {
    public:
      struct GLTexture {
        TextureType        type            = TextureType_Unknown;
        uint32_t           glID            = 0;
        Vec3i              size            = Vec3i(0);
        PixelFormat        format          = PixelFormat_Unknown;
        DepthStencilFormat depthStencilFmt = DepthStencilFormat_Unknown;
      };

      struct GLSampler {
        uint32_t glID = 0;

        FilterMode minFilter    = FilterMode_Linear;
        FilterMode magFilter    = FilterMode_Linear;
        FilterMode minMipFilter = FilterMode_None;
        FilterMode magMipFilter = FilterMode_None;

        float minLOD = -1000.0f;
        float maxLOD = 1000.0f;

        Vector3<WrapMode> wrapMode = { WrapMode_Repeat, WrapMode_Repeat, WrapMode_Repeat };
      };

      virtual GraphicsResource   createTexture(TextureType type) override;
      virtual GraphicsResource   refTexture(GraphicsResource textureID) override;
      virtual void               releaseTexture(GraphicsResource * pResource) override;
      virtual int64_t            getTextureReferences(GraphicsResource textureID) override;
      virtual bool               upload(GraphicsResource textureID, DepthStencilFormat format, Vec3i size) override;
      virtual bool               upload(GraphicsResource textureID, media::Surface const & src) override;
      virtual bool               uploadSubData(GraphicsResource textureID, media::Surface const & src, Vec3i offset) override;
      virtual void               generateMipMaps(GraphicsResource textureID) override;
      virtual bool               download(GraphicsResource textureID, media::Surface * pDest) override;
      virtual TextureType        getType(GraphicsResource textureID) override;
      virtual Vec3i              getSize(GraphicsResource textureID, int64_t mipLevel) override;
      virtual bool               isDepthTexture(GraphicsResource textureID) override;
      virtual DepthStencilFormat getDepthStencilFormat(GraphicsResource textureID) override;
      virtual PixelFormat        getColourFormat(GraphicsResource textureID) override;

      virtual GraphicsResource createSampler() override;
      virtual GraphicsResource refSampler(GraphicsResource samplerID) override;
      virtual void             releaseSampler(GraphicsResource * pResource) override;
      virtual int64_t          getSamplerReferences(GraphicsResource samplerID) override;

      virtual void setSamplerMinFilter(GraphicsResource samplerID, FilterMode filter, FilterMode mipFilter) override;
      virtual void setSamplerMagFilter(GraphicsResource samplerID, FilterMode filter, FilterMode mipFilter) override;

      virtual void setSamplerMinLOD(GraphicsResource samplerID, float level) override;
      virtual void setSamplerMaxLOD(GraphicsResource samplerID, float level) override;

      virtual void setSamplerWrapU(GraphicsResource samplerID, WrapMode mode) override;
      virtual void setSamplerWrapV(GraphicsResource samplerID, WrapMode mode) override;
      virtual void setSamplerWrapW(GraphicsResource samplerID, WrapMode mode) override;

      GLTexture & getTexture(GraphicsResource textureID);
      GLSampler & getSampler(GraphicsResource samplerID);

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

      struct GLProgram {
        uint32_t         glID = 0;
        GraphicsResource shaders[ShaderType_Count];

        Vector<Attribute> attributes;
        Vector<Uniform>   uniforms;
        Vector<Buffer>    buffers;
        Vector<Texture>   textures;
      };

      struct GLShader {
        ShaderType type   = ShaderType_Unknown;
        uint32_t   glID   = 0;
        String     source = ""; // Shader source
        String     file   = ""; // Shader file
      };

      virtual GraphicsResource createShader(ShaderType type) override;
      virtual GraphicsResource refShader(GraphicsResource shaderID) override;
      virtual void             releaseShader(GraphicsResource * pResource) override;
      virtual int64_t          getShaderReferences(GraphicsResource shaderID) override;
      virtual ShaderType       getType(GraphicsResource shaderID) override;
      virtual void             setSource(GraphicsResource shaderID, StringView src) override;
      virtual void             setFile(GraphicsResource shaderID, StringView path) override;
      virtual bool             compile(GraphicsResource shaderID, String * pError) override;

      virtual GraphicsResource createProgram() override;
      virtual GraphicsResource refProgram(GraphicsResource programID) override;
      virtual void             releaseProgram(GraphicsResource * pResource) override;
      virtual int64_t          getProgramReferences(GraphicsResource programID) override;
      virtual void             addShader(GraphicsResource programID, GraphicsResource shaderID) override;
      virtual GraphicsResource getShader(GraphicsResource programID, ShaderType shaderID) override;
      virtual bool             linkProgram(GraphicsResource programID, String * pError) override;

      virtual void    setUniform(GraphicsResource programID, int64_t uniformIndex, void const * pBuffer) override;
      virtual void    setBufferBinding(GraphicsResource programID, int64_t bufferIndex, int64_t bindPoint) override;
      virtual void    setTextureBinding(GraphicsResource programID, int64_t textureIndex, int64_t bindPoint) override;
      virtual void    getUniform(GraphicsResource programID, int64_t uniformIndex, void * pBuffer, ProgramUniformDesc * pDesc) override;
      virtual int64_t getBufferBinding(GraphicsResource programID, int64_t bufferIndex) override;
      virtual int64_t getTextureBinding(GraphicsResource programID, int64_t bufferIndex) override;

      virtual int64_t getAttributeCount(GraphicsResource programID) override;
      virtual int64_t getUniformCount(GraphicsResource programID) override;
      virtual int64_t getBufferCount(GraphicsResource programID) override;
      virtual int64_t getTextureCount(GraphicsResource programID) override;
      virtual void    getAttributeDesc(GraphicsResource programID, int64_t uniformIndex, ProgramAttributeDesc * pDesc) override;
      virtual void    getUniformDesc(GraphicsResource programID, int64_t uniformIndex, ProgramUniformDesc * pDesc) override;
      virtual void    getTextureDesc(GraphicsResource programID, int64_t textureIndex, ProgramTextureDesc * pDesc) override;
      virtual void    getBufferDesc(GraphicsResource programID, int64_t bufferIndex, ProgramBufferDesc * pDesc) override;

      void reflect(uint32_t glID, Vector<Attribute> * pAttributes, Vector<Uniform> * pUniforms, Vector<Texture> * pTextures, Vector<Buffer> * pBuffers);

      GLShader &  getShader(GraphicsResource shaderID);
      GLProgram & getProgram(GraphicsResource programID);

      RefPool<GLShader>  shaders;
      RefPool<GLProgram> programs;
    };

    class BFC_API RenderTargetManager_OpenGL : public RenderTargetManager {
    public:
      RenderTargetManager_OpenGL(TextureManager * pTextures);

      struct Attachment {
        int64_t          layer    = 0; // Level/Face for 3D textures
        int64_t          mipLevel = 0;
        GraphicsResource texture  = InvalidGraphicsResource;
      };

      struct GLRenderTarget {
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

      virtual GraphicsResource createRenderTarget(RenderTargetType type) override;
      virtual GraphicsResource refRenderTarget(GraphicsResource renderTargetID) override;
      virtual void             releaseRenderTarget(GraphicsResource * pRenderTargetID) override;
      virtual int64_t          getRenderTargetReferences(GraphicsResource renderTargetID) override;

      virtual RenderTargetType getType(GraphicsResource renderTargetID) override;
      virtual Vec2i            getSize(GraphicsResource renderTargetID) override;
      virtual bool             attachWindow(GraphicsResource renderTargetID, platform::Window * pWindow, DepthStencilFormat depthStencilFormat) override;
      virtual void attachColour(GraphicsResource renderTargetID, GraphicsResource textureID, int64_t slot, int64_t mipLevel, int64_t layer) override;
      virtual void setReadAttachment(GraphicsResource renderTargetID, int64_t slot) override;
      virtual void attachDepth(GraphicsResource renderTargetID, GraphicsResource textureID, int64_t mipLevel, int64_t layer) override;

      GLRenderTarget & getRenderTarget(GraphicsResource renderTargetID);

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
  } // namespace graphics

  class GraphicsDevice_OpenGL : public GraphicsDevice {
  public:
    GraphicsDevice_OpenGL();

    virtual bool init(platform::Window * pWindow) override;
    virtual void destroy() override;

    virtual void clear(RGBAu8 colour) override;
    virtual void swap() override;

    virtual void bindProgram(GraphicsResource programID) override;
    virtual void bindVertexArray(GraphicsResource vertexArrayID) override;
    virtual void bindTexture(GraphicsResource textureID, int64_t textureUnit) override;
    virtual void bindSampler(GraphicsResource samplerID, int64_t textureUnit) override;
    virtual void bindUniformBuffer(GraphicsResource bufferID, int64_t bindPoint, int64_t offset, int64_t size) override;
    virtual void bindShaderStorageBuffer(GraphicsResource bufferID, int64_t bindPoint, int64_t offset, int64_t size) override;
    virtual void bindRenderTarget(GraphicsResource renderTargetID, MapAccess renderTargetAccess) override;
    virtual void bindScreen(MapAccess renderTargetAccess) override;

    virtual void draw(int64_t elementCount, int64_t elementOffset, PrimitiveType primType, int64_t instanceCount) override;
    virtual void drawIndexed(int64_t elementCount, int64_t elementOffset, PrimitiveType primType, int64_t instanceCount) override;

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

    HGLRC    m_hGLRC       = 0;
    int64_t  m_indexCount  = -1;
    int64_t  m_vertexCount = -1;
    DataType m_vaIndexType = DataType_Unknown;

    int64_t m_lastTextureUnit = 0;

    GraphicsResource m_activeTarget       = InvalidGraphicsResource;
    GraphicsResource m_activeWindowTarget = InvalidGraphicsResource;
    GraphicsResource m_defaultTarget      = InvalidGraphicsResource;

    uint32_t m_emptyVertexArray = 0;
  };
} // namespace bfc
