#pragma once

#include "../core/Map.h"
#include "../core/StringView.h"
#include "../media/Surface.h"
#include "../core/Timestamp.h"

#include <variant>
#include <future>

namespace bfc {
  namespace platform {
    class Window;
  }
  class String;
  class GraphicsDevice;

  inline static const std::nullptr_t InvalidGraphicsResource = nullptr; ///< Invalid GraphicsResource value.
  constexpr int64_t                       MaxVertexBuffers        = 8;  ///< Max buffers allowed in a Vertex Array
  constexpr int64_t                       MaxColourAttachments    = 8;  ///< Max colour attachments allowed in a Render Target

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
    CubeMapFace_Front,
    CubeMapFace_Back,
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
    class BFC_API Buffer {
    public:
      /// Get the size of a buffer in bytes.
      virtual int64_t getSize() const = 0;
    };
    using BufferRef = Ref<Buffer>;

    class BFC_API VertexArray {
    public:
      virtual void setLayout(VertexInputLayout const & layout) = 0;

      virtual bool setVertexBuffer(int64_t slot, BufferRef vertexBufferID) = 0;

      virtual bool setIndexBuffer(BufferRef indexBufferID, DataType indexType) = 0;

      virtual VertexInputLayout getLayout() const = 0;

      virtual BufferRef getVertexBuffer(int64_t slot) const = 0;

      virtual BufferRef getIndexBuffer() const = 0;

      virtual DataType getIndexType() const = 0;
    };
    using VertexArrayRef = Ref<VertexArray>;

    class BFC_API Texture {
    public:
      virtual TextureType        getType() const                     = 0;
      virtual Vec3i              getSize(int64_t mipLevel = 0) const = 0;
      virtual bool               isDepthTexture() const              = 0;
      virtual DepthStencilFormat getDepthStencilFormat() const       = 0;
      virtual PixelFormat        getColourFormat() const             = 0;
    };
    using TextureRef = Ref<Texture>;

    class BFC_API Sampler {
    public:
      virtual void setSamplerMinFilter(FilterMode filter, FilterMode mipFilter) = 0;
      virtual void setSamplerMagFilter(FilterMode filter, FilterMode mipFilter) = 0;
      virtual void setSamplerMinLOD(float level) = 0;
      virtual void setSamplerMaxLOD(float level) = 0;
      virtual void setSamplerWrapU(WrapMode mode) = 0;
      virtual void setSamplerWrapV(WrapMode mode) = 0;
      virtual void setSamplerWrapW(WrapMode mode) = 0;

      inline void setSamplerWrap(WrapMode mode) {
        setSamplerWrapU(mode);
        setSamplerWrapV(mode);
        setSamplerWrapW(mode);
      }
    };
    using SamplerRef = Ref<Sampler>;

    struct ShaderDesc {
      Filename           path;
      String             src;
    };

    class BFC_API Program {
    public:
      virtual void                      addShader(ShaderType type, std::optional<ShaderDesc> desc) = 0;
      virtual std::optional<ShaderDesc> getShader(ShaderType type) const                           = 0;
      virtual bool                      compile(String * pError = nullptr)                         = 0;

      virtual int64_t getAttributeCount() const = 0;
      virtual int64_t getUniformCount() const   = 0;
      virtual int64_t getBufferCount() const    = 0;
      virtual int64_t getTextureCount() const   = 0;

      virtual void getAttributeDesc(int64_t uniformIndex, ProgramAttributeDesc * pDesc) const = 0;
      virtual void getUniformDesc(int64_t uniformIndex, ProgramUniformDesc * pDesc)     const = 0;
      virtual void getTextureDesc(int64_t textureIndex, ProgramTextureDesc * pDesc)     const = 0;
      virtual void getBufferDesc(int64_t bufferIndex, ProgramBufferDesc * pDesc)        const = 0;
    };
    using ProgramRef = Ref<Program>;

    class BFC_API RenderTarget {
    public:
      virtual RenderTargetType getType() const = 0;

      virtual Vec2i getSize() const = 0;

      virtual bool attachWindow(platform::Window * pWindow, DepthStencilFormat depthStencilFormat) = 0;

      virtual void attachColour(TextureRef textureID, int64_t slot = 0, int64_t mipLevel = 0,
                                int64_t layer = 0) = 0;

      virtual void setReadAttachment(int64_t slot) = 0;

      virtual void attachDepth(TextureRef textureID, int64_t mipLevel = 0, int64_t layer = 0) = 0; 
    };
    using RenderTargetRef = Ref<RenderTarget>;

