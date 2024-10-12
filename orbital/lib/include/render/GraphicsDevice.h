#pragma once

#include "../core/Map.h"
#include "../core/StringView.h"
#include "../media/Surface.h"

namespace bfc {
  namespace platform {
    class Window;
  }
  class String;

  /// Available graphics resources types.
  enum GraphicsResourceType : int32_t {
    GraphicsResourceType_Invalid = -1,
    GraphicsResourceType_Buffer,
    GraphicsResourceType_VertexArray,
    GraphicsResourceType_Texture,
    GraphicsResourceType_Sampler,
    GraphicsResourceType_Shader,
    GraphicsResourceType_Program,
    GraphicsResourceType_RenderTarget,
    GraphicsResourceType_Count,
  };

  /// Graphics resource identifier.
  /// This struct encodes the ID and type of a graphics resource.
  struct GraphicsResource {
    /// Construct a GraphicsResource from a packed intptr_t.
    /// see operator inptr_t().
    constexpr GraphicsResource(intptr_t packed)
      : id((packed >> 32) & 0xFFFFFFFF)
      , type(GraphicsResourceType(packed & 0xFFFFFFFF)) {}

    /// Default construct a graphics resource.
    /// This is an invalid graphics resource.
    constexpr GraphicsResource()
      : id(0)
      , type(GraphicsResourceType_Invalid) {}

    /// Construct a GraphicsResource from the resource id and type.
    constexpr GraphicsResource(uint32_t id, GraphicsResourceType type)
      : id(id)
      , type(type) {}

    uint32_t             id   = 0;
    GraphicsResourceType type = GraphicsResourceType_Invalid;

    /// Equality check.
    /// @param rhs The resource to compare this instance with.
    /// @retval true  Both resources are of an invalid type, or the type and id are not identical.
    /// @retval false `rhs` is of a different type, or has a different `id`.
    constexpr bool operator==(GraphicsResource const & rhs) {
      return (type == -1 && rhs.type == -1) || (type == rhs.type && id == rhs.id);
    }

    /// Inequality check.
    constexpr bool operator!=(GraphicsResource const & rhs) {
      return !(*this == rhs);
    }

    /// Pack a GraphicsResource's id and type into an intptr_t.
    /// The id is stored in the high 32 bits.
    /// The type is stored in the low 32 bits.
    /// @returns The packed id.
    constexpr operator intptr_t() const {
      return (intptr_t(id) << 32) | intptr_t(type);
    }

    /// Get this GraphicsResource id, then reset it.
    /// This graphics resource will equal InvalidGraphicsResource after this operation.
    /// @returns The previous contents of this GraphicsResource.
    GraphicsResource take() {
      GraphicsResource ret = *this;
      *this                = GraphicsResource();
      return ret;
    }
  };

  constexpr GraphicsResource InvalidGraphicsResource = GraphicsResource(); ///< Invalid GraphicsResource value.
  constexpr int64_t          MaxVertexBuffers        = 8;                  ///< Max buffers allowed in a Vertex Array
  constexpr int64_t          MaxColourAttachments    = 8;                  ///< Max colour attachments allowed in a Render Target

  enum TextureType {
    TextureType_Unknown = -1,
    TextureType_2D,
    TextureType_2DArray,
    TextureType_3D,
    TextureType_CubeMap,
    TextureType_Count,
  };

  enum CubeMapFace {
    CubeMapFace_None = -1,
    CubeMapFace_Right,
    CubeMapFace_Left,
    CubeMapFace_Top,
    CubeMapFace_Bottom,
    CubeMapFace_Back,
    CubeMapFace_Front,
    CubeMapFace_Count,
  };

  inline Vec3d getCubeMapDirection(CubeMapFace const & face) {
    static constexpr Vec3d directions[] = {Vec3d(1, 0, 0), Vec3d(-1, 0, 0), Vec3d(0, 1, 0), Vec3d(0, -1, 0), Vec3d(0, 0, 1), Vec3d(0, 0, -1)};
    return directions[face];
  }

  inline Vec3d getCubeMapUp(CubeMapFace const & face) {
    static constexpr Vec3d directions[] = {Vec3d(0, -1, 0), Vec3d(0, -1, 0), Vec3d(0, 0, 1), Vec3d(0, 0, -1), Vec3d(0, -1, 0), Vec3d(0, -1, 0)};
    return directions[face];
  }

