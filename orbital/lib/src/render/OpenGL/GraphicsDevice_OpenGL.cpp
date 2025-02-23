#include "GraphicsDevice_OpenGL.h"
#include "core/File.h"
#include "platform/Window.h"

namespace bfc {
  bool graphicsDevice_registerOpenGL()
  {
    return registerGraphicsDevice("OpenGL", []() -> Ref<GraphicsDevice> { return NewRef<GraphicsDevice_OpenGL>(); });
  }

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
    GraphicsResource BufferManager_OpenGL::createBuffer(BufferUsageHint usageHint) {
      GLBuffer newBuffer;

      glGenBuffers(1, &newBuffer.glID);
      if (newBuffer.glID == 0) {
        // failed to create buffer
        return InvalidGraphicsResource;
      }

      if ((usageHint & BufferUsageHint_Dynamic) > 0)
        newBuffer.glUsage = GL_DYNAMIC_DRAW;
      else
        newBuffer.glUsage = GL_STATIC_DRAW;

      if ((usageHint & BufferUsageHint_Uniform) > 0)
        newBuffer.defaultTarget = GL_UNIFORM_BUFFER;
      else if ((usageHint & BufferUsageHint_Storage) > 0)
        newBuffer.defaultTarget = GL_SHADER_STORAGE_BUFFER;
      else if ((usageHint & BufferUsageHint_Vertices) > 0 || (usageHint & BufferUsageHint_Indices) > 0)
        newBuffer.defaultTarget = GL_ARRAY_BUFFER;
      else
        newBuffer.defaultTarget = GL_NONE;

      return {(uint32_t)buffers.emplace(newBuffer), GraphicsResourceType_Buffer};
    }

    GraphicsResource BufferManager_OpenGL::refBuffer(GraphicsResource bufferID) {
      buffers.addRef(bufferID.id);
      return bufferID;
    }

    void BufferManager_OpenGL::releaseBuffer(GraphicsResource * pResource) {
      if (buffers.release(pResource->id) == 0) {
        GLBuffer & buf = getBuffer(*pResource);
        glDeleteBuffers(1, &buf.glID);
        buf.glID = 0;
        buffers.erase(pResource->id);
      }

      *pResource = InvalidGraphicsResource;
    }

    int64_t BufferManager_OpenGL::getBufferReferences(GraphicsResource bufferID) {
      return buffers.getReferenceCount(bufferID.id);
    }

    int64_t BufferManager_OpenGL::getSize(GraphicsResource bufferID) {
      return getBuffer(bufferID).size;
    }

    void * BufferManager_OpenGL::map(GraphicsResource bufferID, MapAccess access) {
      GLBuffer & buf     = getBuffer(bufferID);
      void *     pMapped = glMapNamedBuffer(buf.glID, ToGLAccess(access));
      return pMapped;
    }

    void * BufferManager_OpenGL::map(GraphicsResource bufferID, int64_t offset, int64_t size, MapAccess access) {
      GLBuffer & buf       = getBuffer(bufferID);
      GLenum     bindPoint = buf.getBindTarget(false);
      glBindBuffer(bindPoint, buf.glID);
      void * pMapped = glMapBufferRange(bindPoint, offset, size, ToGLMapBits(access));
      glBindBuffer(bindPoint, 0);
      return pMapped;
    }

    void BufferManager_OpenGL::unmap(GraphicsResource bufferID) {
      GLBuffer & buf       = getBuffer(bufferID);
      GLenum     bindPoint = buf.getBindTarget(false);
      glBindBuffer(bindPoint, buf.glID);
      glUnmapBuffer(bindPoint);
      glBindBuffer(bindPoint, 0);
    }

    bool BufferManager_OpenGL::upload(GraphicsResource bufferID, int64_t size, void const * pData) {
      GLBuffer & buf = getBuffer(bufferID);
      GLenum     bindPoint = buf.getBindTarget(false);
      glBindBuffer(bindPoint, buf.glID);

      if (buf.allocated < size) {
        glBufferData(bindPoint, size, pData, buf.glUsage);
        buf.allocated = size;
      } else {
        glBufferSubData(bindPoint, 0, size, pData);
      }

      buf.size = size;
      glBindBuffer(bindPoint, 0);

      return false;
    }

    int64_t BufferManager_OpenGL::download(GraphicsResource bufferID, void * pDst, int64_t offset, int64_t size) {
      GLBuffer & buf       = getBuffer(bufferID);
      GLenum     bindPoint = buf.getBindTarget(true);
      glBindBuffer(bindPoint, buf.glID);

      if (size == 0) {
        size = buf.size;
      }
      int64_t start = offset;
      int64_t end   = std::min(buf.size, offset + size);
      glGetBufferSubData(bindPoint, (GLintptr)start, (GLsizeiptr)(end - start), pDst);
      glBindBuffer(bindPoint, 0);
      return false;
    }

    GraphicsResource BufferManager_OpenGL::createVertexArray() {
      GLVertexArray va;
      glGenVertexArrays(1, &va.glID);
      if (va.glID == 0) {
        // Failed to create vao
        return InvalidGraphicsResource;
      }

      for (GraphicsResource & vb : va.vertexBuffers)
        vb = InvalidGraphicsResource;

      return {(uint32_t)vertexArrays.emplace(va), GraphicsResourceType_VertexArray};
    }

    GraphicsResource BufferManager_OpenGL::refVertexArray(GraphicsResource vaID) {
      vertexArrays.addRef(vaID.id);
      return vaID;
    }

    void BufferManager_OpenGL::releaseVertexArray(GraphicsResource * pResource) {
      if (vertexArrays.release(pResource->id) == 0) {
        GLVertexArray & va = getVertexArray(*pResource);
        glDeleteVertexArrays(1, &va.glID);
        va.glID = 0;
        vertexArrays.erase(pResource->id);
      }

      *pResource = InvalidGraphicsResource;
    }

    int64_t BufferManager_OpenGL::getVertexArrayReferences(GraphicsResource vaID) {
      return vertexArrays.getReferenceCount(vaID.id);
    }

    void BufferManager_OpenGL::setLayout(GraphicsResource vaID, VertexInputLayout const & layout) {
      GLVertexArray & va = getVertexArray(vaID);
      va.layout          = layout;
      va.rebind          = true;
    }

    bool BufferManager_OpenGL::setVertexBuffer(GraphicsResource vaID, int64_t slot, GraphicsResource vertexBufferID) {
      GLVertexArray & va     = getVertexArray(vaID);
      va.vertexBuffers[slot] = vertexBufferID;
      va.rebind              = true;
      return true;
    }

    bool BufferManager_OpenGL::setIndexBuffer(GraphicsResource vaID, GraphicsResource indexBufferID, DataType indexType) {
      GLVertexArray & va = getVertexArray(vaID);
      va.indexBufferType = indexType;
      va.indexBuffer     = indexBufferID;
      va.rebind          = true;
      return true;
    }

    VertexInputLayout BufferManager_OpenGL::getLayout(GraphicsResource vaID) {
      return getVertexArray(vaID).layout;
    }

    GraphicsResource BufferManager_OpenGL::getVertexBuffer(GraphicsResource vaID, int64_t slot) {
      return getVertexArray(vaID).vertexBuffers[slot];
    }

    GraphicsResource BufferManager_OpenGL::getIndexBuffer(GraphicsResource vaID) {
      return getVertexArray(vaID).indexBuffer;
    }

    DataType BufferManager_OpenGL::getIndexType(GraphicsResource vaID) {
      return getVertexArray(vaID).indexBufferType;
    }