    /// Graphics device state
    class BFC_API State {
    public:
      struct EnableBlend {
        bool enabled;
      };

      struct EnableStencilTest {
        bool enabled;
      };

      struct EnableScissorTest {
        bool enabled;
      };

      struct EnableDepthRead {
        bool enabled;
      };

      struct EnableDepthWrite {
        bool enabled;
      };

      struct Viewport {
        Vec2i position;
        Vec2i size;
      };

      struct Scissor {
        Vec2i position;
        Vec2i size;
      };

      struct DepthRange {
        float min = 0.0f;
        float max = 1.0f;
      };

      struct DepthFunc {
        ComparisonFunction comparison;
      };

      struct BlendFunc {
        BlendFunc(BlendFunction sourceFactor, BlendFunction destFactor, std::optional<int64_t> colourAttachment = std::nullopt)
          : sourceColourFactor(sourceFactor)
          , sourceAlphaFactor(sourceFactor)
          , destColourFactor(destFactor)
          , destAlphaFactor(destFactor)
          , colourAttachment(colourAttachment) {}
        BlendFunc(BlendFunction sourceColourFactor, BlendFunction sourceAlphaFactor, BlendFunction destColourFactor, BlendFunction destAlphaFactor,
                  std::optional<int64_t> colourAttachment = std::nullopt)
          : sourceColourFactor(sourceColourFactor)
          , sourceAlphaFactor(sourceAlphaFactor)
          , destColourFactor(destColourFactor)
          , destAlphaFactor(destAlphaFactor)
          , colourAttachment(colourAttachment) {}

        BlendFunction sourceColourFactor;
        BlendFunction sourceAlphaFactor;
        BlendFunction destColourFactor;
        BlendFunction destAlphaFactor;

        std::optional<int64_t> colourAttachment = std::nullopt;
      };

      struct BlendEq {
        BlendEq(BlendEquation colourAndAlpha, std::optional<int64_t> colourAttachment = std::nullopt)
          : colour(colourAndAlpha)
          , alpha(colourAndAlpha)
          , colourAttachment(colourAttachment) {}
        BlendEq(BlendEquation colour, BlendEquation alpha, std::optional<int64_t> colourAttachment = std::nullopt)
          : colour(colour)
          , alpha(alpha)
          , colourAttachment(colourAttachment) {}

        BlendEquation colour;
        BlendEquation alpha;

        std::optional<int64_t> colourAttachment = std::nullopt;
      };

      struct ColourWrite {
        ColourWrite(bool r, bool g, bool b, bool a)
          : r(r)
          , g(g)
          , b(b)
          , a(a) {}
        ColourWrite(bool rgba)
          : ColourWrite(rgba, rgba, rgba, rgba) {}
        bool r = true;
        bool g = true;
        bool b = true;
        bool a = true;
      };

      struct ColourFactor {
        ColourFactor(float r, float g, float b, float a)
          : r(r)
          , g(g)
          , b(b)
          , a(a) {}
        ColourFactor(float rgba)
          : ColourFactor(rgba, rgba, rgba, rgba) {}
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
      };

      using Storage = std::variant<
        std::monostate,
        EnableBlend,
        EnableStencilTest,
        EnableScissorTest,
        EnableDepthRead,
        EnableDepthWrite,
        Viewport,
        Scissor,
        DepthRange,
        DepthFunc,
        BlendFunc,
        BlendEq,
        ColourWrite,
        ColourFactor
      >;

      State(Storage const & value = std::monostate{})
        : m_storage(value) {}

      /// Visit stored state.
      template<typename Callable>
      auto visit(Callable&& v) const {
        return std::visit(std::forward<Callable>(v), m_storage);
      }

      template<typename T>
      bool is() const {
        return std::holds_alternative<T>(m_storage);
      }

      int64_t index() const {
        return m_storage.index();
      }

      inline static constexpr int64_t NumStates = std::variant_size_v<Storage>;

    private:
      Storage m_storage;
    };

    class BFC_API StateManager {
    public:
      StateManager();

      /// Push states, recording the previously known value.
      template<typename... States>
      void push(States &&... states) {
        Array<State, sizeof...(States)> values{std::forward<States>(states)...};
        beginGroup();
        (push(states), ...);
        pushGroup();
      }

      /// Begin a group of state changes that will be pushed to the stack.
      /// Use set() to make a state change within the group.
      /// Use endGroup() to push the group to the stack.
      /// Use pop() to pop the group from the stack and revert the state changes.
      /// @retval true A new group was started.
      /// @retval false A group is already started. You cannot nest beginGroup() calls.
      bool beginGroup();