  inline Vec3d getCubeMapRight(CubeMapFace const & face) {
    static constexpr Vec3d directions[] = {Vec3d(0, 0, -1), Vec3d(0, 0, 1), Vec3d(1, 0, 0), Vec3d(1, 0, 0), Vec3d(1, 0, 0), Vec3d(-1, 0, 0)};
    return directions[face];
  }

  enum FilterMode {
    FilterMode_None,
    FilterMode_Linear,
    FilterMode_Nearest,
    FilterMode_Count,
  };

  enum WrapMode {
    WrapMode_Repeat,
    WrapMode_MirroredRepeat,
    WrapMode_ClampToEdge,
    WrapMode_ClampToBorder,
    WrapMode_Count,
  };

  enum ShaderType {
    ShaderType_Unknown = -1,
    ShaderType_Vertex,
    ShaderType_Fragment,
    ShaderType_Geometry,
    ShaderType_TessControl,
    ShaderType_TessEval,
    ShaderType_Compute,
    ShaderType_Count
  };

  enum BackBufferType {
    BackBufferType_Mono,
  };

  enum RenderTargetType {
    RenderTargetType_Unknown = -1,
    RenderTargetType_Texture,
    RenderTargetType_Window,
    RenderTargetType_Count,
  };

  enum DepthStencilFormat {
    DepthStencilFormat_Unknown = -1,
    DepthStencilFormat_None,
    DepthStencilFormat_D16,
    DepthStencilFormat_D32,
    DepthStencilFormat_D24S8,
    DepthStencilFormat_Count,
  };
  BFC_API int64_t getDepthStencilFormatStride(DepthStencilFormat const & type);

  enum MapAccess {
    MapAccess_None      = 0,
    MapAccess_Read      = 1 << 0,
    MapAccess_Write     = 1 << 1,
    MapAccess_ReadWrite = MapAccess_Read | MapAccess_Write,
  };

  enum BufferUsageHint {
    BufferUsageHint_Unknown  = 0,      ///< Buffer usage is unknown. A reasonable default should be used.
    BufferUsageHint_Uniform  = 1 << 0, ///< The buffer will be used for shader uniforms.
    BufferUsageHint_Storage  = 1 << 1, ///< The buffer will be used as a shader storage buffer.
    BufferUsageHint_Vertices = 1 << 2, ///< The buffer will be used for vertex data.
    BufferUsageHint_Indices  = 1 << 3, ///< The buffer will be used for index data.
    BufferUsageHint_Dynamic  = 1 << 5, ///< The buffer contents will be updated frequently. If this is not specified, buffer data is assumed to be static.
  };
  template<>
  struct enable_bitwise_operators<BufferUsageHint> : std::true_type {};

  enum LayoutFlag {
    LayoutFlag_None      = 0,
    LayoutFlag_Normalize = 1 << 0,
    LayoutFlag_Integer   = 1 << 1,
  };

  enum DataType {
    DataType_Unknown = -1,
    DataType_Bool,
    DataType_Int8,
    DataType_UInt8,
    DataType_Int16,
    DataType_UInt16,
    DataType_Int32,
    DataType_UInt32,
    DataType_Float32,
    DataType_Float64,
    DataType_Count,
  };
  BFC_API int64_t getDataTypeSize(DataType const & type);

  enum DataClass {
    DataClass_Unknown = -1,
    DataClass_Scalar,
    DataClass_Vector,
    DataClass_Matrix,
    DataClass_Count,
  };

  enum PrimitiveType {
    PrimitiveType_Triangle,
    PrimitiveType_Line,
    PrimitiveType_Point,
  };

  enum GraphicsState {
    GraphicsState_Blend,
    GraphicsState_StencilTest,
    GraphicsState_ScissorTest,
    GraphicsState_DepthTest,
    GraphicsState_DepthWrite,
    GraphicsState_Count,
  };

  enum BlendEquation {
    BlendEquation_Add,
    BlendEquation_Subtract,
    BlendEquation_ReverseSubtract,
    BlendEquation_Min,
    BlendEquation_Max,
    BlendEquation_Count,
  };