    BufferManager_OpenGL::GLBuffer & BufferManager_OpenGL::getBuffer(GraphicsResource bufferID) {
      return buffers[bufferID.id];
    }
    BufferManager_OpenGL::GLVertexArray & BufferManager_OpenGL::getVertexArray(GraphicsResource bufferID) {
      return vertexArrays[bufferID.id];
    }

    GraphicsResource TextureManager_OpenGL::createTexture(TextureType type) {
      GLTexture newTexture;
      newTexture.type = type;
      GLenum target   = ToGLTextureType(type);

      if (target == GL_NONE) {
        // Invalid texture type
        return InvalidGraphicsResource;
      }

      glGenTextures(1, &newTexture.glID);

      if (newTexture.glID == 0) {
        // Failed to create texture
        return InvalidGraphicsResource;
      }

      glBindTexture(target, newTexture.glID);

      glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glBindTexture(target, 0);

      return {(uint32_t)textures.emplace(newTexture), GraphicsResourceType_Texture};
    }

    GraphicsResource TextureManager_OpenGL::refTexture(GraphicsResource textureID) {
      textures.addRef(textureID.id);
      return textureID;
    }

    void TextureManager_OpenGL::releaseTexture(GraphicsResource * pResource) {
      if (textures.release(pResource->id) == 0) {
        GLTexture & tex = getTexture(*pResource);
        glDeleteTextures(1, &tex.glID);
        tex.glID = 0;
        textures.erase(pResource->id);
      }

      *pResource = InvalidGraphicsResource;
    }

    int64_t TextureManager_OpenGL::getTextureReferences(GraphicsResource textureID) {
      return textures.getReferenceCount(textureID.id);
    }

    TextureType TextureManager_OpenGL::getType(GraphicsResource textureID) {
      return getTexture(textureID).type;
    }

    bool TextureManager_OpenGL::isDepthTexture(GraphicsResource textureID) {
      return getTexture(textureID).depthStencilFmt != DepthStencilFormat_None;
    }

    DepthStencilFormat TextureManager_OpenGL::getDepthStencilFormat(GraphicsResource textureID) {
      return getTexture(textureID).depthStencilFmt;
    }

    PixelFormat TextureManager_OpenGL::getColourFormat(GraphicsResource textureID) {
      return getTexture(textureID).format;
    }

    Vec3i TextureManager_OpenGL::getSize(GraphicsResource textureID, int64_t mipLevel) {
      Vec3i size = getTexture(textureID).size;
      size /= (1 << (int32_t)mipLevel);
      return {std::max(1, size.x), std::max(1, size.y), std::max(1, size.z)};
    }

    bool TextureManager_OpenGL::upload(GraphicsResource textureID, DepthStencilFormat format, Vec3i size) {
      GLTexture & tex = getTexture(textureID);

      GLenum glFormat         = ToGLFormat(format);
      GLenum glType           = ToGLType(format);
      GLenum glInternalFormat = ToGLInternalFormat(format);
      GLenum glTarget         = ToGLTextureType(tex.type);

      glBindTexture(glTarget, tex.glID);

      switch (tex.type) {
      case TextureType_2D: glTexImage2D(glTarget, 0, glInternalFormat, size.x, size.y, GL_NONE, glFormat, glType, nullptr); break;
      case TextureType_2DArray: // Fall-through
      case TextureType_3D: glTexImage3D(glTarget, 0, glInternalFormat, size.x, size.y, size.z, GL_NONE, glFormat, glType, nullptr); break;
      case TextureType_CubeMap:
        BFC_ASSERT(size.z == 6, "A cubemap must have a depth of 6.");
        break;
      }

      glBindTexture(glTarget, 0);

      tex.depthStencilFmt = format;
      tex.format          = PixelFormat_Unknown;
      tex.size            = size;

      return true;
    }

    bool TextureManager_OpenGL::upload(GraphicsResource textureID, media::Surface const & src) {
      GLTexture & tex = getTexture(textureID);

      GLenum glFormat         = ToGLFormat(src.format);
      GLenum glType           = ToGLType(src.format);
      GLenum glInternalFormat = ToGLInternalFormat(src.format);
      GLenum glTarget         = ToGLTextureType(tex.type);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glBindTexture(glTarget, tex.glID);
      switch (tex.type) {
      case TextureType_2D:      glTexImage2D(glTarget, 0, glInternalFormat, src.size.x, src.size.y, GL_NONE, glFormat, glType, src.pBuffer); break;
      case TextureType_2DArray: // Fall-through
      case TextureType_3D:      glTexImage3D(glTarget, 0, glInternalFormat, src.size.x, src.size.y, src.size.z, GL_NONE, glFormat, glType, src.pBuffer); break;
      case TextureType_CubeMap:
        BFC_ASSERT(src.size.z == 6, "A cubemap must have a depth of 6.");
        for (int64_t i = 0; i < src.size.z; ++i) {
          void * pPixels = getSurfacePixel(src, {0, 0, i});
          glTexImage2D(ToGLCubeMapFace((CubeMapFace)i), 0, glInternalFormat, src.size.x, src.size.y, GL_NONE, glFormat, glType, pPixels);
        }
        break;
      }
      glBindTexture(glTarget, 0);
      
      if (src.pBuffer != nullptr) {
        generateMipMaps(textureID);
      }

      tex.depthStencilFmt = DepthStencilFormat_Unknown;
      tex.format          = src.format;
      tex.size            = src.size;

      return true;
    }

    bool TextureManager_OpenGL::uploadSubData(GraphicsResource textureID, media::Surface const & src, Vec3i offset) {
      GLTexture & tex = getTexture(textureID);

      GLenum glFormat = ToGLFormat(src.format);
      GLenum glType   = ToGLType(src.format);
      GLenum glTarget = ToGLTextureType(tex.type);

      glBindTexture(glTarget, tex.glID);
      switch (tex.type) {
      case TextureType_2D: glTexSubImage2D(glTarget, 0, offset.x, offset.y, src.size.x, src.size.y, glFormat, glType, src.pBuffer); break;
      case TextureType_3D: glTexSubImage3D(glTarget, 0, offset.x, offset.y, offset.z, src.size.x, src.size.y, src.size.z, glFormat, glType, src.pBuffer); break;
      case TextureType_CubeMap: break;
      }
      glBindTexture(glTarget, 0);
      return true;
    }