      /// End a group of state changes and push the group to the stack.
      /// @retval true The group was closed and pushed to the stack.
      /// @retval false No group has been started. See beginGroup().
      bool endGroup();

      /// Push states, recording the previously known value.
      /// Equivalent to `beginGroup(); set(..), set(...), ...; endGroup();`
      void push(Span<const State> const & states);
      void push(State const & states);

      /// Pop a pushed state. Revert to the previous value (if known).
      void pop();

      /// Set a state without pushing the previous value to the stack.
      template<typename... States>
      void set(States &&... states) {
        Array<State, sizeof...(States)> values{std::forward<States>(states)...};
        set(Span<const State>{values.begin(), values.size()});
      }
      void set(Span<const State> const & states);
      void set(State const & state);

      /// Apply the state changes.
      /// This should only be called from a render thread.
      /// Users should not need to call this function under normal circumstances.
      void apply();

    protected:
      /// Implementation should apply the state to the graphics device.
      virtual void apply(State const & state) = 0;

    private:
      State m_state[State::NumStates]; // Currently set states.

      Vector<State> m_changes; // Current set of unapplied state changes.

      std::optional<int64_t> m_groupStart;

      Vector<int64_t> m_groups; // Number of states in each group. Groups are pushed/popped together. Sum should always be `m_stack.size()`.
      Vector<State>   m_stack;  // History of state changes.
    };

    class BFC_API BufferDownload {
    public:
      /// Wait for the download to complete.
      virtual bool wait(std::optional<Timestamp> const & timeout = std::nullopt) = 0;

      /// Take the downloaded data.
      virtual Vector<uint8_t> take() = 0;

      /// View the downloaded data.
      virtual Span<const uint8_t> view() const = 0;
    };
    using BufferDownloadRef = Ref<BufferDownload>;

    class BFC_API TextureDownload {
    public:
      /// Wait for the download to complete.
      virtual bool wait(std::optional<Timestamp> const & timeout = std::nullopt) = 0;
      /// View the downloaded data.
      virtual media::Surface view() const = 0;
      /// Take the downloaded texture.
      virtual std::tuple<media::Surface, Vector<uint8_t>> take() = 0;
    };
    using TextureDownloadRef = Ref<TextureDownload>;

    class BFC_API CommandList {
    public:
      // Pipeline state
      virtual void bindProgram(ProgramRef programID)                                                                    = 0;
      virtual void bindVertexArray(VertexArrayRef vertexArrayID)                                                        = 0;
      virtual void bindTexture(TextureRef textureID, int64_t textureUnit)                                               = 0;
      virtual void bindSampler(SamplerRef samplerID, int64_t textureUnit)                                               = 0;
      virtual void bindUniformBuffer(BufferRef bufferID, int64_t bindPoint, int64_t offset = 0, int64_t size = 0)       = 0;
      virtual void bindShaderStorageBuffer(BufferRef bufferID, int64_t bindPoint, int64_t offset = 0, int64_t size = 0) = 0;
      virtual void bindRenderTarget(RenderTargetRef renderTargetID, MapAccess renderTargetAccess = MapAccess_ReadWrite) = 0;
      virtual void bindScreen(MapAccess renderTargetAccess = MapAccess_ReadWrite)                                       = 0;

      /// Set a state without pushing the previous value to the stack.
      template<typename... States>
      void set(States &&... states) {
        std::array<State, sizeof...(States)> values{std::forward<States>(states)...};
        setState(Span<const State>{values.begin(), values.size()});
      }
      virtual void setState(Span<const State> const & state) = 0;
      /// Push a state, recording the previously known value.
      template<typename... States>
      void pushState(States &&... states) {
        std::array<State, sizeof...(States)> values{std::forward<States>(states)...};
        pushState(Span<const State>{values.begin(), values.size()});
      }
      virtual void pushState(Span<const State> const & state) = 0;
      virtual void popState()  = 0;

      // Buffers
      virtual bool upload(BufferRef bufferID, int64_t size, void const * pData = nullptr) = 0;
      
      /// Map a buffer to client memory.
      /// @param bufferID The buffer to map.
      /// @param access   Type of access needed (read, write, read/write).
      /// @returns A pointer to the mapped buffer.
      virtual std::future<void *> map(BufferRef bufferID, MapAccess access = MapAccess_ReadWrite) = 0;

      /// Map part of a buffer to client memory.
      /// @param bufferID The buffer to map.
      /// @param offset   The offset into the buffer to map (in bytes).
      /// @param size     The number of bytes to map, starting at `offset`.
      /// @param access   Type of access needed (read, write, read/write).
      /// @returns A pointer to the mapped buffer.
      virtual std::future<void *> map(BufferRef bufferID, int64_t offset, int64_t size, MapAccess access = MapAccess_ReadWrite) = 0;