  enum BlendFunction {
    BlendFunction_Zero,
    BlendFunction_One,
    BlendFunction_SourceColour,
    BlendFunction_SourceAlpha,
    BlendFunction_DestColour,
    BlendFunction_DestAlpha,
    BlendFunction_OneMinusSourceColour,
    BlendFunction_OneMinusSourceAlpha,
    BlendFunction_OneMinusDestColour,
    BlendFunction_OneMinusDestAlpha,
    BlendFunction_ConstantAlpha,
    BlendFunction_OneMinusConstantAlpha,
    BlendFunction_Count,
  };

  enum ComparisonFunction {
    ComparisonFunction_Equal,
    ComparisonFunction_NotEqual,
    ComparisonFunction_Less,
    ComparisonFunction_Greater,
    ComparisonFunction_LessEqual,
    ComparisonFunction_GreaterEqual,
    ComparisonFunction_Always,
    ComparisonFunction_Never,
    ComparisonFunction_Count,
  };

  struct ProgramAttributeDesc {
    StringView name;
    DataClass  cls   = DataClass_Unknown;
    DataType   type  = DataType_Unknown;
    int64_t    width = 0, height = 0;
  };

  struct ProgramUniformDesc {
    StringView name;
    DataClass  cls   = DataClass_Unknown;
    DataType   type  = DataType_Unknown;
    int64_t    width = 0, height = 0;
  };

  struct ProgramBufferDesc {
    StringView name;
    int64_t    size = 0;
  };

  struct ProgramTextureDesc {
    StringView  name;
    TextureType type = TextureType_Unknown;
  };

  /// This class describes the layout of data used as vertex shader inputs.
  class BFC_API VertexInputLayout {
  public:
    struct Element {
      int64_t    slot      = 0;                ///< Vertex buffer slot
      DataType   dataType  = DataType_Unknown; ///< Type of data in vertex buffer
      DataClass  dataClass = DataClass_Scalar; ///< Class of data in vertex buffer
      int64_t    width     = 0;                ///< Width of item in buffer
      int64_t    height    = 0;                ///< Height of item in buffer
      int64_t    offset    = -1;               ///< Stride offset in buffer
      int64_t    stride    = -1;               ///< Stride of a vertex in the vertex buffer
      LayoutFlag flags     = LayoutFlag_None;  ///< Addition flags for how vertex buffer data is interpreted.
    };

    template<typename T>
    static VertexInputLayout Create() {
      return VertexTraits<T>::getLayout();
    }

    VertexInputLayout(Map<String, Element> const & description = {}) {
      for (Pair<String, Element> const & item : description) {
        m_semantics.pushBack(item.first);
        m_bufferLayout.pushBack(item.second);
      }
    }

    const Vector<String> & getSemantics() const {
      return m_semantics;
    }

    int64_t getAttributeCount() const {
      return m_semantics.size();
    }

    String const & getAttributeSemantic(int64_t index) const {
      return m_semantics[index];
    }

    Element const & getAttributeLayout(int64_t index) const {
      return m_bufferLayout[index];
    }

    void setAttribute(StringView semantic, Element const & element) {
      String * pFound = std::find(m_semantics.begin(), m_semantics.end(), semantic);

      if (pFound != m_semantics.end()) {
        m_bufferLayout[pFound - m_semantics.begin()] = element;
      } else {
        m_semantics.pushBack(semantic);
        m_bufferLayout.pushBack(element);
      }
    }

  private:
    Vector<String>  m_semantics;
    Vector<Element> m_bufferLayout;
  };

  /// Traits struct that describes the vertex layout of type `T`.
  /// Specialize this struct to add support for new vertex types.
  template<typename T>
  struct VertexTraits {
    static VertexInputLayout getLayout();
  };

  namespace graphics {
    class BFC_API BufferManager {
    public:
      /// Create a new buffer resource.
      /// @param usageHint A bitfield of usage flags indicating how the buffer is intended to be used.
      /// @returns A GraphicsResource that refers to the new buffer.
      virtual GraphicsResource createBuffer(BufferUsageHint usageHint = BufferUsageHint_Unknown) = 0;

