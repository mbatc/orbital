#include "GraphicsDevice_OpenGL.h"
#include "OpenGLCommands.h"

#include "core/File.h"
#include "platform/Window.h"

namespace bfc {
  bool graphicsDevice_registerOpenGL() {
    return registerGraphicsDevice("OpenGL", []() -> Ref<GraphicsDevice> { return NewRef<GraphicsDevice_OpenGL>(); });
  }

  static graphics::GLBuffer &          ToGL(graphics::BufferRef pBuffer);
  static graphics::GLBufferDownload &  ToGL(graphics::BufferDownloadRef pBuffer);
  static graphics::GLTexture &         ToGL(graphics::TextureRef pBuffer);
  static graphics::GLTextureDownload & ToGL(graphics::TextureDownloadRef pBuffer);
  static graphics::GLSampler &         ToGL(graphics::SamplerRef pBuffer);
  static graphics::GLRenderTarget &    ToGL(graphics::RenderTargetRef pBuffer);
  static graphics::GLProgram &         ToGL(graphics::ProgramRef pBuffer);
  static graphics::GLVertexArray &     ToGL(graphics::VertexArrayRef pBuffer);

  static GLenum ToGLShaderType(ShaderType type);
  static GLenum ToGLTextureType(TextureType type);
  static GLenum ToGLAccess(MapAccess access);
  static GLenum ToGLMapBits(MapAccess access);
  static GLenum ToGLInternalFormat(PixelFormat pixelFormat);
  static GLenum ToGLInternalFormat(DepthStencilFormat pixelFormat);
  static GLenum ToGLFormat(PixelFormat pixelFormat);
  static GLenum ToGLFormat(DepthStencilFormat depthStencilFormat);
  static GLenum ToGLType(PixelFormat pixelFormat);
  static GLenum ToGLType(DepthStencilFormat depthStencilFormat);
  static GLenum ToGLDataType(DataType type);
  static GLenum ToGLPrimType(PrimitiveType primType);
  static GLenum ToGLFramebufferAttachment(DepthStencilFormat depthStencilFmt);
  static GLenum ToGLFramebufferTextureTarget(TextureType texType, int64_t level);
  static GLenum ToGLEquation(BlendEquation blendEquation);
  static GLenum ToGLBlendFunction(BlendFunction function);
  static GLenum ToGLComparison(ComparisonFunction function);
  static GLenum ToGLCubeMapFace(CubeMapFace face);
  static GLenum ToGLWrapMode(WrapMode mode);
  static GLenum ToGLFilterMode(FilterMode filter, FilterMode mipFilter);

  static int64_t GetSemanticLocation(StringView const & semantic);
  static bool    GetGLTypeDetails(GLenum glType, DataType * pType, DataClass * pClass, int64_t * pWidth, int64_t * pHeight);
  static bool    GetGLTextureType(GLenum glType, TextureType * pType);
  static void    SetGLStateEnabled(GLenum feature, bool enabled);