      virtual void unmap(BufferRef bufferID) = 0;

      virtual void download(BufferRef bufferID, BufferDownloadRef dst, int64_t offset = 0, int64_t size = 0) = 0;

      // Textures
      virtual bool uploadTexture(TextureRef textureID, DepthStencilFormat format, Vec3i size)        = 0;
      virtual bool uploadTexture(TextureRef textureID, media::Surface const & src)                               = 0;
      virtual bool uploadTextureSubData(TextureRef textureID, media::Surface const & src, Vec3i offset)          = 0;
      virtual void generateMipMaps(TextureRef textureID)                                                         = 0;
      virtual bool downloadTexture(TextureRef textureID, TextureDownloadRef pDownload)                           = 0;

      // Shaders
      virtual void    setUniform(int64_t uniformIndex, void const * pBuffer, int64_t size)         = 0;
      virtual void    setBufferBinding(int64_t bufferIndex, int64_t bindPoint)                     = 0;
      virtual void    setTextureBinding(int64_t bufferIndex, int64_t bindPoint)                    = 0;
      // virtual void    getUniform(int64_t uniformIndex, void * pBuffer, ProgramUniformDesc * pDesc) = 0;
      // virtual int64_t getBufferBinding(int64_t bufferIndex)                                        = 0;
      // virtual int64_t getTextureBinding(int64_t bufferIndex)                                       = 0;

      // Rendering commands
      virtual void clear(RGBAu8 colour) = 0;
      virtual void swap()               = 0;

      virtual void draw(int64_t elementCount = std::numeric_limits<int64_t>::max(), int64_t elementOffset = 0, PrimitiveType primType = PrimitiveType_Triangle,
                        int64_t instanceCount = 1)                                                         = 0;
      virtual void drawIndexed(int64_t elementCount = std::numeric_limits<int64_t>::max(), int64_t elementOffset = 0,
                               PrimitiveType primType = PrimitiveType_Triangle, int64_t instanceCount = 1) = 0;

      /// Get the graphics device that created this command list.
      virtual GraphicsDevice * getDevice() const = 0;

      inline graphics::BufferRef createBuffer(BufferUsageHint usageHint = BufferUsageHint_Unknown) {
        return getDevice()->createBuffer();
      }
      inline graphics::VertexArrayRef createVertexArray() {
        return getDevice()->createVertexArray();
      }
      inline graphics::ProgramRef createProgram() {
        return getDevice()->createProgram();
      }
      inline graphics::TextureRef createTexture(TextureType type) {
        return getDevice()->createTexture(type);
      }
      inline graphics::SamplerRef createSampler() {
        return getDevice()->createSampler();
      }
      inline graphics::RenderTargetRef createRenderTarget(RenderTargetType type) {
        return getDevice()->createRenderTarget(type);
      }
    };
  } // namespace graphics

  class BFC_API GraphicsDevice {
  public:
    virtual bool init(platform::Window * pWindow) = 0;
    virtual void destroy()                        = 0;

    /// Get the state manager
    virtual std::unique_ptr<graphics::CommandList> createCommandList() = 0;
    /// Create a new buffer resource.
    /// @param usageHint A bitfield of usage flags indicating how the buffer is intended to be used.
    /// @returns A GraphicsResource that refers to the new buffer.
    virtual graphics::BufferRef createBuffer(BufferUsageHint usageHint = BufferUsageHint_Unknown) = 0;
    /// Create a new vertex array.
    virtual graphics::VertexArrayRef createVertexArray() = 0;
    /// Create a new shader.
    /// Create a new shader program.
    virtual graphics::ProgramRef createProgram() = 0;
    /// Create a new texture resource.
    /// @param type The type of texture to create.
    virtual graphics::TextureRef createTexture(TextureType type) = 0;
    /// Create a new sampler resource.
    virtual graphics::SamplerRef createSampler() = 0;
    /// Create a new render target resource.
    /// @param type The type of render target to create.
    virtual graphics::RenderTargetRef createRenderTarget(RenderTargetType type) = 0;
    /// Get the state manager
    virtual graphics::StateManager * getStateManager() = 0;

    /// Submit a command list to be executed.
    virtual uint64_t submit(std::unique_ptr<graphics::CommandList> const & pCommandList) = 0;

    /// Wait for a command list to complete execution.
    virtual bool wait(uint64_t handle, std::optional<Timestamp> const & timeout = std::nullopt) = 0;
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
} // namespace bfc