      /// Add a reference to a buffer.
      /// @param `bufferID` The buffer to increment the reference count for.
      /// @returns `bufferID` for convenience.
      virtual GraphicsResource refBuffer(GraphicsResource bufferID) = 0;

      /// Release a reference to a buffer.
      /// The resource is released and the handle is reset. If no references to the buffer remain,
      /// the buffer is destroyed.
      /// @param pResource A pointer to the GraphicsResource to release.
      virtual void releaseBuffer(GraphicsResource * pResource) = 0;

      /// Get the number of references to a buffer.
      virtual int64_t getBufferReferences(GraphicsResource bufferID) = 0;

      /// Get the size of a buffer in bytes.
      virtual int64_t getSize(GraphicsResource bufferID) = 0;

      /// Map a buffer to client memory.
      /// @param bufferID The buffer to map.
      /// @param access   Type of access needed (read, write, read/write).
      /// @returns A pointer to the mapped buffer.
      virtual void * map(GraphicsResource bufferID, MapAccess access = MapAccess_ReadWrite) = 0;

      /// Map part of a buffer to client memory.
      /// @param bufferID The buffer to map.
      /// @param offset   The offset into the buffer to map (in bytes).
      /// @param size     The number of bytes to map, starting at `offset`.
      /// @param access   Type of access needed (read, write, read/write).
      /// @returns A pointer to the mapped buffer.
      virtual void * map(GraphicsResource bufferID, int64_t offset, int64_t size, MapAccess access = MapAccess_ReadWrite) = 0;

      virtual void unmap(GraphicsResource bufferID) = 0;

      virtual bool upload(GraphicsResource bufferID, int64_t size, void const * pData = nullptr) = 0;

      virtual int64_t download(GraphicsResource bufferID, void * pDst, int64_t offset = 0, int64_t size = 0) = 0;

      // Vertex Array Interface

      virtual GraphicsResource createVertexArray() = 0;

      virtual GraphicsResource refVertexArray(GraphicsResource vaID) = 0;

      virtual void releaseVertexArray(GraphicsResource * pResource) = 0;

      virtual int64_t getVertexArrayReferences(GraphicsResource vertexArrayID) = 0;

      virtual void setLayout(GraphicsResource vaID, VertexInputLayout const & layout) = 0;

      virtual bool setVertexBuffer(GraphicsResource vaID, int64_t slot, GraphicsResource vertexBufferID) = 0;

      virtual bool setIndexBuffer(GraphicsResource vaID, GraphicsResource indexBufferID, DataType indexType) = 0;

      virtual VertexInputLayout getLayout(GraphicsResource vaID) = 0;

      virtual GraphicsResource getVertexBuffer(GraphicsResource vaID, int64_t slot) = 0;

      virtual GraphicsResource getIndexBuffer(GraphicsResource vaID) = 0;

      virtual DataType getIndexType(GraphicsResource vaID) = 0;
    };

    class BFC_API TextureManager {
    public:
      /// Create a new texture resource.
      virtual GraphicsResource createTexture(TextureType type) = 0;

      virtual GraphicsResource refTexture(GraphicsResource textureID) = 0;

      virtual void releaseTexture(GraphicsResource * pResource) = 0;

      virtual int64_t getTextureReferences(GraphicsResource resourceID) = 0;

      virtual bool upload(GraphicsResource textureID, DepthStencilFormat format, Vec3i size) = 0;

      virtual bool upload(GraphicsResource textureID, media::Surface const & src) = 0;

      virtual bool uploadSubData(GraphicsResource textureID, media::Surface const & src, Vec3i offset) = 0;

      virtual void generateMipMaps(GraphicsResource textureID) = 0;

      virtual bool download(GraphicsResource textureID, media::Surface * pDest) = 0;

      virtual TextureType getType(GraphicsResource textureID) = 0;

      virtual Vec3i getSize(GraphicsResource textureID, int64_t mipLevel = 0) = 0;

      virtual bool isDepthTexture(GraphicsResource textureID) = 0;

      virtual DepthStencilFormat getDepthStencilFormat(GraphicsResource textureID) = 0;

      virtual PixelFormat getColourFormat(GraphicsResource textureID) = 0;

      // Samplers

      virtual GraphicsResource createSampler() = 0;

      virtual GraphicsResource refSampler(GraphicsResource samplerID) = 0;