  static void GLAPIENTRY ErrorMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message,
                                              const void * userParam);

  static PIXELFORMATDESCRIPTOR g_defaultPfd = {
    sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW, PFD_TYPE_RGBA, 32, 8, 0, 8, 0, 8, 0, 0, 0, 0, 0, 0, 24, 8, 0, 0, 0, PFD_MAIN_PLANE, 0, 0};

  static PFNWGLCHOOSEPIXELFORMATARBPROC    glChoosePixelFormatARB    = nullptr;
  static PFNWGLCREATECONTEXTATTRIBSARBPROC glCreateContextAttribsARB = nullptr;

  namespace graphics {
    int64_t GLBuffer::getSize() const {
      return size;
    }

    GLenum GLBuffer::getBindTarget(bool read) const {
      if (defaultTarget != GL_NONE)
        return defaultTarget;
      else
        return read ? GL_COPY_READ_BUFFER : GL_COPY_WRITE_BUFFER;
    }

    void GLVertexArray::setLayout(VertexInputLayout const & layout) {
      this->layout = layout;
      this->rebind = true;
    }

    bool GLVertexArray::setVertexBuffer(int64_t slot, BufferRef vertexBufferID) {
      this->vertexBuffers[slot] = vertexBufferID;
      this->rebind              = true;
      return true;
    }

    bool GLVertexArray::setIndexBuffer(BufferRef indexBufferID, DataType indexType) {
      this->indexBufferType = indexType;
      this->indexBuffer     = indexBufferID;
      this->rebind          = true;
      return true;
    }

    VertexInputLayout GLVertexArray::getLayout() const {
      return layout;
    }

    BufferRef GLVertexArray::getVertexBuffer(int64_t slot) const {
      return vertexBuffers[slot];
    }

    BufferRef GLVertexArray::getIndexBuffer() const {
      return indexBuffer;
    }

    DataType GLVertexArray::getIndexType() const {
      return indexBufferType;
    }

    TextureType GLTexture::getType() const {
      return type;
    }

    bool GLTexture::isDepthTexture() const {
      return depthStencilFmt != DepthStencilFormat_None;
    }

    DepthStencilFormat GLTexture::getDepthStencilFormat() const {
      return depthStencilFmt;
    }

    PixelFormat GLTexture::getColourFormat() const {
      return format;
    }

    Vec3i GLTexture::getSize(int64_t mipLevel) const {
      Vec3i size = size;
      size /= (1 << (int32_t)mipLevel);
      return {std::max(1, size.x), std::max(1, size.y), std::max(1, size.z)};
    }

    void GLSampler::setSamplerMinFilter(FilterMode filter, FilterMode mipFilter) {
      if (minFilter != filter || minMipFilter != mipFilter) {
        minFilter    = filter;
        minMipFilter = mipFilter;
        changed      = true;
      }
    }

    void GLSampler::setSamplerMagFilter(FilterMode filter, FilterMode mipFilter) {
      if (magFilter != filter || magMipFilter != mipFilter) {
        magFilter    = filter;
        magMipFilter = mipFilter;
        changed      = true;
      }
    }

    void GLSampler::setSamplerMinLOD(float level) {
      if (minLOD != level) {
        minLOD  = level;
        changed = true;
      }
    }

    void GLSampler::setSamplerMaxLOD(float level) {
      if (minLOD != level) {
        minLOD  = level;
        changed = true;
      }
    }

    void GLSampler::setSamplerWrapU(WrapMode mode) {
      if (wrapMode.x != mode) {
        wrapMode.x = mode;
        changed    = true;
      }
    }

    void GLSampler::setSamplerWrapV(WrapMode mode) {
      if (wrapMode.y != mode) {
        wrapMode.y = mode;
        changed    = true;
      }
    }

    void GLSampler::setSamplerWrapW(WrapMode mode) {
      if (wrapMode.z != mode) {
        wrapMode.z = mode;
        changed    = true;
      }
    }

    void GLProgram::addShader(ShaderType type, std::optional<ShaderDesc> desc) {
      shaders[type] = desc;
    }

    std::optional<ShaderDesc> GLProgram::getShader(ShaderType shaderType) const {
      return shaders[shaderType];
    }

    static bool compileShader(GLint glID, ShaderDesc const & desc, String * pError) {
      String src;
      if (desc.path.length() > 0) {
        if (!readTextFile(desc.path, &src)) {
          *pError = String("Failed to read source file ").concat(desc.path);
          return false;
        }
      }

      if (src.empty()) {
        if (pError != nullptr) {
          *pError = "Source was empty";
        }
        return false;
      }

      char const * srcPtr = src.c_str();
      GLint        len    = (GLint)src.length();
      glShaderSource(glID, 1, &srcPtr, &len);
      glCompileShader(glID);
      int status = 1;
      glGetShaderiv(glID, GL_COMPILE_STATUS, &status);

      if (status == GL_FALSE) {
        // Compilation failed
        int logLen = 0;
        glGetShaderiv(glID, GL_INFO_LOG_LENGTH, &logLen);
        Vector<char> log(logLen + 1, 0);
        glGetShaderInfoLog(glID, (GLsizei)log.size(), 0, log.data());

        if (pError != nullptr) {
          *pError = log.data();
        }

        return false;
      }

      return true;
    }

    bool GLProgram::compile(String * pError) {
      GLuint shaderIDs[ShaderType_Count] = { 0 };

      for (auto &[i, shader] : enumerate(shaders)) {
        if (!shader.has_value()) {
          continue;
        }

        GLuint shaderID = glCreateShader(ToGLShaderType((ShaderType)i));
        if (!compileShader(shaderID, shader.value(), pError)) {
          glDeleteShader(shaderID);
          return false;
        }

        shaderIDs[i] = shaderID;
        glAttachShader(glID, shaderID);
        return true;
      }

      GLint status = GL_FALSE;
      glLinkProgram(glID);
      glGetProgramiv(glID, GL_LINK_STATUS, &status);

      if (status == GL_FALSE) {
        int logLen = 0;
        glGetProgramiv(glID, GL_INFO_LOG_LENGTH, &logLen);
        Vector<char> log(logLen + 1, 0);
        glGetProgramInfoLog(glID, (GLsizei)log.size(), 0, log.data());

        printf("%s\n", log.data());

        if (pError != nullptr) {
          *pError = log.data();
        }
      }

      for (auto & [i, shader] : enumerate(shaders)) {
        if (!shader.has_value()) {
          continue;
        }

        glDetachShader(glID, shaderIDs[i]);
        glDeleteShader(glID);
      }

      if (status != GL_FALSE)
        reflect(glID, &attributes, &uniforms, &textures, &buffers);

      return status == GL_TRUE;
    }

    int64_t GLProgram::getAttributeCount() const {
      return attributes.size();
    }

    int64_t GLProgram::getUniformCount() const {
      return uniforms.size();
    }

    int64_t GLProgram::getBufferCount() const {
      return buffers.size();
    }

    int64_t GLProgram::getTextureCount() const {
      return textures.size();
    }

    void GLProgram::getAttributeDesc(int64_t attributeIndex, ProgramAttributeDesc * pDesc) const {
      Attribute const & attributeDesc = attributes[attributeIndex];
      pDesc->name                     = attributeDesc.name;
      pDesc->cls                      = attributeDesc.cls;
      pDesc->type                     = attributeDesc.type;
      pDesc->width                    = attributeDesc.width;
      pDesc->height                   = attributeDesc.height;
    }

    void GLProgram::getUniformDesc(int64_t uniformIndex, ProgramUniformDesc * pDesc) const {
      Uniform const & uniformDesc = uniforms[uniformIndex];
      pDesc->name                 = uniformDesc.name;
      pDesc->cls                  = uniformDesc.cls;
      pDesc->type                 = uniformDesc.type;
      pDesc->width                = uniformDesc.width;
      pDesc->height               = uniformDesc.height;
    }

    void GLProgram::getTextureDesc(int64_t textureIndex, ProgramTextureDesc * pDesc) const {
      Texture const & textureDesc = textures[textureIndex];
      pDesc->name                 = textureDesc.name;
      pDesc->type                 = textureDesc.type;
    }

    void GLProgram::getBufferDesc(int64_t bufferIndex, ProgramBufferDesc * pDesc) const {
      Buffer const & bufferDesc = buffers[bufferIndex];
      pDesc->name               = bufferDesc.name;
      pDesc->size               = bufferDesc.size;
    }

    void GLProgram::reflect(uint32_t glID, Vector<Attribute> * pAttributes, Vector<Uniform> * pUniforms, Vector<Texture> * pTextures,
                            Vector<Buffer> * pBuffers) {
      int32_t numUniformBlocks   = 0;
      int32_t numUniforms        = 0;
      int32_t numAttributes      = 0;
      int32_t storageBufferCount = 0;
      glGetProgramiv(glID, GL_ACTIVE_ATTRIBUTES, &numAttributes);
      glGetProgramiv(glID, GL_ACTIVE_UNIFORMS, &numUniforms);
      glGetProgramiv(glID, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBlocks);
      glGetProgramInterfaceiv(glID, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &storageBufferCount);

      int32_t maxNameLen = 0;
      int32_t nameLen    = 0;
      glGetProgramiv(glID, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &nameLen);
      maxNameLen = std::max(maxNameLen, nameLen);
      glGetProgramiv(glID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &nameLen);
      maxNameLen = std::max(maxNameLen, nameLen);
      glGetProgramiv(glID, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &nameLen);
      maxNameLen = std::max(maxNameLen, nameLen);
      glGetProgramInterfaceiv(glID, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, &nameLen);
      maxNameLen = std::max(maxNameLen, nameLen);

      Vector<char> nameBuffer(maxNameLen + 1, '\0');
      int32_t      bufSize = (int32_t)nameBuffer.size();
      int32_t      len = 0, size = 0;
      // Query attributes
      for (int32_t i = 0; i < numAttributes; ++i) {
        GLenum glType = GL_NONE;
        glGetActiveAttrib(glID, (GLuint)i, maxNameLen, &len, &size, &glType, nameBuffer.data());

        Attribute attribDesc;
        attribDesc.glLoc = glGetAttribLocation(glID, nameBuffer.data());
        attribDesc.name  = nameBuffer.data();

        GetGLTypeDetails(glType, &attribDesc.type, &attribDesc.cls, &attribDesc.width, &attribDesc.height);

        pAttributes->pushBack(attribDesc);
      }

      // Local function to add a uniform to the reflection data
      auto addUniform = [pUniforms, pTextures](uint32_t glLoc, GLenum glType, String name) {
        Texture textureDesc;
        Uniform uniformDesc;
        if (GetGLTypeDetails(glType, &uniformDesc.type, &uniformDesc.cls, &uniformDesc.width, &uniformDesc.height)) {
          uniformDesc.glLoc = glLoc;
          uniformDesc.name  = name;
          pUniforms->pushBack(uniformDesc);
        } else if (GetGLTextureType(glType, &textureDesc.type)) {
          textureDesc.glLoc = glLoc;
          pTextures->pushBack(textureDesc);
        }
      };

      // Query uniforms
      for (int32_t i = 0; i < numUniforms; ++i) {
        GLenum glType = GL_NONE;
        glGetActiveUniform(glID, (GLuint)i, maxNameLen, &len, &size, &glType, nameBuffer.data());

        uint32_t glLoc = glGetUniformLocation(glID, nameBuffer.data());

        if (size > 1) {
          String fullName = nameBuffer.data();
          String baseName = fullName.substr(0, fullName.findLast('['));
          for (int32_t i = 0; i < size; ++i) {
            addUniform(glLoc + i, glType, String::format("%s[%d]", baseName.c_str(), i));
          }
        } else {
          addUniform(glLoc, glType, nameBuffer.data());
        }
      }

      // Query uniform interface blocks
      for (int32_t i = 0; i < numUniformBlocks; ++i) {
        GLint binding = 0, size = 0;

        glGetActiveUniformBlockiv(glID, (GLuint)i, GL_UNIFORM_BLOCK_BINDING, &binding);
        glGetActiveUniformBlockiv(glID, (GLuint)i, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
        glGetActiveUniformBlockName(glID, (GLuint)i, maxNameLen, 0, nameBuffer.data());

        Buffer bufferDesc;
        bufferDesc.name        = nameBuffer.data();
        bufferDesc.bindPoint   = binding;
        bufferDesc.size        = size;
        bufferDesc.bufferIndex = (uint32_t)i;

        pBuffers->pushBack(bufferDesc);
      }

      // Query shader storage buffer objects
      for (int32_t buffer = 0; buffer < storageBufferCount; ++buffer) {
        glGetProgramResourceName(glID, GL_SHADER_STORAGE_BLOCK, buffer, maxNameLen, nullptr, nameBuffer.data());
        GLenum        queries[]         = {GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE};
        constexpr int numValues         = sizeof(queries) / sizeof(GLenum);
        GLint         values[numValues] = {0};
        glGetProgramResourceiv(glID, GL_SHADER_STORAGE_BLOCK, buffer, numValues, queries, numValues, 0, values);

        Buffer bufferDesc;
        bufferDesc.name            = nameBuffer.data();
        bufferDesc.size            = values[1];
        bufferDesc.bindPoint       = values[0];
        bufferDesc.bufferIndex     = buffer;
        bufferDesc.isStorageBuffer = true;

        pBuffers->pushBack(bufferDesc);
      }
    }

    RenderTargetType GLRenderTarget::getType() const {
      return type;
    }

    Vec2i GLRenderTarget::getSize() const {
      switch (type) {
      case RenderTargetType_Texture:
        for (auto & [i, colour] : enumerate(textures.colour)) {
          if (colour.texture != InvalidGraphicsResource) {
            return colour.texture->getSize(colour.mipLevel);
          }
        }

        if (textures.depth.texture != InvalidGraphicsResource) {
          return textures.depth.texture->getSize(textures.depth.mipLevel);
        }
        break;
      case RenderTargetType_Window: {
        RECT rect;
        GetClientRect(window.hWnd, &rect);
        return {rect.right - rect.left, rect.bottom - rect.top};
      } break;
      }

      return Vec2i(0);
    }

    bool GLRenderTarget::attachWindow(platform::Window * pWindow, DepthStencilFormat depthStencilFormat) {
      window.hWnd        = (HWND)pWindow->getPlatformHandle();
      window.hDC         = GetDC(window.hWnd);
      window.depthFormat = depthStencilFormat;

      int64_t numDepthBits   = 0;
      int64_t numStencilBits = 0;
      switch (depthStencilFormat) {
      case DepthStencilFormat_D32: numDepthBits = 32; break;
      case DepthStencilFormat_D24S8:
        numDepthBits   = 24;
        numStencilBits = 8;
        break;
      }

      // Get the attribute list
      Vector<int> attribList = {WGL_DRAW_TO_WINDOW_ARB, GL_TRUE, WGL_SUPPORT_OPENGL_ARB, GL_TRUE,          WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
                                WGL_COLOR_BITS_ARB,     32,      WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB};

      // Depth Buffer Format
      if (numDepthBits > 0)
        attribList.pushBack({WGL_DEPTH_BITS_ARB, (int)numDepthBits});
      if (numStencilBits > 0)
        attribList.pushBack({WGL_STENCIL_BITS_ARB, (int)numStencilBits});
      attribList.pushBack(0);

      int  pixelFormat     = 0;
      UINT numFormatsFound = 0;

      if (!glChoosePixelFormatARB(window.hDC, attribList.data(), 0, 1, &pixelFormat, &numFormatsFound))
        return false;

      PIXELFORMATDESCRIPTOR pfd = {0};
      memset(&pfd, 0, sizeof(pfd));
      DescribePixelFormat(window.hDC, pixelFormat, sizeof(pfd), &pfd);

      return SetPixelFormat(window.hDC, pixelFormat, &pfd);
    }

    void GLRenderTarget::attachColour(TextureRef textureID, int64_t slot, int64_t mipLevel, int64_t layer) {
      bool changed = false;
      changed |= textures.colour[slot].texture != textureID;
      changed |= textures.colour[slot].mipLevel != mipLevel;
      changed |= textures.colour[slot].layer != layer;
      textures.colour[slot].texture  = textureID;
      textures.colour[slot].mipLevel = mipLevel;
      textures.colour[slot].layer    = layer;
      textures.rebind |= changed;
    }

    void GLRenderTarget::attachDepth(TextureRef textureID, int64_t mipLevel, int64_t layer) {
      bool changed = false;
      changed |= textures.depth.texture != textureID;
      changed |= textures.depth.mipLevel != mipLevel;
      changed |= textures.depth.layer != layer;
      textures.depth.texture  = textureID;
      textures.depth.mipLevel = mipLevel;
      textures.depth.layer    = layer;
      textures.rebind |= changed;
    }

    void GLRenderTarget::setReadAttachment(int64_t slot) {
      bool changed = false;
      changed |= textures.colourReadAttachment != slot;
      textures.colourReadAttachment = slot;
      textures.rebind               = changed;
    }

    void StateManager_OpenGL::apply(State const & state) {
      state.visit(overloaded([](State::EnableBlend const & state) { SetGLStateEnabled(GL_BLEND, state.enabled); },
                             [](State::EnableStencilTest const & state) { SetGLStateEnabled(GL_STENCIL_TEST, state.enabled); },
                             [](State::EnableScissorTest const & state) { SetGLStateEnabled(GL_SCISSOR_TEST, state.enabled); },
                             [](State::EnableDepthRead const & state) { SetGLStateEnabled(GL_DEPTH_TEST, state.enabled); },
                             [](State::EnableDepthWrite const & state) { glDepthMask(state.enabled ? GL_TRUE : GL_FALSE); },
                             [](State::Viewport const & state) { glViewport(state.position.x, state.position.y, state.size.x, state.size.y); },
                             [](State::Scissor const & state) { glScissor(state.position.x, state.position.y, state.size.x, state.size.y); },
                             [](State::DepthRange const & state) { glDepthRange(state.min, state.max); },
                             [](State::DepthFunc const & state) { glDepthFunc(ToGLComparison(state.comparison)); },
                             [](State::BlendFunc const & state) {
                               GLenum glSrcColourFactor = ToGLBlendFunction(state.sourceColourFactor);
                               GLenum glSrcAlphaFactor  = ToGLBlendFunction(state.sourceAlphaFactor);
                               GLenum glDstColourFactor = ToGLBlendFunction(state.destColourFactor);
                               GLenum glDstAlphaFactor  = ToGLBlendFunction(state.destAlphaFactor);
                               if (state.colourAttachment.has_value())
                                 glBlendFuncSeparatei((GLuint)state.colourAttachment.value(), glSrcColourFactor, glSrcAlphaFactor, glDstColourFactor,
                                                      glDstAlphaFactor);
                               else
                                 glBlendFuncSeparate(glSrcColourFactor, glSrcAlphaFactor, glDstColourFactor, glDstAlphaFactor);
                             },
                             [](State::BlendEq const & state) {
                               GLenum glColourEq = ToGLEquation(state.colour);
                               GLenum glAlphaEq  = ToGLEquation(state.alpha);
                               if (state.colourAttachment.has_value())
                                 glBlendEquationSeparatei((GLuint)state.colourAttachment.value(), glColourEq, glAlphaEq);
                               else
                                 glBlendEquationSeparate(glColourEq, glAlphaEq);
                             },
                             [](State::ColourWrite const & state) { glColorMask(state.r, state.g, state.b, state.a); },
                             [](State::ColourFactor const & state) { glBlendColor(state.r, state.g, state.b, state.a); },
                             [](auto && v) { BFC_FAIL("State is not suported (type: %s)", typeid(v).name()); }));
    }

    CommandList_OpenGL::CommandList_OpenGL(GraphicsDevice *pDevice, RenderTargetRef defaultTarget, VertexArrayRef emptyVertexArray, uint32_t lastTextureUnit)
      : m_pDevice(pDevice)
      , m_defaultTarget(defaultTarget)
      , m_emptyVertexArray(emptyVertexArray)
      , m_lastTextureUnit(lastTextureUnit)
    {}

    void CommandList_OpenGL::bindProgram(ProgramRef programID) {
      impl::OpenGL::BindProgram cmd;
      cmd.pProgram = &ToGL(programID);
      add(cmd);
      track(programID);

      m_boundProgram = programID;
    }

    void CommandList_OpenGL::bindVertexArray(VertexArrayRef vertexArrayID) {
      if (vertexArrayID == InvalidGraphicsResource) {
        bindVertexArray(m_emptyVertexArray);
        return;
      }

      GLVertexArray & va = ToGL(vertexArrayID);
      impl::OpenGL::BindVertexArray cmd;
      cmd.pVertexArray = &va;
      add(cmd);
      track(vertexArrayID);

      int64_t numElements = va.layout.getAttributeCount();
      m_vertexCount       = std::numeric_limits<int64_t>::max();
      for (int64_t i = 0; i < numElements; ++i) {
        auto const & elm          = va.layout.getAttributeLayout(i);
        auto         vertexBuffer = ToGL(va.vertexBuffers[elm.slot]);
        m_vertexCount             = std::min(m_vertexCount, vertexBuffer.size / elm.stride);
      }

      if (va.indexBuffer != InvalidGraphicsResource) {
        m_vaIndexType = va.indexBufferType;
        m_indexCount  = ToGL(va.indexBuffer).size / getDataTypeSize(m_vaIndexType);
      }

      if (va.rebind) {
        impl::OpenGL::RebindVertexArray rebind;
        rebind.pVertexArray = &va;
        rebind.numElements = va.layout.getAttributeCount();
        for (int64_t i = 0; i < rebind.numElements; ++i) {
          String const &                           semantic = va.layout.getAttributeSemantic(i);
          impl::OpenGL::RebindVertexArray::Element item;
          strcpy_s(item.name, semantic.begin());
          item.element = va.layout.getAttributeLayout(i);

          if (i == 0) // Store the first item
            rebind.elements = m_commandBuffer.serialize(item);
          else
            m_commandBuffer.serialize(item);
        }

        for (auto const & [i, vb] : enumerate(va.vertexBuffers)) {
          rebind.vertexBuffers[i] = &ToGL(vb);
          track(vb);
        }
       
        rebind.pIndexBuffer = &ToGL(va.indexBuffer);
        track(va.indexBuffer);
        va.rebind = false;
      }
    }

    void CommandList_OpenGL::bindTexture(TextureRef textureID, int64_t textureUnit) {
      impl::OpenGL::BindTexture cmd;
      cmd.pTexture  = nullptr;
      cmd.target    = GL_TEXTURE_2D;

      if (textureID != InvalidGraphicsResource) {
        cmd.pTexture = &ToGL(textureID);
        cmd.target   = ToGLTextureType(cmd.pTexture->type);
      }

      cmd.pTexture    = textureID == InvalidGraphicsResource ? nullptr : &ToGL(textureID);
      cmd.textureUnit = (GLenum)(GL_TEXTURE0 + textureUnit);

      add(cmd);
      track(textureID);
    }

    void CommandList_OpenGL::bindSampler(SamplerRef samplerID, int64_t textureUnit) {
      if (samplerID == InvalidGraphicsResource) {
        impl::OpenGL::BindSampler cmd;
        cmd.pSampler    = nullptr;
        cmd.textureUnit = (GLenum)(GL_TEXTURE0 + textureUnit);
        add(cmd);
      } else {
        auto & sampler = ToGL(samplerID);

        if (sampler.changed) {
          impl::OpenGL::UpdateSampler update;
          update.pSampler = &sampler;
          update.minFilter = ToGLFilterMode(sampler.minFilter, sampler.minMipFilter);
          update.magFilter = ToGLFilterMode(sampler.magFilter, sampler.magMipFilter);
          update.minLOD    = sampler.minLOD;
          update.maxLOD    = sampler.maxLOD;
          update.wrapMode  = {
            ToGLWrapMode(sampler.wrapMode.x),
            ToGLWrapMode(sampler.wrapMode.y),
            ToGLWrapMode(sampler.wrapMode.z),
          };
          add(update);

          sampler.changed = false;
        }

        impl::OpenGL::BindSampler cmd;
        cmd.pSampler    = &sampler;
        cmd.textureUnit = (GLenum)(GL_TEXTURE0 + textureUnit);

        add(cmd);
        track(samplerID);
      }
    }

    void CommandList_OpenGL::bindUniformBuffer(BufferRef bufferID, int64_t bindPoint, int64_t offset, int64_t size) {
      impl::OpenGL::BindUniformBuffer cmd;
      cmd.pBuffer   = nullptr;
      cmd.bindPoint = (GLint)bindPoint;
      cmd.offset    = (GLintptr)offset;
      cmd.size      = (GLintptr)size;
      cmd.target    = GL_UNIFORM_BUFFER;
      if (bufferID != InvalidGraphicsResource) {
        if (cmd.size == 0) {
          cmd.size = bufferID->getSize() - offset;
        }
        if (cmd.size >= 0) {
          cmd.pBuffer = &ToGL(bufferID);
        }
      }
      add(cmd);
      track(bufferID);
    }

    void CommandList_OpenGL::bindShaderStorageBuffer(BufferRef bufferID, int64_t bindPoint, int64_t offset, int64_t size) {
      impl::OpenGL::BindUniformBuffer cmd;
      cmd.pBuffer   = nullptr;
      cmd.bindPoint = (GLint)bindPoint;
      cmd.offset    = (GLintptr)offset;
      cmd.size      = (GLintptr)size;
      cmd.target    = GL_SHADER_STORAGE_BUFFER;
      if (bufferID != InvalidGraphicsResource) {
        if (cmd.size == 0) {
          cmd.size = bufferID->getSize() - offset;
        }
        if (cmd.size >= 0) {
          cmd.pBuffer = &ToGL(bufferID);
        }
      }
      add(cmd);
      track(bufferID);
    }

    void CommandList_OpenGL::bindRenderTarget(RenderTargetRef renderTargetID, MapAccess renderTargetAccess) {
      if (renderTargetID == InvalidGraphicsResource) {
        return bindRenderTarget(m_defaultTarget, renderTargetAccess);
      }

      auto & rt = ToGL(renderTargetID);

      if (rt.type == RenderTargetType_Texture) {
        if (rt.textures.rebind) {
          impl::OpenGL::RebindTextureRenderTarget cmd;
          cmd.fbClass              = TextureType_Unknown;
          cmd.colourReadAttachment = GLenum(GL_COLOR_ATTACHMENT0 + rt.textures.colourReadAttachment);
          cmd.access               = renderTargetAccess;

          for (int64_t index = 0; index < MaxColourAttachments; ++index) {
            if (rt.textures.colour[index].texture != InvalidGraphicsResource) {
              cmd.colour[index].pTexture = &ToGL(rt.textures.colour[index].texture);
              cmd.colour[index].layer    = rt.textures.colour[index].layer;
              cmd.colour[index].mipLevel = rt.textures.colour[index].mipLevel;
              track(rt.textures.colour[index].texture);

              if (cmd.fbClass == TextureType_Unknown) {
                cmd.fbClass = rt.textures.colour[index].texture->getType();
              }
            } else {
              cmd.colour[index].pTexture = InvalidGraphicsResource;
            }
          }

          if (rt.textures.depth.texture != InvalidGraphicsResource) {
            cmd.depth.pTexture = &ToGL(rt.textures.depth.texture);
            cmd.depth.layer    = rt.textures.depth.layer;
            cmd.depth.mipLevel = rt.textures.depth.mipLevel;
            track(rt.textures.depth.texture);

            if (cmd.fbClass == TextureType_Unknown) {
              cmd.fbClass = rt.textures.depth.texture->getType();
            }
          } else {
            cmd.depth.pTexture = InvalidGraphicsResource;
          }

          add(cmd);
        } else {
          impl::OpenGL::BindTextureRenderTarget cmd;
          cmd.pRenderTarget = &rt;
          cmd.access        = renderTargetAccess;
          add(cmd);
        }
      } else {
        impl::OpenGL::BindWindowRenderTarget cmd;
        cmd.hDC    = rt.window.hDC;
        cmd.access = renderTargetAccess;
        add(cmd);

        m_activeWindowTarget = renderTargetID;
      }

      track(renderTargetID);
    }

    void CommandList_OpenGL::bindScreen(MapAccess renderTargetAccess) {
      bindRenderTarget(m_defaultTarget, renderTargetAccess);
    }

    void CommandList_OpenGL::draw(int64_t elementCount, int64_t elementOffset, PrimitiveType primType, int64_t instanceCount) {
      impl::OpenGL::Draw cmd;
      cmd.elementCount = elementCount;
      cmd.elementOffset = elementOffset;
      cmd.primType      = ToGLPrimType(primType);
      cmd.instanceCount = instanceCount;

      // Only validate elementCount if a vertex array has been bound.
      // This is to support shaders which may not take any vertex inputs.
      // However, you must explicity bind an Invalid vertex array.
      if (m_vertexCount >= 0) {
        cmd.elementCount = std::min(elementCount, m_vertexCount - elementOffset);
        if (cmd.elementCount < 0)
          return;
      }

      add(cmd);
    }

    void CommandList_OpenGL::drawIndexed(int64_t elementCount, int64_t elementOffset, PrimitiveType primType, int64_t instanceCount) {
      impl::OpenGL::DrawIndexed cmd;
      cmd.elementCount = elementCount;
      cmd.elementOffset = elementOffset * getDataTypeSize(m_vaIndexType);
      cmd.indexType     = ToGLPrimType(primType);
      cmd.instanceCount = instanceCount;

      // Only validate elementCount if a vertex array has been bound.
      // This is to support shaders which may not take any vertex inputs.
      // However, you must explicity bind an Invalid vertex array.
      if (m_indexCount >= 0) {
        cmd.elementCount = std::min(elementCount, m_indexCount - elementOffset);
        if (cmd.elementCount < 0)
          return;
      }

      add(cmd);
    }

    void CommandList_OpenGL::clear(RGBAu8 colour) {
      impl::OpenGL::Clear cmd;
      cmd.colour = colour;

      add(cmd);
    }

    void CommandList_OpenGL::swap() {
      impl::OpenGL::Swap cmd;
      cmd.hDC = ToGL(m_activeWindowTarget).window.hDC;

      add(cmd);
    }

    void CommandList_OpenGL::setState(Span<const State> const & state) {
      if (state.size() == 0)
        return;

      impl::OpenGL::SetState cmd;
      cmd.count = state.size();
      cmd.states = m_commandBuffer.serialize(state[0]);
      for (int64_t i = 1; i < cmd.count; ++i)
        m_commandBuffer.serialize(state[i]);
      add(cmd);
    }

    void CommandList_OpenGL::pushState(Span<const State> const & state) {
      if (state.size() == 0)
        return;

      impl::OpenGL::PushState cmd;
      cmd.count  = state.size();
      cmd.states = m_commandBuffer.serialize(state[0]);
      for (int64_t i = 1; i < cmd.count; ++i)
        m_commandBuffer.serialize(state[i]);
      add(cmd);
    }

    void CommandList_OpenGL::popState() {
      add(impl::OpenGL::PopState{});
    }

    bool CommandList_OpenGL::upload(BufferRef bufferID, int64_t size, void const * pData) {
      impl::OpenGL::Upload cmd;
      cmd.dataHandle = m_commandBuffer.write({(const uint8_t*)pData, size});
      cmd.dataSize   = size;
      cmd.pGLBuffer    = &ToGL(bufferID);
      add(cmd);
      track(bufferID);

      cmd.pGLBuffer->size = size;

      return true;
    }

    std::future<void*> CommandList_OpenGL::map(BufferRef bufferID, MapAccess access) {
      return map(bufferID, 0, ToGL(bufferID).size, access);
    }

    std::future<void *> CommandList_OpenGL::map(BufferRef bufferID, int64_t offset, int64_t size, MapAccess access) {
      auto pPromise = bfc::Ref<std::promise<void *>>();

      impl::OpenGL::Map cmd;
      cmd.pBuffer = &ToGL(bufferID);
      cmd.offset  = (GLintptr)offset;
      cmd.size    = (GLsizeiptr)size;
      cmd.access  = ToGLMapBits(access);
      cmd.pPromise = pPromise.get();

      add(cmd);
      track(bufferID);
      track(pPromise);
    }

    void CommandList_OpenGL::unmap(BufferRef bufferID) {
      impl::OpenGL::Unmap cmd;
      cmd.pBuffer = &ToGL(bufferID);
      add(cmd);
      track(bufferID);
    }

    void CommandList_OpenGL::download(BufferRef bufferID, BufferDownloadRef pDownload, int64_t offset, int64_t size) {
      auto pPromise = bfc::Ref<std::promise<void>>();

      impl::OpenGL::Download cmd;
      cmd.pBuffer = &ToGL(bufferID);
      cmd.pDownload = &ToGL(pDownload);
      cmd.offset  = offset;
      cmd.size    = size;
      if (cmd.size == 0)
        cmd.size = std::max(0ll, cmd.pBuffer->size - offset);
      cmd.pPromise            = pPromise.get();
      cmd.pDownload->complete = pPromise->get_future();
      cmd.pDownload->storage.resize(cmd.size);

      add(cmd);
      track(bufferID);
      track(pPromise);
    }

    bool CommandList_OpenGL::uploadTexture(TextureRef textureID, DepthStencilFormat format, Vec3i size) {
      impl::OpenGL::AllocateTexture cmd;
      cmd.pTexture       = &ToGL(textureID);
      cmd.format         = ToGLFormat(format);
      cmd.type           = ToGLType(format);
      cmd.internalFormat = ToGLInternalFormat(format);
      cmd.target         = ToGLTextureType(cmd.pTexture->type);
      cmd.size           = size;

      cmd.pTexture->depthStencilFmt = format;
      cmd.pTexture->format          = PixelFormat_Unknown;
      cmd.pTexture->size            = size;

      add(cmd);
      track(textureID);

      return true;
    }

    bool CommandList_OpenGL::uploadTexture(TextureRef textureID, media::Surface const & src) {
      auto & tex = ToGL(textureID);

      impl::OpenGL::UploadTexture cmd;
      cmd.pTexture        = &tex;
      cmd.format          = ToGLFormat(src.format);
      cmd.type            = ToGLType(src.format);
      cmd.internalFormat  = ToGLInternalFormat(src.format);
      cmd.target          = ToGLTextureType(tex.type);
      cmd.surface         = src;
      cmd.surface.pBuffer = nullptr;
      cmd.dataSize        = media::calculateSurfaceSize(src); 
      cmd.dataHandle      = m_commandBuffer.write({(const uint8_t *)src.pBuffer, cmd.dataSize});

      add(cmd);
      track(textureID);

      if (cmd.dataHandle != -1) {
        generateMipMaps(textureID);
      }

      tex.depthStencilFmt = DepthStencilFormat_Unknown;
      tex.format          = src.format;
      tex.size            = src.size;

      return true;
    }

    bool CommandList_OpenGL::uploadTextureSubData(TextureRef textureID, media::Surface const & src, Vec3i offset) {
      auto & tex = *(GLTexture *)textureID.get();

      impl::OpenGL::UploadTextureSubData cmd;
      cmd.pTexture        = &tex;
      cmd.format          = ToGLFormat(src.format);
      cmd.type            = ToGLType(src.format);
      cmd.target          = ToGLTextureType(tex.type);
      cmd.surface         = src;
      cmd.surface.pBuffer = nullptr;
      cmd.dataSize        = media::calculateSurfaceSize(src);
      cmd.dataHandle      = m_commandBuffer.write({(const uint8_t *)src.pBuffer, cmd.dataSize});
      cmd.offset          = offset;

      add(cmd);
      track(textureID);

      return true;
    }

    void CommandList_OpenGL::generateMipMaps(TextureRef textureID) {
      impl::OpenGL::UploadTexture mips;
      mips.pTexture = &ToGL(textureID);
      add(mips);
      track(textureID);
    }

    bool CommandList_OpenGL::downloadTexture(TextureRef textureID, TextureDownloadRef pDownload) {
      BFC_FAIL("Not implemented");

      impl::OpenGL::DownloadTexture cmd;
      cmd.pTexture = &ToGL(textureID);
      track(textureID);
    }

    void CommandList_OpenGL::setUniform(int64_t uniformIndex, void const * pBuffer, int64_t size) {
      impl::OpenGL::SetUniform cmd;
      cmd.uniformIndex = uniformIndex;
      cmd.dataSize     = size;
      cmd.dataHandle   = m_commandBuffer.write({(const uint8_t *)pBuffer, size});
      cmd.pProgram     = &ToGL(m_boundProgram);

      add(cmd);
    }

    void CommandList_OpenGL::setBufferBinding(int64_t bufferIndex, int64_t bindPoint) {
      impl::OpenGL::SetBufferBinding cmd;
      cmd.bufferIndex  = bufferIndex;
      cmd.bindPoint    = bindPoint;
      cmd.pProgram     = &ToGL(m_boundProgram);

      add(cmd);
    }

    void CommandList_OpenGL::setTextureBinding(int64_t textureIndex, int64_t bindPoint) {
      impl::OpenGL::SetBufferBinding cmd;
      cmd.bufferIndex = textureIndex;
      cmd.bindPoint   = bindPoint;
      cmd.pProgram    = &ToGL(m_boundProgram);

      add(cmd);
    }

    // void CommandList_OpenGL::getUniform(int64_t uniformIndex, void * pBuffer, ProgramUniformDesc * pDesc) {
    //   GLProgram &          prog        = *((GLProgram *)m_boundProgram.get());
    //   GLProgram::Uniform & uniformDesc = prog.uniforms[uniformIndex];
    // 
    //   if (pDesc != nullptr) {
    //     prog.getUniformDesc(uniformIndex, pDesc);
    //   }
    // 
    //   if (pBuffer != nullptr) {
    //     switch (uniformDesc.type) {
    //     case DataType_Float32: glGetUniformfv(prog.glID, uniformDesc.glLoc, (float *)pBuffer); break;
    //     case DataType_Int32: glGetUniformiv(prog.glID, uniformDesc.glLoc, (int32_t *)pBuffer); break;
    //     case DataType_UInt32: glGetUniformuiv(prog.glID, uniformDesc.glLoc, (uint32_t *)pBuffer); break;
    //     }
    //   }
    // }
    // 
    // int64_t CommandList_OpenGL::getBufferBinding(int64_t bufferIndex) {
    //   GLProgram & prog = *((GLProgram *)m_boundProgram.get());
    //   return prog.buffers[bufferIndex].bindPoint;
    // }
    // 
    // int64_t CommandList_OpenGL::getTextureBinding(int64_t textureIndex) {
    //   GLProgram & prog = *((GLProgram *)m_boundProgram.get());
    //   return prog.textures[textureIndex].bindPoint;
    // }

    GraphicsDevice * CommandList_OpenGL::getDevice() const {
      return m_pDevice;
    }

  } // namespace graphics

  GraphicsDevice_OpenGL::GraphicsDevice_OpenGL() {}

  bool GraphicsDevice_OpenGL::init(platform::Window * pWindow) {
    // Create a temporary window to make our fake GL context
    HINSTANCE hInstance = GetModuleHandle(0);

    // Register Window
    WNDCLASSEX cls    = {0};
    cls.cbSize        = sizeof(WNDCLASSEX);
    cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hInstance     = hInstance;
    cls.hIcon         = 0;
    cls.hIconSm       = 0;
    cls.hCursor       = LoadCursor(0, IDC_ARROW);
    cls.lpszClassName = "glTmpWindow";
    cls.style         = 0;
    cls.lpfnWndProc   = DefWindowProc;
    ATOM clsAtom      = RegisterClassEx(&cls);

    // Create Window
    HWND hTempWnd        = CreateWindowEx(0, MAKEINTATOM(clsAtom), "Window", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, 0, 0, hInstance, 0);
    HDC  hTempDC         = GetDC(hTempWnd);
    int  tempPixelFormat = ChoosePixelFormat(hTempDC, &g_defaultPfd);

    SetPixelFormat(hTempDC, tempPixelFormat, &g_defaultPfd);
    HGLRC hTempGLRC = wglCreateContext(hTempDC);
    wglMakeCurrent(hTempDC, hTempGLRC);

    glChoosePixelFormatARB    = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
    glCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));

    // Destroy the temporary context
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(hTempGLRC);
    ReleaseDC(hTempWnd, hTempDC);

    const int major_min        = 4;
    const int minor_min        = 5;
    int       contextAttribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,    major_min, WGL_CONTEXT_MINOR_VERSION_ARB, minor_min, WGL_CONTEXT_PROFILE_MASK_ARB,
                            WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0};

    m_defaultTarget = createRenderTarget(RenderTargetType_Window);
    if (!m_defaultTarget->attachWindow(pWindow, DepthStencilFormat_D24S8))
      return false;

    HDC hDC = ((graphics::GLRenderTarget *)m_defaultTarget.get())->window.hDC;

    // Create the render target for the actual window
    HGLRC hGLRC = glCreateContextAttribsARB(hDC, nullptr, contextAttribs);

    // Make the new context current
    if (hGLRC) {
      wglMakeCurrent(hDC, hGLRC);
      wglewInit();
      glewInit();
    }