    void TextureManager_OpenGL::generateMipMaps(GraphicsResource textureID) {
      GLTexture & tex = getTexture(textureID);
      if (glGenerateTextureMipmap != 0 && glTextureParameteri != 0) { // Use 'named' function if available
        glGenerateTextureMipmap(tex.glID);
        glTextureParameteri(tex.glID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(tex.glID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      } else {
        GLenum texTarget = ToGLTextureType(tex.type);
        glBindTexture(texTarget, tex.glID);
        glGenerateMipmap(texTarget);
        glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(texTarget, 0);
      }
    }

    bool TextureManager_OpenGL::download(GraphicsResource textureID, media::Surface * pDest) {
      return false;
    }

    GraphicsResource TextureManager_OpenGL::createSampler() {
      GLSampler newSampler;

      glCreateSamplers(1, &newSampler.glID);
      glSamplerParameteri(newSampler.glID, GL_TEXTURE_MIN_FILTER, ToGLFilterMode(newSampler.minFilter, newSampler.minMipFilter));
      glSamplerParameteri(newSampler.glID, GL_TEXTURE_MAG_FILTER, ToGLFilterMode(newSampler.magFilter, newSampler.magMipFilter));
      glSamplerParameterf(newSampler.glID, GL_TEXTURE_MIN_LOD, newSampler.minLOD);
      glSamplerParameterf(newSampler.glID, GL_TEXTURE_MAX_LOD, newSampler.maxLOD);
      glSamplerParameteri(newSampler.glID, GL_TEXTURE_WRAP_S, ToGLWrapMode(newSampler.wrapMode.x));
      glSamplerParameteri(newSampler.glID, GL_TEXTURE_WRAP_T, ToGLWrapMode(newSampler.wrapMode.y));
      glSamplerParameteri(newSampler.glID, GL_TEXTURE_WRAP_R, ToGLWrapMode(newSampler.wrapMode.z));

      return {(uint32_t)samplers.emplace(newSampler), GraphicsResourceType_Sampler};
    }

    GraphicsResource TextureManager_OpenGL::refSampler(GraphicsResource samplerID) {
      samplers.addRef(samplerID.id);
      return samplerID;
    }

    void TextureManager_OpenGL::releaseSampler(GraphicsResource * pResource) {
      if (samplers.release(pResource->id) == 0) {
        GLSampler & sampler = getSampler(*pResource);
        glDeleteSamplers(1, &sampler.glID);
        samplers.erase(pResource->id);
      }

      *pResource = InvalidGraphicsResource;
    }

    int64_t TextureManager_OpenGL::getSamplerReferences(GraphicsResource samplerID) {
      return samplers.getReferenceCount(samplerID.id);
    }

    void TextureManager_OpenGL::setSamplerMinFilter(GraphicsResource samplerID, FilterMode filter, FilterMode mipFilter) {
      GLSampler & sampler = getSampler(samplerID);
      if (sampler.minFilter != filter || sampler.minMipFilter != mipFilter) {
        glSamplerParameteri(sampler.glID, GL_TEXTURE_MIN_FILTER, ToGLFilterMode(filter, mipFilter));
      }

      sampler.minFilter    = filter;
      sampler.minMipFilter = mipFilter;
    }

    void TextureManager_OpenGL::setSamplerMagFilter(GraphicsResource samplerID, FilterMode filter, FilterMode mipFilter) {
      GLSampler & sampler = getSampler(samplerID);
      if (sampler.magFilter != filter || sampler.magMipFilter != mipFilter) {
        glSamplerParameteri(sampler.glID, GL_TEXTURE_MAG_FILTER, ToGLFilterMode(filter, mipFilter));
      }

      sampler.magFilter    = filter;
      sampler.magMipFilter = mipFilter;
    }

    void TextureManager_OpenGL::setSamplerMinLOD(GraphicsResource samplerID, float level) {
      GLSampler & sampler = getSampler(samplerID);
      if (sampler.minLOD != level) {
        glSamplerParameterf(sampler.glID, GL_TEXTURE_MIN_LOD, level);
      }
      sampler.minLOD = level;
    }

    void TextureManager_OpenGL::setSamplerMaxLOD(GraphicsResource samplerID, float level) {
      GLSampler & sampler = getSampler(samplerID);
      if (sampler.minLOD != level) {
        glSamplerParameterf(sampler.glID, GL_TEXTURE_MAX_LOD, level);
      }
      sampler.minLOD = level;
    }

    void TextureManager_OpenGL::setSamplerWrapU(GraphicsResource samplerID, WrapMode mode) {
      GLSampler & sampler = getSampler(samplerID);
      if (sampler.wrapMode.x != mode) {
        glSamplerParameteri(sampler.glID, GL_TEXTURE_WRAP_S, ToGLWrapMode(mode));
      }
      sampler.wrapMode.x  = mode;
    }

    void TextureManager_OpenGL::setSamplerWrapV(GraphicsResource samplerID, WrapMode mode) {
      GLSampler & sampler = getSampler(samplerID);
      if (sampler.wrapMode.y != mode) {
        glSamplerParameteri(sampler.glID, GL_TEXTURE_WRAP_T, ToGLWrapMode(mode));
      }
      sampler.wrapMode.y  = mode;
    }

    void TextureManager_OpenGL::setSamplerWrapW(GraphicsResource samplerID, WrapMode mode) {
      GLSampler & sampler = getSampler(samplerID);
      if (sampler.wrapMode.z != mode) {
        glSamplerParameteri(sampler.glID, GL_TEXTURE_WRAP_R, ToGLWrapMode(mode));
      }
      sampler.wrapMode.z    = mode;
    }

    TextureManager_OpenGL::GLTexture & TextureManager_OpenGL::getTexture(GraphicsResource textureID) {
      return textures[textureID.id];
    }

    TextureManager_OpenGL::GLSampler & TextureManager_OpenGL::getSampler(GraphicsResource samplerID) {
      return samplers[samplerID.id];
    }

    GraphicsResource ShaderManager_OpenGL::createShader(ShaderType type) {
      GLShader newShader;
      GLenum   glType = ToGLShaderType(type);

      if (glType == GL_NONE) {
        // Invalid shader type
        return InvalidGraphicsResource;
      }
      newShader.type = type;
      newShader.glID = glCreateShader(glType);

      if (newShader.glID == 0) {
        // Failed to create shader, maybe invalid shader type
        return InvalidGraphicsResource;
      }

      return {(uint32_t)shaders.emplace(newShader), GraphicsResourceType_Shader};
    }

    GraphicsResource ShaderManager_OpenGL::refShader(GraphicsResource shaderID) {
      shaders.addRef(shaderID.id);
      return shaderID;
    }

    void ShaderManager_OpenGL::releaseShader(GraphicsResource * pShaderID) {
      if (shaders.release(pShaderID->id) == 0) {
        GLShader & shader = getShader(*pShaderID);
        glDeleteShader(shader.glID);
        shader.glID = 0;
        shaders.erase(pShaderID->id);
      }

      *pShaderID = InvalidGraphicsResource;
    }

    int64_t ShaderManager_OpenGL::getShaderReferences(GraphicsResource shaderID) {
      return shaders.getReferenceCount(shaderID.id);
    }

    ShaderType ShaderManager_OpenGL::getType(GraphicsResource shaderID) {
      return getShader(shaderID).type;
    }

    void ShaderManager_OpenGL::setSource(GraphicsResource shaderID, StringView src) {
      GLShader & shader = getShader(shaderID);
      shader.source     = src;
      shader.file       = "";
    }

    void ShaderManager_OpenGL::setFile(GraphicsResource shaderID, StringView path) {
      GLShader & shader = getShader(shaderID);
      shader.file       = path;
      shader.source     = "";
    }

    bool ShaderManager_OpenGL::compile(GraphicsResource shaderID, String * pError) {
      GLShader & shader = getShader(shaderID);

      if (shader.file.length() > 0) {
        if (!readTextFile(shader.file, &shader.source)) {
          *pError = String("Failed to read source file ").concat(shader.file);
          return false;
        }
      }

      if (shader.source.empty()) {
        if (pError != nullptr) {
          *pError = "Source was empty";
        }
        return false;
      }

      char const * src = shader.source.c_str();
      GLint        len = (GLint)shader.source.length();
      glShaderSource(shader.glID, 1, &src, &len);
      glCompileShader(shader.glID);
      int status = 1;
      glGetShaderiv(shader.glID, GL_COMPILE_STATUS, &status);

      if (status == GL_FALSE) {
        // Compilation failed
        int logLen = 0;
        glGetShaderiv(shader.glID, GL_INFO_LOG_LENGTH, &logLen);
        Vector<char> log(logLen + 1, 0);
        glGetShaderInfoLog(shader.glID, (GLsizei)log.size(), 0, log.data());

        if (pError != nullptr) {
          *pError = log.data();
        }

        return false;
      }

      return true;
    }

    GraphicsResource ShaderManager_OpenGL::createProgram() {
      GLProgram newProgram;
      newProgram.glID = glCreateProgram();
      for (GraphicsResource & shaderID : newProgram.shaders) {
        shaderID = InvalidGraphicsResource;
      }

      return {(uint32_t)programs.emplace(newProgram), GraphicsResourceType_Program};
    }

    GraphicsResource ShaderManager_OpenGL::refProgram(GraphicsResource programID) {
      programs.addRef(programID.id);
      return programID;
    }

    void ShaderManager_OpenGL::releaseProgram(GraphicsResource * pResource) {
      if (programs.release(pResource->id) == 0) {
        GLProgram & prog = getProgram(*pResource);
        glDeleteProgram(prog.glID);
        prog.glID = 0;
        for (GraphicsResource & resource : prog.shaders) {
          if (resource != InvalidGraphicsResource) {
            releaseShader(&resource);
          }
        }
        programs.erase(pResource->id);
      }

      *pResource = InvalidGraphicsResource;
    }

    int64_t ShaderManager_OpenGL::getProgramReferences(GraphicsResource programID) {
      return programs.getReferenceCount(programID.id);
    }

    void ShaderManager_OpenGL::addShader(GraphicsResource programID, GraphicsResource shaderID) {
      GLProgram & prog   = getProgram(programID);
      GLShader &  shader = getShader(shaderID);

      if (shaderID != InvalidGraphicsResource) {
        refShader(shaderID);
      }

      if (prog.shaders[shader.type] != InvalidGraphicsResource) {
        releaseShader(&prog.shaders[shader.type]);
      }

      prog.shaders[shader.type] = shaderID;
    }

    GraphicsResource ShaderManager_OpenGL::getShader(GraphicsResource programID, ShaderType shaderType) {
      return getProgram(programID).shaders[shaderType];
    }

    bool ShaderManager_OpenGL::linkProgram(GraphicsResource programID, String * pError) {
      GLProgram & prog = getProgram(programID);
      for (GraphicsResource & shaderID : prog.shaders) {
        if (shaderID != InvalidGraphicsResource) {
          glAttachShader(prog.glID, getShader(shaderID).glID);
        }
      }

      glLinkProgram(prog.glID);
      GLint status;
      glGetProgramiv(prog.glID, GL_LINK_STATUS, &status);

      if (status == GL_FALSE) {
        int logLen = 0;
        glGetProgramiv(prog.glID, GL_INFO_LOG_LENGTH, &logLen);
        Vector<char> log(logLen + 1, 0);
        glGetProgramInfoLog(prog.glID, (GLsizei)log.size(), 0, log.data());

        printf("%s\n", log.data());

        if (pError != nullptr) {
          *pError = log.data();
        }
      }

      for (GraphicsResource & shaderID : prog.shaders) {
        if (shaderID != InvalidGraphicsResource) {
          glDetachShader(prog.glID, getShader(shaderID).glID);
        }
      }

      if (status != GL_FALSE)
        reflect(prog.glID, &prog.attributes, &prog.uniforms, &prog.textures, &prog.buffers);

      return status == GL_TRUE;
    }

    void ShaderManager_OpenGL::setUniform(GraphicsResource programID, int64_t uniformIndex, void const * pBuffer) {
      float *    f32  = (float *)pBuffer;
      int32_t *  i32  = (int32_t *)pBuffer;
      uint32_t * ui32 = (uint32_t *)pBuffer;

      GLProgram & prog        = getProgram(programID);
      Uniform &   uniformDesc = prog.uniforms[uniformIndex];
      uint32_t    pid         = prog.glID;
      uint32_t    l           = uniformDesc.glLoc;

      switch (uniformDesc.cls) {
      case DataClass_Matrix:
        if (uniformDesc.type != DataType_Float32)
          return;

        switch (uniformDesc.width) {
        case 2:
          switch (uniformDesc.height) {
          case 2: glProgramUniformMatrix2fv(pid, l, 1, GL_TRUE, f32); break;
          case 3: glProgramUniformMatrix2x3fv(pid, l, 1, GL_TRUE, f32); break;
          case 4: glProgramUniformMatrix2x4fv(pid, l, 1, GL_TRUE, f32); break;
          }
          break;
        case 3:
          switch (uniformDesc.height) {
          case 2: glProgramUniformMatrix3x2fv(pid, l, 1, GL_TRUE, f32); break;
          case 3: glProgramUniformMatrix3fv(pid, l, 1, GL_TRUE, f32); break;
          case 4: glProgramUniformMatrix3x4fv(pid, l, 1, GL_TRUE, f32); break;
          }
          break;
        case 4:
          switch (uniformDesc.height) {
          case 2: glProgramUniformMatrix4x2fv(pid, l, 1, GL_TRUE, f32); break;
          case 3: glProgramUniformMatrix4x3fv(pid, l, 1, GL_TRUE, f32); break;
          case 4: glProgramUniformMatrix4fv(pid, l, 1, GL_TRUE, f32); break;
          }
          break;
        }
        break;
      case DataClass_Vector:
        switch (uniformDesc.width) {
        case 2:
          switch (uniformDesc.type) {
          case DataType_Float32: glProgramUniform2f(pid, l, f32[0], f32[1]); break;
          case DataType_UInt32: glProgramUniform2ui(pid, l, ui32[0], i32[1]); break;
          case DataType_Int32: glProgramUniform2i(pid, l, i32[0], ui32[1]); break;
          }
          break;
        case 3:
          switch (uniformDesc.type) {
          case DataType_Float32: glProgramUniform3f(pid, l, f32[0], f32[1], f32[2]); break;
          case DataType_UInt32: glProgramUniform3ui(pid, l, ui32[0], ui32[1], ui32[2]); break;
          case DataType_Int32: glProgramUniform3i(pid, l, i32[0], i32[1], i32[2]); break;
          }
          break;
        case 4:
          switch (uniformDesc.type) {
          case DataType_Float32: glProgramUniform4f(pid, l, f32[0], f32[1], f32[2], f32[3]); break;
          case DataType_UInt32: glProgramUniform4ui(pid, l, ui32[0], ui32[1], ui32[2], ui32[3]); break;
          case DataType_Int32: glProgramUniform4i(pid, l, i32[0], i32[1], i32[2], i32[3]); break;
          }
        }
        break;
      case DataClass_Scalar:
        if (uniformDesc.width != 1 || uniformDesc.height != 1)
          return; // Scalar should not have width/height != 1
        switch (uniformDesc.type) {
        case DataType_Float32: glProgramUniform1f(pid, l, f32[0]); break;
        case DataType_UInt32: glProgramUniform1ui(pid, l, ui32[0]); break;
        case DataType_Int32: glProgramUniform1i(pid, l, i32[0]); break;
        }
        break;
      }
    }

    void ShaderManager_OpenGL::setBufferBinding(GraphicsResource programID, int64_t bufferIndex, int64_t bindPoint) {
      GLProgram & prog       = getProgram(programID);
      Buffer &    bufferDesc = prog.buffers[bufferIndex];

      if (bindPoint != bufferDesc.bindPoint) {
        bufferDesc.bindPoint = (uint32_t)bindPoint;
        if (bufferDesc.isStorageBuffer)
          glShaderStorageBlockBinding(prog.glID, bufferDesc.bufferIndex, bufferDesc.bindPoint);
        else
          glUniformBlockBinding(prog.glID, bufferDesc.bufferIndex, bufferDesc.bindPoint);
      }
    }

    void ShaderManager_OpenGL::setTextureBinding(GraphicsResource programID, int64_t textureIndex, int64_t bindPoint) {
      GLProgram & prog        = getProgram(programID);
      Texture &   textureDesc = prog.textures[textureIndex];

      if (bindPoint != textureDesc.bindPoint) {
        textureDesc.bindPoint = (int32_t)bindPoint;
        glProgramUniform1i(prog.glID, textureDesc.glLoc, textureDesc.bindPoint);
      }
    }

    void ShaderManager_OpenGL::getUniform(GraphicsResource programID, int64_t uniformIndex, void * pBuffer, ProgramUniformDesc * pDesc) {
      GLProgram & prog        = getProgram(programID);
      Uniform &   uniformDesc = prog.uniforms[uniformIndex];

      if (pDesc != nullptr) {
        getUniformDesc(programID, uniformIndex, pDesc);
      }

      if (pBuffer != nullptr) {
        switch (uniformDesc.type) {
        case DataType_Float32: glGetUniformfv(prog.glID, uniformDesc.glLoc, (float *)pBuffer); break;
        case DataType_Int32: glGetUniformiv(prog.glID, uniformDesc.glLoc, (int32_t *)pBuffer); break;
        case DataType_UInt32: glGetUniformuiv(prog.glID, uniformDesc.glLoc, (uint32_t *)pBuffer); break;
        }
      }
    }

    int64_t ShaderManager_OpenGL::getBufferBinding(GraphicsResource programID, int64_t bufferIndex) {
      return getProgram(programID).buffers[bufferIndex].bindPoint;
    }

    int64_t ShaderManager_OpenGL::getTextureBinding(GraphicsResource programID, int64_t textureIndex) {
      return getProgram(programID).textures[textureIndex].bindPoint;
    }

    int64_t ShaderManager_OpenGL::getAttributeCount(GraphicsResource programID) {
      return getProgram(programID).attributes.size();
    }

    int64_t ShaderManager_OpenGL::getUniformCount(GraphicsResource programID) {
      return getProgram(programID).uniforms.size();
    }

    int64_t ShaderManager_OpenGL::getBufferCount(GraphicsResource programID) {
      return getProgram(programID).buffers.size();
    }

    int64_t ShaderManager_OpenGL::getTextureCount(GraphicsResource programID) {
      return getProgram(programID).textures.size();
    }

    void ShaderManager_OpenGL::getAttributeDesc(GraphicsResource programID, int64_t attributeIndex, ProgramAttributeDesc * pDesc) {
      Attribute & attributeDesc = getProgram(programID).attributes[attributeIndex];
      pDesc->name               = attributeDesc.name;
      pDesc->cls                = attributeDesc.cls;
      pDesc->type               = attributeDesc.type;
      pDesc->width              = attributeDesc.width;
      pDesc->height             = attributeDesc.height;
    }

    void ShaderManager_OpenGL::getUniformDesc(GraphicsResource programID, int64_t uniformIndex, ProgramUniformDesc * pDesc) {
      Uniform & uniformDesc = getProgram(programID).uniforms[uniformIndex];
      pDesc->name           = uniformDesc.name;
      pDesc->cls            = uniformDesc.cls;
      pDesc->type           = uniformDesc.type;
      pDesc->width          = uniformDesc.width;
      pDesc->height         = uniformDesc.height;
    }

    void ShaderManager_OpenGL::getTextureDesc(GraphicsResource programID, int64_t textureIndex, ProgramTextureDesc * pDesc) {
      Texture & textureDesc = getProgram(programID).textures[textureIndex];
      pDesc->name           = textureDesc.name;
      pDesc->type           = textureDesc.type;
    }

    void ShaderManager_OpenGL::getBufferDesc(GraphicsResource programID, int64_t bufferIndex, ProgramBufferDesc * pDesc) {
      Buffer & bufferDesc = getProgram(programID).buffers[bufferIndex];
      pDesc->name         = bufferDesc.name;
      pDesc->size         = bufferDesc.size;
    }

    void ShaderManager_OpenGL::reflect(uint32_t glID, Vector<Attribute> * pAttributes, Vector<Uniform> * pUniforms, Vector<Texture> * pTextures,
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

    ShaderManager_OpenGL::GLShader & ShaderManager_OpenGL::getShader(GraphicsResource shaderID) {
      return shaders[shaderID.id];
    }

    ShaderManager_OpenGL::GLProgram & ShaderManager_OpenGL::getProgram(GraphicsResource programID) {
      return programs[programID.id];
    }

    RenderTargetManager_OpenGL::RenderTargetManager_OpenGL(TextureManager * pTextures)
      : m_pTextures(pTextures) {}

    GraphicsResource RenderTargetManager_OpenGL::createRenderTarget(RenderTargetType type) {
      GLRenderTarget target;

      if (type == RenderTargetType_Texture) {
        glGenFramebuffers(1, &target.textures.glID);
      }

      target.type = type;

      return {(uint32_t)renderTargets.emplace(target), GraphicsResourceType_RenderTarget};
    }

    GraphicsResource RenderTargetManager_OpenGL::refRenderTarget(GraphicsResource renderTargetID) {
      renderTargets.addRef(renderTargetID.id);
      return renderTargetID;
    }

    void RenderTargetManager_OpenGL::releaseRenderTarget(GraphicsResource * pResource) {
      if (renderTargets.release(pResource->id) == 0) {
        GLRenderTarget & rt = getRenderTarget(*pResource);
        glDeleteFramebuffers(1, &rt.textures.glID);
        renderTargets.erase(pResource->id);
      }
      *pResource = InvalidGraphicsResource;
    }

    RenderTargetType RenderTargetManager_OpenGL::getType(GraphicsResource renderTargetID) {
      return getRenderTarget(renderTargetID).type;
    }

    Vec2i RenderTargetManager_OpenGL::getSize(GraphicsResource renderTargetID) {
      GLRenderTarget & rt = getRenderTarget(renderTargetID);
      switch (rt.type) {
      case RenderTargetType_Texture:
        for (auto & [i, colour] : enumerate(rt.textures.colour)) {
          if (colour.texture != InvalidGraphicsResource) {
            return m_pTextures->getSize(colour.texture, colour.mipLevel);
          }
        }

        if (rt.textures.depth.texture != InvalidGraphicsResource) {
          return m_pTextures->getSize(rt.textures.depth.texture, rt.textures.depth.mipLevel);
        }
        break;
      case RenderTargetType_Window: {
        RECT rect;
        GetClientRect(rt.window.hWnd, &rect);
        return {rect.right - rect.left, rect.bottom - rect.top};
      } break;
      }

      return Vec2i(0);
    }

    bool RenderTargetManager_OpenGL::attachWindow(GraphicsResource renderTargetID, platform::Window * pWindow, DepthStencilFormat depthStencilFormat) {
      GLRenderTarget & rt   = getRenderTarget(renderTargetID);
      rt.window.hWnd        = (HWND)pWindow->getPlatformHandle();
      rt.window.hDC         = GetDC(rt.window.hWnd);
      rt.window.depthFormat = depthStencilFormat;

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

      if (!glChoosePixelFormatARB(rt.window.hDC, attribList.data(), 0, 1, &pixelFormat, &numFormatsFound))
        return false;

      PIXELFORMATDESCRIPTOR pfd = {0};
      memset(&pfd, 0, sizeof(pfd));
      DescribePixelFormat(rt.window.hDC, pixelFormat, sizeof(pfd), &pfd);

      return SetPixelFormat(rt.window.hDC, pixelFormat, &pfd);
    }

    void RenderTargetManager_OpenGL::attachColour(GraphicsResource renderTargetID, GraphicsResource textureID, int64_t slot, int64_t mipLevel, int64_t layer) {
      GLRenderTarget & rt      = getRenderTarget(renderTargetID);
      bool             changed = false;
      changed |= rt.textures.colour[slot].texture != textureID;
      changed |= rt.textures.colour[slot].mipLevel != mipLevel;
      changed |= rt.textures.colour[slot].layer != layer;
      rt.textures.colour[slot].texture  = textureID;
      rt.textures.colour[slot].mipLevel = mipLevel;
      rt.textures.colour[slot].layer    = layer;
      rt.textures.rebind |= changed;
    }

    void RenderTargetManager_OpenGL::attachDepth(GraphicsResource renderTargetID, GraphicsResource textureID, int64_t mipLevel, int64_t layer) {
      GLRenderTarget & rt      = getRenderTarget(renderTargetID);
      bool             changed = false;
      changed |= rt.textures.depth.texture != textureID;
      changed |= rt.textures.depth.mipLevel != mipLevel;
      changed |= rt.textures.depth.layer != layer;
      rt.textures.depth.texture  = textureID;
      rt.textures.depth.mipLevel = mipLevel;
      rt.textures.depth.layer    = layer;
      rt.textures.rebind |= changed;
    }

    void RenderTargetManager_OpenGL::setReadAttachment(GraphicsResource renderTargetID, int64_t slot) {
      GLRenderTarget & rt      = getRenderTarget(renderTargetID);
      bool             changed = false;
      changed |= rt.textures.colourReadAttachment != slot;
      rt.textures.colourReadAttachment = slot;
      rt.textures.rebind               = changed;
    }

    int64_t RenderTargetManager_OpenGL::getRenderTargetReferences(GraphicsResource renderTargetID) {
      return renderTargets.getReferenceCount(renderTargetID.id);
    }

    RenderTargetManager_OpenGL::GLRenderTarget & RenderTargetManager_OpenGL::getRenderTarget(GraphicsResource renderTargetID) {
      return renderTargets[renderTargetID.id];
    }

    void StateManager_OpenGL::setFeatureEnabled(GraphicsState state, bool enabled) {
      switch (state) {
      case GraphicsState_Blend: SetGLStateEnabled(GL_BLEND, enabled); break;
      case GraphicsState_DepthTest: SetGLStateEnabled(GL_DEPTH_TEST, enabled); break;
      case GraphicsState_DepthWrite: glDepthMask(enabled ? GL_TRUE : GL_FALSE); break;
      case GraphicsState_StencilTest: SetGLStateEnabled(GL_STENCIL_TEST, enabled); break;
      case GraphicsState_ScissorTest: SetGLStateEnabled(GL_SCISSOR_TEST, enabled); break;
      }
    }

    void StateManager_OpenGL::setViewport(Vec2i position, Vec2i size) {
      glViewport(position.x, position.y, size.x, size.y);
    }

    void StateManager_OpenGL::setScissor(Vec2i position, Vec2i size) {
      glScissor(position.x, position.y, size.x, size.y);
    }

    void StateManager_OpenGL::setDepthRange(float min, float max) {
      glDepthRange(min, max);
    }

    void StateManager_OpenGL::setDepthFunction(ComparisonFunction function) {
      glDepthFunc(ToGLComparison(function));
    }

    void StateManager_OpenGL::setBlendEquation(BlendEquation colourAndAlpha, int64_t colourAttachment) {
      GLenum glEq = ToGLEquation(colourAndAlpha);
      if (colourAttachment == -1)
        glBlendEquation(glEq);
      else
        glBlendEquationi((GLuint)colourAttachment, glEq);
    }

    void StateManager_OpenGL::setBlendEquation(BlendEquation colour, BlendEquation alpha, int64_t colourAttachment) {
      GLenum glColourEq = ToGLEquation(colour);
      GLenum glAlphaEq  = ToGLEquation(alpha);
      if (colourAttachment == -1)
        glBlendEquationSeparate(glColourEq, glAlphaEq);
      else
        glBlendEquationSeparatei((GLuint)colourAttachment, glColourEq, glAlphaEq);
    }

    void StateManager_OpenGL::setBlendFunction(BlendFunction sourceFactor, BlendFunction destFactor, int64_t colourAttachment) {
      GLenum glSrcFactor = ToGLBlendFunction(sourceFactor);
      GLenum glDstFactor = ToGLBlendFunction(destFactor);
      if (colourAttachment == -1)
        glBlendFunc(glSrcFactor, glDstFactor);
      else
        glBlendFunci((GLuint)colourAttachment, glSrcFactor, glDstFactor);
    }

    void StateManager_OpenGL::setBlendFunction(BlendFunction sourceColourFactor, BlendFunction sourceAlphaFactor, BlendFunction destColourFactor,
                                               BlendFunction destAlphaFactor, int64_t colourAttachment) {
      GLenum glSrcColourFactor = ToGLBlendFunction(sourceColourFactor);
      GLenum glSrcAlphaFactor  = ToGLBlendFunction(sourceAlphaFactor);
      GLenum glDstColourFactor = ToGLBlendFunction(destColourFactor);
      GLenum glDstAlphaFactor  = ToGLBlendFunction(destAlphaFactor);
      if (colourAttachment == -1)
        glBlendFuncSeparate(glSrcColourFactor, glSrcAlphaFactor, glDstColourFactor, glDstAlphaFactor);
      else
        glBlendFuncSeparatei((GLuint)colourAttachment, glSrcColourFactor, glSrcAlphaFactor, glDstColourFactor, glDstAlphaFactor);
    }

    void StateManager_OpenGL::setColourWriteEnabled(bool red, bool green, bool blue, bool alpha) {
      glColorMask(red, green, blue, alpha);
    }

    void StateManager_OpenGL::setColourFactor(float red, float green, float blue, float alpha) {
      glBlendColor(red, green, blue, alpha);
    }
    GLenum BufferManager_OpenGL::GLBuffer::getBindTarget(bool read) const {
      if (defaultTarget != GL_NONE)
        return defaultTarget;
      else
        return read ? GL_COPY_READ_BUFFER : GL_COPY_WRITE_BUFFER;
    }
  } // namespace graphics

  GraphicsDevice_OpenGL::GraphicsDevice_OpenGL()
    : m_renderTargets(&m_textures) {}

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

    m_defaultTarget = m_renderTargets.createRenderTarget(RenderTargetType_Window);
    if (!m_renderTargets.attachWindow(m_defaultTarget, pWindow, DepthStencilFormat_D24S8))
      return false;

    HDC hDC = m_renderTargets.getRenderTarget(m_defaultTarget).window.hDC;

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

    m_activeWindowTarget = m_defaultTarget;
    m_activeTarget       = m_defaultTarget;

    glGetInteger64v(GL_MAX_TEXTURE_IMAGE_UNITS, &m_lastTextureUnit);
    m_lastTextureUnit -= 1;

    glGenVertexArrays(1, &m_emptyVertexArray);
    glBindVertexArray(m_emptyVertexArray);

    return true;
  }

  void GraphicsDevice_OpenGL::bindProgram(GraphicsResource programID) {
    if (programID == InvalidGraphicsResource) {
      glUseProgram(0);
    } else {
      BFC_ASSERT(programID.type == GraphicsResourceType_Program, "resource id is not a Program.");
      glUseProgram(m_shaders.getProgram(programID).glID);
    }
  }

  void GraphicsDevice_OpenGL::bindVertexArray(GraphicsResource vertexArrayID) {
    if (vertexArrayID == InvalidGraphicsResource) {
      m_indexCount  = -1;
      m_vertexCount = -1;
      glBindVertexArray(m_emptyVertexArray);
      return;
    }

    BFC_ASSERT(vertexArrayID.type == GraphicsResourceType_VertexArray, "resource id is not a Vertex Array.");

    graphics::BufferManager_OpenGL::GLVertexArray & va = m_buffers.getVertexArray(vertexArrayID);
    glBindVertexArray(va.glID);

    if (va.rebind) {
      for (int64_t slot : va.activeSlots) {
        glDisableVertexAttribArray((GLuint)slot);
      }

      va.activeSlots.clear();
      int64_t numElements = va.layout.getAttributeCount();
      for (int64_t i = 0; i < numElements; ++i) {
        auto const & elm = va.layout.getAttributeLayout(i);
        auto const & sem = va.layout.getAttributeSemantic(i);

        GLuint           loc          = (GLuint)GetSemanticLocation(sem);
        GraphicsResource vertexBuffer = va.vertexBuffers[elm.slot];
        if (vertexBuffer == InvalidGraphicsResource)
          continue;
        glBindBuffer(GL_ARRAY_BUFFER, m_buffers.getBuffer(vertexBuffer).glID);
        glEnableVertexAttribArray(loc);

        bool normalized = (elm.flags & LayoutFlag_Normalize) > 0;
        if ((elm.flags & LayoutFlag_Integer) > 0)
          glVertexAttribIPointer(loc, (GLint)(elm.width * elm.height), ToGLDataType(elm.dataType), (GLsizei)elm.stride, (void *)elm.offset);
        else
          glVertexAttribPointer(loc, (GLint)(elm.width * elm.height), ToGLDataType(elm.dataType), normalized, (GLsizei)elm.stride, (void *)elm.offset);

        va.activeSlots.pushBack(loc);
      }

      if (m_vertexCount == std::numeric_limits<int64_t>::max())
        m_vertexCount = 0;

      if (va.indexBuffer != InvalidGraphicsResource) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers.getBuffer(va.indexBuffer).glID);
      } else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        m_vaIndexType = DataType_Unknown;
        m_indexCount  = 0;
      }
      va.rebind = false;
    }

    int64_t numElements = va.layout.getAttributeCount();
    m_vertexCount       = std::numeric_limits<int64_t>::max();
    for (int64_t i = 0; i < numElements; ++i) {
      auto const &     elm          = va.layout.getAttributeLayout(i);
      GraphicsResource vertexBuffer = va.vertexBuffers[elm.slot];
      m_vertexCount                 = std::min(m_vertexCount, m_buffers.getBuffer(vertexBuffer).size / elm.stride);
    }

    if (va.indexBuffer != InvalidGraphicsResource) {
      m_vaIndexType = va.indexBufferType;
      m_indexCount  = m_buffers.getBuffer(va.indexBuffer).size / getDataTypeSize(m_vaIndexType);
    }
  }

  void GraphicsDevice_OpenGL::bindTexture(GraphicsResource textureID, int64_t textureUnit) {
    uint32_t glID   = 0;
    GLenum   target = GL_TEXTURE_2D;

    if (textureID != InvalidGraphicsResource) {
      BFC_ASSERT(textureID.type == GraphicsResourceType_Texture, "resource id is not a Texture.");

      auto & tex = m_textures.getTexture(textureID);
      target     = ToGLTextureType(tex.type);
      glID       = tex.glID;
    }

    glActiveTexture((GLenum)(GL_TEXTURE0 + textureUnit));
    glBindTexture(target, glID);
    glActiveTexture((GLenum)(GL_TEXTURE0 + m_lastTextureUnit));
    glBindTexture(target, 0);
  }

  void GraphicsDevice_OpenGL::bindSampler(GraphicsResource samplerID, int64_t textureUnit) {
    uint32_t glID = 0;
    if (samplerID != InvalidGraphicsResource) {
      BFC_ASSERT(samplerID.type == GraphicsResourceType_Sampler, "resource id is not a Sampler.");
      glID = m_textures.getSampler(samplerID).glID;
    }

    glBindSampler((uint32_t)textureUnit, glID);
  }

  void GraphicsDevice_OpenGL::bindUniformBuffer(GraphicsResource bufferID, int64_t bindPoint, int64_t offset, int64_t size) {
    uint32_t glID = 0;
    if (bufferID != InvalidGraphicsResource) {
      BFC_ASSERT(bufferID.type == GraphicsResourceType_Buffer, "resource id is not a Buffer.");

      if (size == 0) {
        size = m_buffers.getSize(bufferID) - offset;
      }

      if (size >= 0) {
        glID = m_buffers.getBuffer(bufferID).glID;
      }
    }

    glBindBufferRange(GL_UNIFORM_BUFFER, (GLint)bindPoint, glID, (GLintptr)offset, (GLsizeiptr)size);
  }

  void GraphicsDevice_OpenGL::bindShaderStorageBuffer(GraphicsResource bufferID, int64_t bindPoint, int64_t offset, int64_t size) {
    uint32_t glID = 0;
    if (bufferID != InvalidGraphicsResource) {
      BFC_ASSERT(bufferID.type == GraphicsResourceType_Buffer, "resource id is not a Buffer.");

      if (size == 0) {
        size = m_buffers.getSize(bufferID) - offset;
      }

      if (size >= 0) {
        glID = m_buffers.getBuffer(bufferID).glID;
      }
    }

    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, (GLint)bindPoint, glID, (GLintptr)offset, (GLsizeiptr)size);
  }

  void GraphicsDevice_OpenGL::bindRenderTarget(GraphicsResource renderTargetID, MapAccess renderTargetAccess) {
    if (renderTargetID == InvalidGraphicsResource) {
      return bindRenderTarget(m_defaultTarget, renderTargetAccess);
    }

    BFC_ASSERT(renderTargetID.type == GraphicsResourceType_RenderTarget, "resource id is not a Render Target.");

    bool write = (renderTargetAccess & MapAccess_Write) > 0;
    bool read  = (renderTargetAccess & MapAccess_Read) > 0;

    graphics::RenderTargetManager_OpenGL::GLRenderTarget & rt = m_renderTargets.getRenderTarget(renderTargetID);

    if (rt.type == RenderTargetType_Texture) {
      Vector<GLenum> activeAttachments;
      GLenum         fboTarget = GL_NONE;
      if (read) {
        fboTarget = GL_READ_FRAMEBUFFER;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, rt.textures.glID);
      }
      if (write) {
        fboTarget = GL_DRAW_FRAMEBUFFER;
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rt.textures.glID);
      }

      if (rt.textures.rebind) {
        TextureType fbClass = TextureType_Unknown;
        for (int64_t index = 0; index < MaxColourAttachments; ++index) {
          auto & attachment = rt.textures.colour[index];
          if (attachment.texture != InvalidGraphicsResource) {
            auto & glTex = m_textures.getTexture(attachment.texture);
            fbClass      = glTex.type;
            break;
          }
        }

        if (fbClass == TextureType_Unknown) {
          if (rt.textures.depth.texture != InvalidGraphicsResource) {
            auto & glTex      = m_textures.getTexture(rt.textures.depth.texture);
            fbClass      = glTex.type;
          }
        }

        for (int64_t index = 0; index < MaxColourAttachments; ++index) {
          GLenum attachmentID = (GLenum)(GL_COLOR_ATTACHMENT0 + index);
          auto & attachment   = rt.textures.colour[index];
          if (attachment.texture != InvalidGraphicsResource) {
            auto & glTex         = m_textures.getTexture(attachment.texture);
            GLenum textureTarget = ToGLFramebufferTextureTarget(glTex.type, attachment.layer);

            switch (glTex.type) {
            case TextureType_2D:
            case TextureType_CubeMap: glFramebufferTexture2D(fboTarget, attachmentID, textureTarget, glTex.glID, (GLint)attachment.mipLevel); break;
            case TextureType_2DArray:
              glFramebufferTextureLayer(fboTarget, attachmentID, glTex.glID, (GLint)attachment.mipLevel, (GLint)attachment.layer);
              break;
            case TextureType_3D:
              glFramebufferTexture3D(fboTarget, attachmentID, textureTarget, glTex.glID, (GLint)attachment.mipLevel, (GLint)attachment.layer);
              break;
            }
            activeAttachments.pushBack(attachmentID);
          } else {
            switch (fbClass) {
            case TextureType_2D: glFramebufferTexture2D(fboTarget, attachmentID, GL_TEXTURE_2D, 0, 0); break;
            case TextureType_3D:
            case TextureType_2DArray:
              glFramebufferTextureLayer(fboTarget, attachmentID, 0, 0, 0);
              break;
            case TextureType_CubeMap:
              glFramebufferTexture3D(fboTarget, attachmentID, GL_TEXTURE_3D, 0, 0, 0);
              break;
            }
          }
        }

        if (rt.textures.depth.texture != InvalidGraphicsResource) {
          auto & attachment    = rt.textures.depth;
          auto & glTex         = m_textures.getTexture(attachment.texture);
          GLenum textureTarget = ToGLFramebufferTextureTarget(glTex.type, attachment.layer);
          GLenum attachmentID  = ToGLFramebufferAttachment(glTex.depthStencilFmt);
          switch (glTex.type) {
          case TextureType_2D:
          case TextureType_CubeMap: glFramebufferTexture2D(fboTarget, attachmentID, textureTarget, glTex.glID, (GLint)attachment.mipLevel); break;
          case TextureType_2DArray: glFramebufferTextureLayer(fboTarget, attachmentID, glTex.glID, (GLint)attachment.mipLevel, (GLint)attachment.layer); break;
          case TextureType_3D:
            glFramebufferTexture3D(fboTarget, attachmentID, textureTarget, glTex.glID, (GLint)attachment.mipLevel, (GLint)attachment.layer);
            break;
          }
        } else {
          switch (fbClass) {
          case TextureType_2D: glFramebufferTexture2D(fboTarget, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0); break;
          case TextureType_3D:
          case TextureType_2DArray: glFramebufferTextureLayer(fboTarget, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0, 0); break;
          case TextureType_CubeMap: glFramebufferTexture3D(fboTarget, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_3D, 0, 0, 0); break;
          }
        }

        if (write) {
          if (activeAttachments.size() == 0) {
            glDrawBuffer(GL_NONE);
          } else {
            glDrawBuffers((GLint)activeAttachments.size(), activeAttachments.data());
          }
        }

        if (read) {
          if (activeAttachments.size() == 0) {
            glReadBuffer(GL_NONE);
          } else {
            glReadBuffer(GLenum(GL_COLOR_ATTACHMENT0 + rt.textures.colourReadAttachment));
          }
        }

        rt.textures.rebind = false;
      }
    } else if (rt.type == RenderTargetType_Window) {
      wglMakeCurrent(rt.window.hDC, m_hGLRC);

      if (read) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      }

      if (write) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      }

      glDrawBuffer(write ? GL_BACK : GL_NONE);
      glReadBuffer(read ? GL_BACK : GL_NONE);

      m_activeWindowTarget = renderTargetID;
    }
  }

  void GraphicsDevice_OpenGL::bindScreen(MapAccess renderTargetAccess) {
    bindRenderTarget(m_defaultTarget, renderTargetAccess);
  }

  void GraphicsDevice_OpenGL::draw(int64_t elementCount, int64_t elementOffset, PrimitiveType primType, int64_t instanceCount) {
    int64_t count = elementCount;

    // Only validate elementCount if a vertex array has been bound.
    // This is to support shaders which may not take any vertex inputs.
    // However, you must explicity bind an Invalid vertex array.
    if (m_vertexCount >= 0) {
      count = std::min(elementCount, m_vertexCount - elementOffset);
      if (count < 0)
        return;
    }

    if (instanceCount == 1) {
      glDrawArrays(ToGLPrimType(primType), (GLint)elementOffset, (GLsizei)count);
    } else {
      glDrawArraysInstanced(ToGLPrimType(primType), (GLint)elementOffset, (GLsizei)count, (GLsizei)instanceCount);
    }
  }

  void GraphicsDevice_OpenGL::drawIndexed(int64_t elementCount, int64_t elementOffset, PrimitiveType primType, int64_t instanceCount) {
    int64_t count = elementCount;

    // Only validate elementCount if a vertex array has been bound.
    // This is to support shaders which may not take any vertex inputs.
    // However, you must explicity bind an Invalid vertex array.
    if (m_indexCount >= 0) {
      count = std::min(elementCount, m_indexCount - elementOffset);
      if (count < 0)
        return;
    }

    if (instanceCount == 1) {
      glDrawElements(ToGLPrimType(primType), (GLsizei)count, ToGLDataType(m_vaIndexType), (void *)(elementOffset * getDataTypeSize(m_vaIndexType)));
    } else {
      glDrawElementsInstanced(ToGLPrimType(primType), (GLsizei)count, ToGLDataType(m_vaIndexType), (void *)elementOffset, (GLsizei)instanceCount);
    }
  }

  graphics::BufferManager * GraphicsDevice_OpenGL::getBufferManager() {
    return &m_buffers;
  }

  graphics::TextureManager * GraphicsDevice_OpenGL::getTextureManager() {
    return &m_textures;
  }

  graphics::ShaderManager * GraphicsDevice_OpenGL::getShaderManager() {
    return &m_shaders;
  }

  void GraphicsDevice_OpenGL::destroy() {
    delete this;
  }

  void GraphicsDevice_OpenGL::clear(RGBAu8 colour) {
    glClearColor(colour.r / 255.0f, colour.g / 255.0f, colour.b / 255.0f, colour.a / 255.0f);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  }

  void GraphicsDevice_OpenGL::swap() {
    SwapBuffers(m_renderTargets.getRenderTarget(m_activeWindowTarget).window.hDC);
  }

  graphics::RenderTargetManager * GraphicsDevice_OpenGL::getRenderTargetManager() {
    return &m_renderTargets;
  }

  graphics::StateManager * GraphicsDevice_OpenGL::getStateManager() {
    return &m_stateManager;
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
    case PixelFormat_Ru8:     // Fall-through
    case PixelFormat_RGBu8:   // Fall-through
    case PixelFormat_RGBAu8:  // Fall-through
    case PixelFormat_Lu8:     // Fall-through
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
      case bfc::FilterMode_Linear:  return GL_LINEAR_MIPMAP_LINEAR;
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