      virtual void releaseSampler(GraphicsResource * pResource) = 0;

      virtual int64_t getSamplerReferences(GraphicsResource samplerID) = 0;

      virtual void setSamplerMinFilter(GraphicsResource samplerID, FilterMode filter, FilterMode mipFilter) = 0;
      virtual void setSamplerMagFilter(GraphicsResource samplerID, FilterMode filter, FilterMode mipFilter) = 0;

      virtual void setSamplerMinLOD(GraphicsResource samplerID, float level) = 0;
      virtual void setSamplerMaxLOD(GraphicsResource samplerID, float level) = 0;

      virtual void setSamplerWrapU(GraphicsResource samplerID, WrapMode mode) = 0;
      virtual void setSamplerWrapV(GraphicsResource samplerID, WrapMode mode) = 0;
      virtual void setSamplerWrapW(GraphicsResource samplerID, WrapMode mode) = 0;

      inline void setSamplerWrap(GraphicsResource samplerID, WrapMode mode) {
        setSamplerWrapU(samplerID, mode);
        setSamplerWrapV(samplerID, mode);
        setSamplerWrapW(samplerID, mode);
      }
    };

    class BFC_API ShaderManager {
    public:
      /// Create a new shader resource.
      virtual GraphicsResource createShader(ShaderType type) = 0;

      virtual GraphicsResource refShader(GraphicsResource shaderID) = 0;

      virtual void releaseShader(GraphicsResource * pResource) = 0;

      virtual int64_t getShaderReferences(GraphicsResource shaderID) = 0;

      virtual ShaderType getType(GraphicsResource shaderID) = 0;

      virtual void setSource(GraphicsResource shaderID, StringView src) = 0;

      virtual void setFile(GraphicsResource shaderID, StringView path) = 0;

      virtual bool compile(GraphicsResource shaderID, String * pError = nullptr) = 0;

      virtual GraphicsResource createProgram() = 0;

      virtual GraphicsResource refProgram(GraphicsResource programID) = 0;

      virtual void releaseProgram(GraphicsResource * pResource) = 0;

      virtual int64_t getProgramReferences(GraphicsResource programID) = 0;

      virtual void addShader(GraphicsResource programID, GraphicsResource shaderID) = 0;

      virtual GraphicsResource getShader(GraphicsResource programID, ShaderType shaderID) = 0;

      virtual bool linkProgram(GraphicsResource programID, String * pError = nullptr) = 0;

      // Program inputs

      virtual void setUniform(GraphicsResource programID, int64_t uniformIndex, void const * pBuffer) = 0;

      virtual void setBufferBinding(GraphicsResource programID, int64_t bufferIndex, int64_t bindPoint) = 0;

      virtual void setTextureBinding(GraphicsResource programID, int64_t bufferIndex, int64_t bindPoint) = 0;

      virtual void getUniform(GraphicsResource programID, int64_t uniformIndex, void * pBuffer, ProgramUniformDesc * pDesc) = 0;

      virtual int64_t getBufferBinding(GraphicsResource programID, int64_t bufferIndex) = 0;

      virtual int64_t getTextureBinding(GraphicsResource programID, int64_t bufferIndex) = 0;

      // Program reflection

      virtual int64_t getAttributeCount(GraphicsResource programID) = 0;

      virtual int64_t getUniformCount(GraphicsResource programID) = 0;

      virtual int64_t getBufferCount(GraphicsResource programID) = 0;

      virtual int64_t getTextureCount(GraphicsResource programID) = 0;

      virtual void getAttributeDesc(GraphicsResource programID, int64_t uniformIndex, ProgramAttributeDesc * pDesc) = 0;

      virtual void getUniformDesc(GraphicsResource programID, int64_t uniformIndex, ProgramUniformDesc * pDesc) = 0;

      virtual void getTextureDesc(GraphicsResource programID, int64_t textureIndex, ProgramTextureDesc * pDesc) = 0;

      virtual void getBufferDesc(GraphicsResource programID, int64_t bufferIndex, ProgramBufferDesc * pDesc) = 0;
    };

    class BFC_API RenderTargetManager {
    public:
      virtual GraphicsResource createRenderTarget(RenderTargetType type) = 0;