#ifdef _DEBUG // Enable error logging in debug
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(ErrorMessageCallback, 0);
#endif

    m_hGLRC = hGLRC;

    // Enable seamless cube-map sampling
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int64_t val = 0;
    glGetInteger64v(GL_MAX_TEXTURE_IMAGE_UNITS, &val);
    m_lastTextureUnit = (uint32_t)(val - 1);

    m_emptyVertexArray = createVertexArray();

    return true;
  }

  graphics::CommandListRef GraphicsDevice_OpenGL::createCommandList() {
    return NewRef<graphics::CommandList_OpenGL>(this, m_defaultTarget, m_emptyVertexArray, m_lastTextureUnit);
  }

  graphics::BufferRef GraphicsDevice_OpenGL::createBuffer(BufferUsageHint usageHint) {
    // TODO: Custom allocator?
    Ref<graphics::GLBuffer> newBuffer(new graphics::GLBuffer, [this](graphics::GLBuffer * pBuffer) {
      // TODO: Queue the glID for deletion.
      glDeleteBuffers(1, &pBuffer->glID);
      delete pBuffer;
    });

    if ((usageHint & BufferUsageHint_Dynamic) > 0)
      newBuffer->glUsage = GL_DYNAMIC_DRAW;
    else
      newBuffer->glUsage = GL_STATIC_DRAW;

    if ((usageHint & BufferUsageHint_Uniform) > 0)
      newBuffer->defaultTarget = GL_UNIFORM_BUFFER;
    else if ((usageHint & BufferUsageHint_Storage) > 0)
      newBuffer->defaultTarget = GL_SHADER_STORAGE_BUFFER;
    else if ((usageHint & BufferUsageHint_Vertices) > 0 || (usageHint & BufferUsageHint_Indices) > 0)
      newBuffer->defaultTarget = GL_ARRAY_BUFFER;
    else
      newBuffer->defaultTarget = GL_NONE;

    return newBuffer;
  }

  graphics::VertexArrayRef GraphicsDevice_OpenGL::createVertexArray() {
    Ref<graphics::GLVertexArray> va(new graphics::GLVertexArray, [this](graphics::GLVertexArray * pPtr) {
      glDeleteVertexArrays(1, &pPtr->glID);
      delete pPtr;
    });

    return va;
  }

  graphics::ProgramRef GraphicsDevice_OpenGL::createProgram() {
    Ref<graphics::GLProgram> newProgram(new graphics::GLProgram, [](graphics::GLProgram * pPtr) {
      glDeleteProgram(pPtr->glID);
      delete pPtr;
    });

    return newProgram;
  }

  graphics::TextureRef GraphicsDevice_OpenGL::createTexture(TextureType type) {
    Ref<graphics::GLTexture> newTexture(new graphics::GLTexture, [](graphics::GLTexture * pPtr) {
      glDeleteTextures(1, &pPtr->glID);
      delete pPtr;
    });

    newTexture->type = type;
    return newTexture;
  }

  graphics::SamplerRef GraphicsDevice_OpenGL::createSampler() {
    Ref<graphics::GLSampler> newSampler(new graphics::GLSampler, [](graphics::GLSampler * pPtr) {
      glDeleteSamplers(1, &pPtr->glID);
      delete pPtr;
    });

    return newSampler;
  }

  graphics::RenderTargetRef GraphicsDevice_OpenGL::createRenderTarget(RenderTargetType type) {
    Ref<graphics::GLRenderTarget> target(new graphics::GLRenderTarget, [this](graphics::GLRenderTarget * pPtr) {
      if (pPtr->type == RenderTargetType_Texture) {
        glDeleteFramebuffers(1, &pPtr->textures.glID);
      }
      delete pPtr;
    });

    target->type = type;

    return target;
  }

  void GraphicsDevice_OpenGL::destroy() {
    delete this;
  }

  graphics::StateManager * GraphicsDevice_OpenGL::getStateManager() {
    return &m_stateManager;
  }

  uint64_t GraphicsDevice_OpenGL::submit(std::unique_ptr<graphics::CommandList> const & pCommandList) {
    m_queueLock.lock();
    uint64_t commandList = ++m_nextCommandListID;
    m_commandListQueue.pushBack(std::move(pCommandList));
    m_queueLock.unlock();
    m_queueNotifier.notify_one();
    return commandList;
  }

  bool GraphicsDevice_OpenGL::wait(uint64_t handle, std::optional<Timestamp> const & timeout) {
    std::unique_lock guard{m_fenceLock};
    if (timeout.has_value())
      return m_fenceNotifier.wait_for(guard, (std::chrono::microseconds)timeout.value(), [=]() { return m_commandListFence >= handle; });

    m_fenceNotifier.wait(guard, [=]() { return m_commandListFence >= handle; });
    return true;
  }

  void GraphicsDevice_OpenGL::RenderThread() {
    bool                                           running = false;
    Vector<std::unique_ptr<graphics::CommandList>> lists;
    while (m_running) {
      std::unique_lock guard{m_queueLock};
      m_queueNotifier.wait(guard, [&]() {
        running = m_running || m_commandListQueue.size() > 0;
        lists   = std::move(m_commandListQueue);
        return !running || lists.size() > 0;
      });

      for (auto &list : lists)
        list->
    }
  }

  static graphics::GLBuffer & ToGL(graphics::BufferRef pBuffer) {
    return *(graphics::GLBuffer *)pBuffer.get();
  }

  static graphics::GLBufferDownload & ToGL(graphics::BufferDownloadRef pBuffer) {
    return *(graphics::GLBufferDownload *)pBuffer.get();
  }

  static graphics::GLTexture & ToGL(graphics::TextureRef pTexture) {
    return *(graphics::GLTexture *)pTexture.get();
  }

  static graphics::GLTextureDownload & ToGL(graphics::TextureDownloadRef pBuffer) {
    return *(graphics::GLTextureDownload *)pBuffer.get();
  }

  static graphics::GLSampler & ToGL(graphics::SamplerRef pSampler) {
    return *(graphics::GLSampler *)pSampler.get();
  }

  static graphics::GLRenderTarget & ToGL(graphics::RenderTargetRef pRenderTarget) {
    return *(graphics::GLRenderTarget *)pRenderTarget.get();
  }

  static graphics::GLProgram & ToGL(graphics::ProgramRef pProgram) {
    return *(graphics::GLProgram *)pProgram.get();
  }

  static graphics::GLVertexArray & ToGL(graphics::VertexArrayRef pVertexArray) {
    return *(graphics::GLVertexArray *)pVertexArray.get();
  }

  void GLAPIENTRY ErrorMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar * message, const void * userParam) {
    switch (type) {
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    case GL_DEBUG_TYPE_PORTABILITY:
    case GL_DEBUG_TYPE_PERFORMANCE:
    case GL_DEBUG_TYPE_ERROR: printf("OpenGL Error: %s\n", message); break;
    }
  }

  GLenum ToGLShaderType(ShaderType type) {
    switch (type) {
    case ShaderType_Vertex: return GL_VERTEX_SHADER;
    case ShaderType_Fragment: return GL_FRAGMENT_SHADER;
    case ShaderType_Geometry: return GL_GEOMETRY_SHADER;
    case ShaderType_TessControl: return GL_TESS_CONTROL_SHADER;
    case ShaderType_TessEval: return GL_TESS_EVALUATION_SHADER;
    case ShaderType_Compute: return GL_COMPUTE_SHADER;
    }
    return GL_NONE;
  }

  GLenum ToGLTextureType(TextureType type) {
    switch (type) {
    case TextureType_2D: return GL_TEXTURE_2D;
    case TextureType_2DArray: return GL_TEXTURE_2D_ARRAY;
    case TextureType_3D: return GL_TEXTURE_3D;
    case TextureType_CubeMap: return GL_TEXTURE_CUBE_MAP;
    }
    return GL_NONE;
  }

  GLenum ToGLAccess(MapAccess access) {
    if (access == MapAccess_Read) {
      return GL_READ_ONLY;
    }
    if (access == MapAccess_Write) {
      return GL_WRITE_ONLY;
    }
    if (access == MapAccess_ReadWrite) {
      return GL_READ_WRITE;
    }
    return GL_NONE;
  }

  GLenum ToGLMapBits(MapAccess access) {
    GLenum mapBits = 0;
    if (access & MapAccess_Read) {
      mapBits |= GL_MAP_READ_BIT;
    }
    if (access == MapAccess_Write) {
      mapBits |= GL_MAP_WRITE_BIT;
    }

    return mapBits;
  }
  GLenum ToGLInternalFormat(PixelFormat pixelFormat) {
    switch (pixelFormat) {
    case PixelFormat_Ru8: // Fall-through
    case PixelFormat_Lu8: return GL_R8;
    case PixelFormat_RGBu8: return GL_RGB8;
    case PixelFormat_RGBAu8: return GL_RGBA8;
    case PixelFormat_RGBAu16: return GL_RGBA16;
    case PixelFormat_LAu8: return GL_RG8;
    case PixelFormat_RGBAf16: return GL_RGBA16F;
    case PixelFormat_RGBAf32: return GL_RGBA32F;
    case PixelFormat_Rf32: return GL_R32F;
    }
    return GL_NONE;
  }

  GLenum ToGLInternalFormat(DepthStencilFormat depthStencilFormat) {
    switch (depthStencilFormat) {
    case DepthStencilFormat_D24S8: return GL_DEPTH24_STENCIL8;
    case DepthStencilFormat_D16: return GL_DEPTH_COMPONENT16;
    case DepthStencilFormat_D32: return GL_DEPTH_COMPONENT32F;
    }
    return GL_NONE;
  }

  GLenum ToGLFormat(PixelFormat pixelFormat) {
    switch (pixelFormat) {
    case PixelFormat_Ru8: // Fall-through
    case PixelFormat_Lu8: // Fall-through
    case PixelFormat_Rf32: return GL_RED;
    case PixelFormat_RGBu8: return GL_RGB;
    case PixelFormat_RGBAf32: // Fall-through
    case PixelFormat_RGBAf16: // Fall-through
    case PixelFormat_RGBAu16: // Fall-through
    case PixelFormat_RGBAu8: return GL_RGBA;
    case PixelFormat_LAu8: return GL_RG;
    }
    return GL_NONE;
  }

  GLenum ToGLFormat(DepthStencilFormat depthStencilFormat) {
    switch (depthStencilFormat) {
    case DepthStencilFormat_D16: // Fall-through
    case DepthStencilFormat_D32: return GL_DEPTH_COMPONENT;
    case DepthStencilFormat_D24S8: return GL_DEPTH_STENCIL;
    }
    return GL_NONE;
  }

  GLenum ToGLType(PixelFormat pixelFormat) {
    switch (pixelFormat) {
    case PixelFormat_Ru8:    // Fall-through
    case PixelFormat_RGBu8:  // Fall-through
    case PixelFormat_RGBAu8: // Fall-through
    case PixelFormat_Lu8:    // Fall-through
    case PixelFormat_LAu8: return GL_UNSIGNED_BYTE;
    case PixelFormat_RGBAu16: return GL_UNSIGNED_SHORT;
    case PixelFormat_RGBAf32: // Fall-through
    case PixelFormat_RGBAf16: // Fall-through
    case PixelFormat_Rf32: return GL_FLOAT;
    }
    return GL_NONE;
  }

  GLenum ToGLType(DepthStencilFormat depthStencilFormat) {
    switch (depthStencilFormat) {
    case DepthStencilFormat_D16: return GL_HALF_FLOAT;
    case DepthStencilFormat_D24S8: return GL_UNSIGNED_INT_24_8;
    case DepthStencilFormat_D32: return GL_FLOAT;
    }
    return GL_NONE;
  }

  GLenum ToGLDataType(DataType type) {
    switch (type) {
    case DataType_Bool: return GL_BOOL;
    case DataType_Int8: return GL_BYTE;
    case DataType_UInt8: return GL_UNSIGNED_BYTE;
    case DataType_Int16: return GL_SHORT;
    case DataType_UInt16: return GL_UNSIGNED_SHORT;
    case DataType_Int32: return GL_INT;
    case DataType_UInt32: return GL_UNSIGNED_INT;
    case DataType_Float32: return GL_FLOAT;
    case DataType_Float64: return GL_DOUBLE;
    }

    return GL_NONE;
  }

  GLenum ToGLPrimType(PrimitiveType primType) {
    switch (primType) {
    case PrimitiveType_Line: return GL_LINES;
    case PrimitiveType_Point: return GL_POINTS;
    case PrimitiveType_Triangle: return GL_TRIANGLES;
    }
    return GL_NONE;
  }

  GLenum ToGLFramebufferTextureTarget(TextureType texType, int64_t level) {
    switch (texType) {
    case TextureType_2D: return GL_TEXTURE_2D;
    case TextureType_3D: return GL_TEXTURE_3D;
    case TextureType_2DArray: return GL_TEXTURE_2D;
    case TextureType_CubeMap:
      if (level < 6)
        return GLenum(GL_TEXTURE_CUBE_MAP_POSITIVE_X + level);
      else
        return GL_NONE;
    }

    return GL_NONE;
  }

  GLenum ToGLFramebufferAttachment(DepthStencilFormat depthStencilFmt) {
    switch (depthStencilFmt) {
    case DepthStencilFormat_D24S8: return GL_DEPTH_STENCIL_ATTACHMENT;
    case DepthStencilFormat_D16: // Fall-through
    case DepthStencilFormat_D32: return GL_DEPTH_ATTACHMENT;
    }

    return GL_NONE;
  }

  GLenum ToGLEquation(BlendEquation blendEquation) {
    switch (blendEquation) {
    case BlendEquation_Add: return GL_FUNC_ADD;
    case BlendEquation_Subtract: return GL_FUNC_SUBTRACT;
    case BlendEquation_ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
    case BlendEquation_Min: return GL_MIN;
    case BlendEquation_Max: return GL_MAX;
    }
    return GL_NONE;
  }

  GLenum ToGLBlendFunction(BlendFunction function) {
    switch (function) {
    case BlendFunction_Zero: return GL_ZERO;
    case BlendFunction_One: return GL_ONE;
    case BlendFunction_SourceColour: return GL_SRC_COLOR;
    case BlendFunction_SourceAlpha: return GL_SRC_ALPHA;
    case BlendFunction_DestColour: return GL_DST_COLOR;
    case BlendFunction_DestAlpha: return GL_DST_ALPHA;
    case BlendFunction_OneMinusSourceColour: return GL_ONE_MINUS_SRC_COLOR;
    case BlendFunction_OneMinusSourceAlpha: return GL_ONE_MINUS_SRC_ALPHA;
    case BlendFunction_OneMinusDestColour: return GL_ONE_MINUS_DST_COLOR;
    case BlendFunction_OneMinusDestAlpha: return GL_ONE_MINUS_DST_ALPHA;
    case BlendFunction_ConstantAlpha: return GL_CONSTANT_ALPHA;
    case BlendFunction_OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
    }
    return GL_NONE;
  }

  GLenum ToGLComparison(ComparisonFunction function) {
    switch (function) {
    case ComparisonFunction_Equal: return GL_EQUAL;
    case ComparisonFunction_NotEqual: return GL_NOTEQUAL;
    case ComparisonFunction_Less: return GL_LESS;
    case ComparisonFunction_Greater: return GL_GREATER;
    case ComparisonFunction_LessEqual: return GL_GEQUAL;
    case ComparisonFunction_GreaterEqual: return GL_LEQUAL;
    case ComparisonFunction_Always: return GL_ALWAYS;
    case ComparisonFunction_Never: return GL_NEVER;
    }
    return GL_NONE;
  }

  GLenum ToGLCubeMapFace(CubeMapFace face) {
    return face >= 0 && face < CubeMapFace_Count ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : GL_NONE;
  }

  GLenum ToGLWrapMode(WrapMode mode) {
    switch (mode) {
    case WrapMode_Repeat: return GL_REPEAT;
    case WrapMode_MirroredRepeat: return GL_MIRRORED_REPEAT;
    case WrapMode_ClampToEdge: return GL_CLAMP_TO_EDGE;
    case WrapMode_ClampToBorder: return GL_CLAMP_TO_BORDER;
    }
    return GL_NONE;
  }

  GLenum ToGLFilterMode(FilterMode filter, FilterMode mipFilter) {
    switch (mipFilter) {
    case bfc::FilterMode_Linear:
      switch (filter) {
      case bfc::FilterMode_Linear: return GL_LINEAR_MIPMAP_LINEAR;
      case bfc::FilterMode_Nearest: return GL_NEAREST_MIPMAP_LINEAR;
      }
      break;
    case bfc::FilterMode_Nearest:
      switch (filter) {
      case bfc::FilterMode_Linear: return GL_LINEAR_MIPMAP_NEAREST;
      case bfc::FilterMode_Nearest: return GL_NEAREST_MIPMAP_NEAREST;
      }
      break;
    default:
      switch (filter) {
      case bfc::FilterMode_Linear: return GL_LINEAR;
      case bfc::FilterMode_Nearest: return GL_NEAREST;
      }
      break;
    }
    return GL_NONE;
  }

  int64_t GetSemanticLocation(StringView const & semantic) {
    int64_t    numStart  = semantic.findLastNotOf("0123456789");
    StringView namePart  = semantic.substr(0, numStart + 1);
    StringView indexPart = semantic.substr(numStart);

    if (namePart.equals("position", true))
      return 0;
    if (namePart.equals("colour", true))
      return 1;
    if (namePart.equals("texcoord", true))
      return 2;
    if (namePart.equals("normal", true))
      return 3;
    if (namePart.equals("tangent", true))
      return 4;
    return -1;
  }

  bool GetGLTypeDetails(GLenum glType, DataType * pType, DataClass * pClass, int64_t * pWidth, int64_t * pHeight) {
    *pHeight = 1;
    *pWidth  = 1;
    *pType   = DataType_Unknown;
    *pClass  = DataClass_Unknown;
    switch (glType) {
    case GL_FLOAT:
      *pWidth  = 1;
      *pHeight = 1;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Scalar;
      break;
    case GL_FLOAT_VEC2:
      *pWidth  = 2;
      *pHeight = 1;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Vector;
      break;
    case GL_FLOAT_VEC3:
      *pWidth  = 3;
      *pHeight = 1;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Vector;
      break;
    case GL_FLOAT_VEC4:
      *pWidth  = 4;
      *pHeight = 1;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Vector;
      break;
    case GL_FLOAT_MAT2:
      *pWidth  = 2;
      *pHeight = 2;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Matrix;
      break;
    case GL_FLOAT_MAT3:
      *pWidth  = 3;
      *pHeight = 3;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Matrix;
      break;
    case GL_FLOAT_MAT4:
      *pWidth  = 4;
      *pHeight = 4;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Matrix;
      break;
    case GL_FLOAT_MAT2x3:
      *pWidth  = 2;
      *pHeight = 3;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Matrix;
      break;
    case GL_FLOAT_MAT2x4:
      *pWidth  = 2;
      *pHeight = 4;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Matrix;
      break;
    case GL_FLOAT_MAT3x2:
      *pWidth  = 3;
      *pHeight = 2;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Matrix;
      break;
    case GL_FLOAT_MAT3x4:
      *pWidth  = 3;
      *pHeight = 4;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Matrix;
      break;
    case GL_FLOAT_MAT4x2:
      *pWidth  = 4;
      *pHeight = 2;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Matrix;
      break;
    case GL_FLOAT_MAT4x3:
      *pWidth  = 4;
      *pHeight = 3;
      *pType   = DataType_Float32;
      *pClass  = DataClass_Matrix;
      break;
    case GL_DOUBLE:
      *pWidth  = 1;
      *pHeight = 1;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Scalar;
      break;
    case GL_DOUBLE_VEC2:
      *pWidth  = 2;
      *pHeight = 1;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Vector;
      break;
    case GL_DOUBLE_VEC3:
      *pWidth  = 3;
      *pHeight = 1;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Vector;
      break;
    case GL_DOUBLE_VEC4:
      *pWidth  = 4;
      *pHeight = 1;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Vector;
      break;
    case GL_DOUBLE_MAT2:
      *pWidth  = 2;
      *pHeight = 2;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Matrix;
      break;
    case GL_DOUBLE_MAT3:
      *pWidth  = 3;
      *pHeight = 3;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Matrix;
      break;
    case GL_DOUBLE_MAT4:
      *pWidth  = 4;
      *pHeight = 4;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Matrix;
      break;
    case GL_DOUBLE_MAT2x3:
      *pWidth  = 2;
      *pHeight = 3;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Matrix;
      break;
    case GL_DOUBLE_MAT2x4:
      *pWidth  = 2;
      *pHeight = 4;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Matrix;
      break;
    case GL_DOUBLE_MAT3x2:
      *pWidth  = 3;
      *pHeight = 2;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Matrix;
      break;
    case GL_DOUBLE_MAT3x4:
      *pWidth  = 3;
      *pHeight = 4;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Matrix;
      break;
    case GL_DOUBLE_MAT4x2:
      *pWidth  = 4;
      *pHeight = 2;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Matrix;
      break;
    case GL_DOUBLE_MAT4x3:
      *pWidth  = 4;
      *pHeight = 3;
      *pType   = DataType_Float64;
      *pClass  = DataClass_Matrix;
      break;
    case GL_INT:
      *pWidth  = 1;
      *pHeight = 1;
      *pType   = DataType_Int32;
      *pClass  = DataClass_Scalar;
      break;
    case GL_INT_VEC2:
      *pWidth  = 2;
      *pHeight = 1;
      *pType   = DataType_Int32;
      *pClass  = DataClass_Vector;
      break;
    case GL_INT_VEC3:
      *pWidth  = 3;
      *pHeight = 1;
      *pType   = DataType_Int32;
      *pClass  = DataClass_Vector;
      break;
    case GL_INT_VEC4:
      *pWidth  = 4;
      *pHeight = 1;
      *pType   = DataType_Int32;
      *pClass  = DataClass_Vector;
      break;
    case GL_UNSIGNED_INT:
      *pWidth  = 1;
      *pHeight = 1;
      *pType   = DataType_UInt32;
      *pClass  = DataClass_Scalar;
      break;
    case GL_UNSIGNED_INT_VEC2:
      *pWidth  = 2;
      *pHeight = 1;
      *pType   = DataType_UInt32;
      *pClass  = DataClass_Vector;
      break;
    case GL_UNSIGNED_INT_VEC3:
      *pWidth  = 3;
      *pHeight = 1;
      *pType   = DataType_UInt32;
      *pClass  = DataClass_Vector;
      break;
    case GL_UNSIGNED_INT_VEC4:
      *pWidth  = 4;
      *pHeight = 1;
      *pType   = DataType_UInt32;
      *pClass  = DataClass_Vector;
      break;
    default: return false;
    }
    return true;
  }

  bool GetGLTextureType(GLenum glType, TextureType * pType) {
    switch (glType) {
    case GL_SAMPLER_1D_ARRAY:
    case GL_SAMPLER_2D: *pType = TextureType_2D; break;
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_3D: *pType = TextureType_3D; break;
    case GL_SAMPLER_CUBE: *pType = TextureType_CubeMap; break;
    default: return false;
    }
    return true;
  }

  void SetGLStateEnabled(GLenum feature, bool enabled) {
    if (enabled)
      glEnable(feature);
    else
      glDisable(feature);
  }
} // namespace bfc