      virtual GraphicsResource refRenderTarget(GraphicsResource renderTargetID) = 0;

      virtual void releaseRenderTarget(GraphicsResource * pRenderTargetID) = 0;

      virtual int64_t getRenderTargetReferences(GraphicsResource renderTargetID) = 0;

      virtual RenderTargetType getType(GraphicsResource renderTargetID) = 0;

      virtual Vec2i getSize(GraphicsResource renderTargetID) = 0;

      virtual bool attachWindow(GraphicsResource renderTargetID, platform::Window * pWindow, DepthStencilFormat depthStencilFormat) = 0;

      virtual void attachColour(GraphicsResource renderTargetID, GraphicsResource textureID, int64_t slot = 0, int64_t mipLevel = 0, int64_t layer = 0) = 0;

      virtual void setReadAttachment(GraphicsResource renderTargetID, int64_t slot) = 0;

      virtual void attachDepth(GraphicsResource renderTargetID, GraphicsResource textureID, int64_t mipLevel = 0, int64_t layer = 0) = 0;
    };

    class BFC_API StateManager {
    public:
      virtual void setFeatureEnabled(GraphicsState state, bool enabled) = 0;

      virtual void setViewport(Vec2i position, Vec2i size) = 0;

      virtual void setScissor(Vec2i position, Vec2i size) = 0;

      virtual void setDepthRange(float min, float max) = 0;

      virtual void setDepthFunction(ComparisonFunction function) = 0;

      virtual void setBlendEquation(BlendEquation colourAndAlpha, int64_t colourAttachment = -1) = 0;

      virtual void setBlendEquation(BlendEquation colour, BlendEquation alpha, int64_t colourAttachment = -1) = 0;

      virtual void setBlendFunction(BlendFunction sourceFactor, BlendFunction destFactor, int64_t colourAttachment = -1) = 0;

      virtual void setBlendFunction(BlendFunction sourceColourFactor, BlendFunction sourceAlphaFactor, BlendFunction destColourFactor,
                                    BlendFunction destAlphaFactor, int64_t colourAttachment = -1) = 0;

      virtual void setColourWriteEnabled(bool red, bool green, bool blue, bool alpha) = 0;

      virtual void setColourFactor(float red, float green, float blue, float alpha) = 0;
    };
  } // namespace graphics

  class BFC_API GraphicsDevice {
  public:
    virtual bool init(platform::Window * pWindow) = 0;
    virtual void destroy()                        = 0;

    virtual void bindProgram(GraphicsResource programID)                                                                     = 0;
    virtual void bindVertexArray(GraphicsResource vertexArrayID)                                                             = 0;
    virtual void bindTexture(GraphicsResource textureID, int64_t textureUnit)                                                = 0;
    virtual void bindSampler(GraphicsResource samplerID, int64_t textureUnit)                                                = 0;
    virtual void bindUniformBuffer(GraphicsResource bufferID, int64_t bindPoint, int64_t offset = 0, int64_t size = 0)       = 0;
    virtual void bindShaderStorageBuffer(GraphicsResource bufferID, int64_t bindPoint, int64_t offset = 0, int64_t size = 0) = 0;
    virtual void bindRenderTarget(GraphicsResource renderTargetID, MapAccess renderTargetAccess = MapAccess_ReadWrite)       = 0;
    virtual void bindScreen(MapAccess renderTargetAccess = MapAccess_ReadWrite)                                              = 0;

    virtual void draw(int64_t elementCount = std::numeric_limits<int64_t>::max(), int64_t elementOffset = 0, PrimitiveType primType = PrimitiveType_Triangle,
                      int64_t instanceCount = 1)                                                         = 0;
    virtual void drawIndexed(int64_t elementCount = std::numeric_limits<int64_t>::max(), int64_t elementOffset = 0,
                             PrimitiveType primType = PrimitiveType_Triangle, int64_t instanceCount = 1) = 0;

    virtual void clear(RGBAu8 colour) = 0;
    virtual void swap()               = 0;

    virtual graphics::BufferManager *       getBufferManager()       = 0;
    virtual graphics::TextureManager *      getTextureManager()      = 0;
    virtual graphics::ShaderManager *       getShaderManager()       = 0;
    virtual graphics::RenderTargetManager * getRenderTargetManager() = 0;
    virtual graphics::StateManager *        getStateManager()        = 0;

    int64_t          getReferences(GraphicsResource resourceID);
    GraphicsResource refResource(GraphicsResource resourceID);
    void             releaseResource(GraphicsResource * pResourceID);
  };

  /// Graphics device factory function type.
  /// see registerGraphicsDevice().
  using GraphicsDeviceFactory = Ref<GraphicsDevice> (*)();

  /// Add a new graphics device.
  /// @param name    The identifier for the graphics device.
  /// @param Factory A factory function that allocates the graphics device.
  /// @retval true The device was registered successfully.
  /// @retval false The device could not be registered (likely a device with `name` already exists).
  BFC_API bool registerGraphicsDevice(StringView const & name, GraphicsDeviceFactory Factory);

  /// Create a new graphics device by `name`.
  /// The device must have been previously registered using registerGraphicsDevice().
  /// @param name The name of the device to create.
  /// @retval nullptr No device with `name` is registered.
  /// @returns A pointer to the new graphics device if successful.
  BFC_API Ref<GraphicsDevice> createGraphicsDevice(StringView const & name);

  /// Get the number of registered graphics devices.
  BFC_API int64_t getGraphicsDeviceCount();

  /// Get the name of a graphics device by index.
  BFC_API StringView getGraphicsDeviceName(int64_t index);

  class BFC_API ManagedGraphicsResource {
  public:
    virtual ~ManagedGraphicsResource();
    ManagedGraphicsResource() = default;
    ManagedGraphicsResource(GraphicsDevice * pDevice, GraphicsResource resource);
    ManagedGraphicsResource(ManagedGraphicsResource const & o);
    ManagedGraphicsResource(ManagedGraphicsResource && o);
    ManagedGraphicsResource & operator=(ManagedGraphicsResource const & o);
    ManagedGraphicsResource & operator=(ManagedGraphicsResource && o);

    bool hasResource() const;
    void release();

    GraphicsResource     takeResource(GraphicsDevice ** ppDevice);
    GraphicsResource     getResource() const;
    GraphicsDevice *     getDevice() const;
    GraphicsResourceType getType() const;
    int64_t              getReferences() const;

    operator GraphicsResource() const;

  protected:
    void set(GraphicsDevice * pDevice, GraphicsResource resource);

  private:
    GraphicsDevice * m_pDevice  = nullptr;
    GraphicsResource m_resource = InvalidGraphicsResource;
  };

  // TODO: Work out a solution that doesn't require exposing these functions
  bool graphicsDevice_registerOpenGL();

  template<typename T>
  struct EnumValueMap;
  template<>
  struct EnumValueMap<ShaderType> {
    // inline static Vector<any> const mapping;
    inline static Map<ShaderType, String> const mapping = {{ShaderType_Fragment, "frag"},
                                                           {ShaderType_Vertex, "vert"},
                                                           {ShaderType_Geometry, "geom"},
                                                           {ShaderType_Compute, "compute"},
                                                           {ShaderType_TessControl, "tess-control"},
                                                           {ShaderType_TessEval, "tess-eval"}};
  };

  template<>
  struct EnumValueMap<CubeMapFace> {
    // inline static Vector<any> const mapping;
    inline static Map<CubeMapFace, String> const mapping = {{CubeMapFace_Left, "left"},     {CubeMapFace_Right, "right"}, {CubeMapFace_Top, "top"},
                                                            {CubeMapFace_Bottom, "bottom"}, {CubeMapFace_Front, "front"}, {CubeMapFace_Back, "back"}};
  };

  template<>
  struct EnumValueMap<GraphicsResourceType> {
    // inline static Vector<any> const mapping;
    inline static Map<GraphicsResourceType, String> const mapping = {{GraphicsResourceType_Buffer, "buffer"},
                                                                     {GraphicsResourceType_VertexArray, "vertex-array"},
                                                                     {GraphicsResourceType_Texture, "texture"},
                                                                     {GraphicsResourceType_Sampler, "sampler"},
                                                                     {GraphicsResourceType_Shader, "shader"},
                                                                     {GraphicsResourceType_Program, "program"},
                                                                     {GraphicsResourceType_RenderTarget, "render-target"}};
  };
} // namespace bfc
