#pragma once

#include "GraphicsDevice_OpenGL.h"

namespace bfc {
  namespace graphics {
    class GLBuffer;
    class GLVertexArray;
    class GLTexture;
    class GLSampler;
    class GLShader;
    class GLProgram;
    class GLRenderTarget;

    namespace impl {
      namespace OpenGL {
        struct BindProgram {
          GLProgram * pProgram;

          static void execute(BindProgram const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            if (pCmd->pProgram == InvalidGraphicsResource) {
              glUseProgram(0);
            } else {
              glUseProgram(pCmd->pProgram->glID);
            }
          }
        };

        struct BindVertexArray {
          GLVertexArray * pVertexArray;

          static void execute(BindVertexArray const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            if (pCmd->pVertexArray->glID == 0)
              glGenVertexArrays(1, &pCmd->pVertexArray->glID);

            GLVertexArray & va = *pCmd->pVertexArray;
            glBindVertexArray(va.glID);

            // TODO: Move to command list
            // int64_t numElements = va.layout.getAttributeCount();
            // m_vertexCount       = std::numeric_limits<int64_t>::max();
            // for (int64_t i = 0; i < numElements; ++i) {
            //   auto const & elm          = va.layout.getAttributeLayout(i);
            //   auto         vertexBuffer = ToGL(va.vertexBuffers[elm.slot]);
            //   m_vertexCount             = std::min(m_vertexCount, vertexBuffer.size / elm.stride);
            // }
            //
            // if (va.indexBuffer != InvalidGraphicsResource) {
            //   m_vaIndexType = va.indexBufferType;
            //   m_indexCount  = ToGL(va.indexBuffer).size / getDataTypeSize(m_vaIndexType);
            // }
          }
        };

        struct RebindVertexArray {
          struct Element {
            GLuint     glLoc;
            GLenum     dataType;
            GLsizei    stride;
            GLint      size;
            int32_t    slot;
            int64_t    offset;
            LayoutFlag flags;
          };

          GLVertexArray *                    pVertexArray;
          int64_t                            numElements = 0;
          CommandBuffer::Serialized<Element> elements;
          GLBuffer *                         vertexBuffers[MaxVertexBuffers];
          GLBuffer *                         pIndexBuffer;

          static void execute(RebindVertexArray const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            BFC_ASSERT(pCmd->pVertexArray->glID != 0, "Vertex array has not been created");

            auto & activeSlots = pCmd->pVertexArray->activeSlots;
            for (int64_t slot : activeSlots) {
              glDisableVertexAttribArray((GLuint)slot);
            }
            activeSlots.clear();

            CommandBuffer::Serialized<Element> handle = pCmd->elements;
            for (int64_t i = 0; i < pCmd->numElements; ++i) {
              int64_t sz;
              Element elm = pBuffer->deserialize(handle, &sz);
              handle.offset += sz;

              GLBuffer * pVertexBuffer = pCmd->vertexBuffers[elm.slot];
              if (pVertexBuffer == InvalidGraphicsResource)
                continue;

              glBindBuffer(GL_ARRAY_BUFFER, pVertexBuffer->glID);
              glEnableVertexAttribArray(elm.glLoc);

              bool normalized = (elm.flags & LayoutFlag_Normalize) > 0;
              if ((elm.flags & LayoutFlag_Integer) > 0)
                glVertexAttribIPointer(elm.glLoc, elm.size, elm.dataType, elm.stride, (void *)elm.offset);
              else
                glVertexAttribPointer(elm.glLoc, elm.size, elm.dataType, normalized, elm.stride, (void *)elm.offset);

              activeSlots.pushBack(elm.glLoc);
            }

            if (pCmd->pIndexBuffer != InvalidGraphicsResource) {
              glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pCmd->pIndexBuffer->glID);
            } else {
              glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
          }
        };

        struct BindTexture {
          GLuint      textureUnit;
          GLenum      target;
          GLTexture * pTexture;

          static void execute(BindTexture const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            uint32_t glID = pCmd->pTexture == InvalidGraphicsResource ? 0 : pCmd->pTexture->glID;
            glActiveTexture(GL_TEXTURE0 + pCmd->textureUnit);
            glBindTexture(pCmd->target, glID);
            glActiveTexture(GL_TEXTURE0 + ((GraphicsDevice_OpenGL *)pDevice)->getReservedTextureUnit());
            glBindTexture(pCmd->target, 0);
          }
        };

        struct UpdateSampler {
          GLSampler * pSampler;

          GLenum minFilter;
          GLenum magFilter;

          float minLOD = -1000.0f;
          float maxLOD = 1000.0f;

          Vector3<GLenum> wrapMode;

          static void execute(UpdateSampler const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            if (pCmd->pSampler->glID == 0) {
              glCreateSamplers(1, &pCmd->pSampler->glID);
            }

            uint32_t glID = pCmd->pSampler->glID;
            glSamplerParameteri(glID, GL_TEXTURE_MIN_FILTER, pCmd->minFilter);
            glSamplerParameteri(glID, GL_TEXTURE_MAG_FILTER, pCmd->magFilter);
            glSamplerParameterf(glID, GL_TEXTURE_MIN_LOD, pCmd->minLOD);
            glSamplerParameterf(glID, GL_TEXTURE_MAX_LOD, pCmd->maxLOD);
            glSamplerParameteri(glID, GL_TEXTURE_WRAP_S, pCmd->wrapMode.x);
            glSamplerParameteri(glID, GL_TEXTURE_WRAP_T, pCmd->wrapMode.y);
            glSamplerParameteri(glID, GL_TEXTURE_WRAP_R, pCmd->wrapMode.z);
          }
        };

        struct BindSampler {
          GLuint      textureUnit;
          GLSampler * pSampler;

          static void execute(BindSampler const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            if (pCmd->pSampler != InvalidGraphicsResource) {
              glBindSampler(pCmd->textureUnit, 0);
            } else {
              glBindSampler(pCmd->textureUnit, pCmd->pSampler->glID);
            }
          }
        };

        struct BindUniformBuffer {
          GLBuffer * pBuffer;
          GLint      bindPoint;
          GLintptr   offset;
          GLsizeiptr size;
          GLenum     target;

          static void execute(BindUniformBuffer const * pCmd, GraphicsDevice_OpenGL * pDevice, CommandBuffer const * pBuffer) {
            GLuint glID = pCmd->pBuffer == nullptr ? 0 : pCmd->pBuffer->glID;
            glBindBufferRange(pCmd->target, pCmd->bindPoint, glID, pCmd->offset, pCmd->size);
          }
        };

        struct BindTextureRenderTarget {
          GLRenderTarget * pRenderTarget;
          MapAccess        access;

          static void execute(BindTextureRenderTarget const * pCmd, GraphicsDevice_OpenGL * pDevice, CommandBuffer const * pBuffer) {
            bool write = (pCmd->access & MapAccess_Write) > 0;
            bool read  = (pCmd->access & MapAccess_Read) > 0;

            uint32_t & glID = pCmd->pRenderTarget->textures.glID;

            if (glID == 0)
              glGenFramebuffers(1, &glID);

            Vector<GLenum> activeAttachments;
            GLenum         fboTarget = GL_NONE;
            if (read) {
              fboTarget = GL_READ_FRAMEBUFFER;
              glBindFramebuffer(GL_READ_FRAMEBUFFER, glID);
            }
            if (write) {
              fboTarget = GL_DRAW_FRAMEBUFFER;
              glBindFramebuffer(GL_DRAW_FRAMEBUFFER, glID);
            }
          }
        };

        struct RebindTextureRenderTarget {
          struct Attachment {
            GLint       layer; // Level/Face for 3D textures
            GLint       mipLevel;
            GLenum      slot;
            GLenum      target;
            GLTexture * pTexture;
          };

          GLRenderTarget * pRenderTarget;
          TextureType      fbClass;
          Attachment       depth;
          Attachment       colour[MaxColourAttachments];
          GLenum           colourReadAttachment;
          MapAccess        access;

          static void execute(RebindTextureRenderTarget const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            bool write = (pCmd->access & MapAccess_Write) > 0;
            bool read  = (pCmd->access & MapAccess_Read) > 0;

            uint32_t & glID = pCmd->pRenderTarget->textures.glID;

            if (glID == 0)
              glGenFramebuffers(1, &glID);

            GLint  numAttachments = 0;
            GLenum activeAttachments[MaxColourAttachments];
            GLenum fboTarget = GL_NONE;

            if (read) {
              fboTarget = GL_READ_FRAMEBUFFER;
              glBindFramebuffer(GL_READ_FRAMEBUFFER, glID);
            }
            if (write) {
              fboTarget = GL_DRAW_FRAMEBUFFER;
              glBindFramebuffer(GL_DRAW_FRAMEBUFFER, glID);
            }

            for (int64_t index = 0; index < MaxColourAttachments; ++index) {
              auto & attachment   = pCmd->colour[index];
              if (attachment.pTexture != InvalidGraphicsResource) {
                auto & glTex         = *attachment.pTexture;
                attachment.target;
                switch (glTex.type) {
                case TextureType_2D:
                case TextureType_CubeMap: glFramebufferTexture2D(fboTarget, attachment.slot, attachment.target, glTex.glID, (GLint)attachment.mipLevel); break;
                case TextureType_2DArray:
                  glFramebufferTextureLayer(fboTarget, attachment.slot, glTex.glID, (GLint)attachment.mipLevel, (GLint)attachment.layer);
                  break;
                case TextureType_3D:
                  glFramebufferTexture3D(fboTarget, attachment.slot, attachment.target, glTex.glID, (GLint)attachment.mipLevel, (GLint)attachment.layer);
                  break;
                }
                activeAttachments[numAttachments++] = attachment.slot;
              } else {
                switch (pCmd->fbClass) {
                case TextureType_2D: glFramebufferTexture2D(fboTarget, attachment.slot, GL_TEXTURE_2D, 0, 0); break;
                case TextureType_3D:
                case TextureType_2DArray: glFramebufferTextureLayer(fboTarget, attachment.slot, 0, 0, 0); break;
                case TextureType_CubeMap: glFramebufferTexture3D(fboTarget, attachment.slot, GL_TEXTURE_3D, 0, 0, 0); break;
                }
              }
            }

            if (pCmd->depth.pTexture != InvalidGraphicsResource) {
              auto & attachment    = pCmd->depth;
              auto & glTex         = *attachment.pTexture;
              switch (glTex.type) {
              case TextureType_2D:
              case TextureType_CubeMap: glFramebufferTexture2D(fboTarget, attachment.slot, attachment.target, glTex.glID, (GLint)attachment.mipLevel); break;
              case TextureType_2DArray:
                glFramebufferTextureLayer(fboTarget, attachment.slot, glTex.glID, (GLint)attachment.mipLevel, (GLint)attachment.layer);
                break;
              case TextureType_3D:
                glFramebufferTexture3D(fboTarget, attachment.slot, attachment.target, glTex.glID, (GLint)attachment.mipLevel, (GLint)attachment.layer);
                break;
              }
            } else {
              switch (pCmd->fbClass) {
              case TextureType_2D: glFramebufferTexture2D(fboTarget, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0); break;
              case TextureType_3D:
              case TextureType_2DArray: glFramebufferTextureLayer(fboTarget, GL_DEPTH_STENCIL_ATTACHMENT, 0, 0, 0); break;
              case TextureType_CubeMap: glFramebufferTexture3D(fboTarget, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_3D, 0, 0, 0); break;
              }
            }

            if (write) {
              if (numAttachments == 0) {
                glDrawBuffer(GL_NONE);
              } else {
                glDrawBuffers(numAttachments, activeAttachments);
              }
            }

            if (read) {
              if (numAttachments == 0) {
                glReadBuffer(GL_NONE);
              } else {
                glReadBuffer(pCmd->colourReadAttachment);
              }
            }
          }
        };

        struct BindWindowRenderTarget {
          HDC       hDC;
          MapAccess access;

          static void execute(BindWindowRenderTarget const * pCmd, GraphicsDevice_OpenGL * pDevice, CommandBuffer const * pBuffer) {
            bool write = (pCmd->access & MapAccess_Write) > 0;
            bool read  = (pCmd->access & MapAccess_Read) > 0;

            wglMakeCurrent(pCmd->hDC, pDevice->getGLRC());

            if (read) {
              glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            }

            if (write) {
              glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            }

            glDrawBuffer(write ? GL_BACK : GL_NONE);
            glReadBuffer(read ? GL_BACK : GL_NONE);
          }
        };

        struct SetState {
          int64_t                          count;
          CommandBuffer::Serialized<State> states;

          static void execute(SetState const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            auto                             pStateManager = pDevice->getStateManager();
            CommandBuffer::Serialized<State> stateHandle   = pCmd->states;

            for (int64_t i = 0; i < pCmd->count; ++i) {
              int64_t sz = 0;
              pStateManager->set(pBuffer->deserialize(stateHandle, &sz));
              stateHandle.offset += sz;
            }
          }
        };

        struct PushState {
          int64_t                          count;
          CommandBuffer::Serialized<State> states;

          static void execute(PushState const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            auto                             pStateManager = pDevice->getStateManager();
            CommandBuffer::Serialized<State> stateHandle   = pCmd->states;

            pStateManager->beginGroup();
            for (int64_t i = 0; i < pCmd->count; ++i) {
              int64_t sz = 0;
              pStateManager->set(pBuffer->deserialize(stateHandle, &sz));
              stateHandle.offset += sz;
            }
            pStateManager->endGroup();
          }
        };

        struct PopState {
          static void execute(PopState const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            pDevice->getStateManager()->pop();
          }
        };

        struct Upload {
          GLBuffer *  pGLBuffer;
          int64_t     dataHandle;
          int64_t     dataSize;
          static void execute(Upload const * pCmd, GraphicsDevice * pDevice, CommandBuffer * pAllocator) {
            const Span<const uint8_t> data = pAllocator->read(pCmd->dataHandle, pCmd->dataSize);

            if (pCmd->pGLBuffer->glID == 0)
              glGenBuffers(1, &pCmd->pGLBuffer->glID);

            const GLenum bindPoint = pCmd->pGLBuffer->getBindTarget(false);
            glBindBuffer(bindPoint, pCmd->pGLBuffer->glID);

            if (pCmd->pGLBuffer->allocated < data.size()) {
              glBufferData(bindPoint, data.size(), data.begin(), pCmd->pGLBuffer->glUsage);
              pCmd->pGLBuffer->allocated = data.size();
            } else {
              glBufferSubData(bindPoint, 0, data.size(), data.begin());
            }

            glBindBuffer(bindPoint, 0);
          }
        };

        struct Map {
          GLBuffer * pBuffer;
          GLintptr   offset;
          GLsizeiptr size;
          GLbitfield access;
          std::promise<void *> * pPromise;
          static void execute(Map const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            const GLenum bindPoint = pCmd->pBuffer->getBindTarget(false);

            glBindBuffer(bindPoint, pCmd->pBuffer->glID);
            pCmd->pPromise->set_value(glMapBufferRange(bindPoint, pCmd->offset, pCmd->size, pCmd->access));
            glBindBuffer(bindPoint, 0);
          }
        };

        struct Unmap {
          GLBuffer * pBuffer;

          static void execute(Unmap const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            const GLenum bindPoint = pCmd->pBuffer->getBindTarget(false);

            glBindBuffer(bindPoint, pCmd->pBuffer->glID);
            glUnmapBuffer(bindPoint);
            glBindBuffer(bindPoint, 0);
          }
        };

        struct Download {
          GLBuffer *           pBuffer;
          GLBufferDownload *   pDownload;
          void *               pDst;
          GLenum               bindPoint;
          GLintptr             offset;
          GLsizeiptr           size;
          std::promise<void> * pPromise;

          static void execute(Download const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            auto & buf = *pCmd->pBuffer;
            glBindBuffer(pCmd->bindPoint, buf.glID);
            glGetBufferSubData(pCmd->bindPoint, pCmd->offset, pCmd->size, pCmd->pDownload->storage.begin());
            glBindBuffer(pCmd->bindPoint, 0);
            pCmd->pPromise->set_value();
          }
        };

        struct AllocateTexture {
          GLTexture * pTexture;
          GLenum      format;
          GLenum      type;
          GLenum      internalFormat;
          GLenum      target;
          Vec3i       size;

          static void execute(AllocateTexture const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            auto & tex = *pCmd->pTexture;

            GLenum const & glFormat         = pCmd->format;
            GLenum const & glType           = pCmd->type;
            GLenum const & glInternalFormat = pCmd->internalFormat;
            GLenum const & glTarget         = pCmd->target;
            Vec3i const &  size             = pCmd->size;

            if (tex.glID == 0) {
              glGenTextures(1, &tex.glID);
              glBindTexture(glTarget, tex.glID);
              glTexParameteri(glTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
              glTexParameteri(glTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
              glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            } else {
              glBindTexture(glTarget, tex.glID);
            }

            switch (tex.type) {
            case TextureType_2D: glTexImage2D(glTarget, 0, glInternalFormat, size.x, size.y, GL_NONE, glFormat, glType, nullptr); break;
            case TextureType_2DArray: // Fall-through
            case TextureType_3D: glTexImage3D(glTarget, 0, glInternalFormat, size.x, size.y, size.z, GL_NONE, glFormat, glType, nullptr); break;
            case TextureType_CubeMap: BFC_ASSERT(size.z == 6, "A cubemap must have a depth of 6."); break;
            }

            glBindTexture(glTarget, 0);
          }
        };

        static GLenum ToGLCubeMapFace(CubeMapFace face) {
          return face >= 0 && face < CubeMapFace_Count ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : GL_NONE;
        }

        struct UploadTexture {
          GLTexture * pTexture;
          GLenum      format;
          GLenum      type;
          GLenum      internalFormat;
          GLenum      target;

          int64_t        dataHandle;
          int64_t        dataSize;
          media::Surface surface;

          static void execute(UploadTexture const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            auto & tex = *pCmd->pTexture;

            GLenum const &      glFormat         = pCmd->format;
            GLenum const &      glType           = pCmd->type;
            GLenum const &      glInternalFormat = pCmd->internalFormat;
            GLenum const &      glTarget         = pCmd->target;
            media::Surface      src              = pCmd->surface;
            Span<const uint8_t> data             = pBuffer->read(pCmd->dataHandle, pCmd->dataSize);
            src.pBuffer                          = (void *)data.begin();

            if (tex.glID == 0) {
              glGenTextures(1, &tex.glID);
              glBindTexture(glTarget, tex.glID);
              glTexParameteri(glTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
              glTexParameteri(glTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
              glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            } else {
              glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
              glBindTexture(glTarget, tex.glID);
            }

            switch (tex.type) {
            case TextureType_2D: glTexImage2D(glTarget, 0, glInternalFormat, src.size.x, src.size.y, GL_NONE, glFormat, glType, src.pBuffer); break;
            case TextureType_2DArray: // Fall-through
            case TextureType_3D: glTexImage3D(glTarget, 0, glInternalFormat, src.size.x, src.size.y, src.size.z, GL_NONE, glFormat, glType, src.pBuffer); break;
            case TextureType_CubeMap:
              BFC_ASSERT(src.size.z == 6, "A cubemap must have a depth of 6.");
              for (int64_t i = 0; i < src.size.z; ++i) {
                void * pPixels = getSurfacePixel(src, {0, 0, i});
                glTexImage2D(ToGLCubeMapFace((CubeMapFace)i), 0, glInternalFormat, src.size.x, src.size.y, GL_NONE, glFormat, glType, pPixels);
              }
              break;
            }
            glBindTexture(glTarget, 0);
          }
        };

        struct UploadTextureSubData {
          GLTexture * pTexture;
          GLenum      format;
          GLenum      type;
          GLenum      target;
          Vec3i       offset;

          int64_t        dataHandle;
          int64_t        dataSize;
          media::Surface surface;

          static void execute(UploadTextureSubData const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            BFC_ASSERT(pCmd->pTexture != nullptr, "pTexture cannot be nullptr");

            auto & tex = *pCmd->pTexture;

            BFC_ASSERT(tex.glID != 0, "Texture has not been allocated.");

            GLenum const &      glFormat = pCmd->format;
            GLenum const &      glType   = pCmd->type;
            GLenum const &      glTarget = pCmd->target;
            Vec3i const &       offset   = pCmd->offset;
            media::Surface      src      = pCmd->surface;
            Span<const uint8_t> data     = pBuffer->read(pCmd->dataHandle, pCmd->dataSize);
            src.pBuffer                  = (void *)data.begin();

            glBindTexture(glTarget, tex.glID);
            switch (tex.type) {
            case TextureType_2D: glTexSubImage2D(glTarget, 0, offset.x, offset.y, src.size.x, src.size.y, glFormat, glType, src.pBuffer); break;
            case TextureType_3D:
              glTexSubImage3D(glTarget, 0, offset.x, offset.y, offset.z, src.size.x, src.size.y, src.size.z, glFormat, glType, src.pBuffer);
              break;
            case TextureType_CubeMap: break;
            }
            glBindTexture(glTarget, 0);
          }
        };

        struct GenerateMipMaps {
          GLTexture * pTexture;
          GLenum      target;

          static void execute(GenerateMipMaps const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            auto & tex = *pCmd->pTexture;
            if (glGenerateTextureMipmap != 0 && glTextureParameteri != 0) { // Use 'named' function if available
              glGenerateTextureMipmap(tex.glID);
              glTextureParameteri(tex.glID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
              glTextureParameteri(tex.glID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            } else {
              glBindTexture(pCmd->target, tex.glID);
              glGenerateMipmap(pCmd->target);
              glTexParameteri(pCmd->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
              glTexParameteri(pCmd->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              glBindTexture(pCmd->target, 0);
            }
          }
        };

        struct DownloadTexture {
          GLTexture * pTexture;
          GLTextureDownload * pDownload;
          static void execute(DownloadTexture const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            
          }
        };

        struct SetUniform {
          GLProgram * pProgram;
          int64_t     uniformIndex;
          int64_t     dataHandle;
          int64_t     dataSize;

          static void execute(SetUniform const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            Span<const uint8_t> data = pBuffer->read(pCmd->dataHandle, pCmd->dataSize);
            const float *       f32  = (const float *)data.begin();
            const int32_t *     i32  = (const int32_t *)data.begin();
            const uint32_t *    ui32 = (const uint32_t *)data.begin();

            const GLProgram &          prog        = *pCmd->pProgram;
            const GLProgram::Uniform & uniformDesc = prog.uniforms[pCmd->uniformIndex];
            uint32_t                   pid         = prog.glID;
            uint32_t                   l           = uniformDesc.glLoc;

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
        };

        struct SetNamedUniform {
          GLProgram * pProgram;
          int64_t     nameHandle;
          int64_t     nameSize;
          int64_t     dataHandle;
          int64_t     dataSize;

          static void execute(SetNamedUniform const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            Span<const char> name = (bfc::Span<const char>)pBuffer->read(pCmd->nameHandle, pCmd->nameSize);

            SetUniform cmd;
            cmd.pProgram   = pCmd->pProgram;
            cmd.dataHandle = pCmd->dataHandle;
            cmd.dataSize   = pCmd->dataSize;

            for (int64_t i = 0; i < cmd.pProgram->getUniformCount(); ++i) {
              ProgramUniformDesc desc;
              cmd.pProgram->getUniformDesc(i, &desc);
              if (desc.name == StringView(name.begin(), name.end())) {
                cmd.uniformIndex = i;
                return SetUniform::execute(&cmd, pDevice, pBuffer);
              }
            }
          }
        };

        struct SetBufferBinding {
          GLProgram * pProgram;
          int64_t     bufferIndex;
          int64_t     bindPoint;

          static void execute(SetBufferBinding const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            GLProgram &         prog       = *pCmd->pProgram;
            GLProgram::Buffer & bufferDesc = prog.buffers[pCmd->bufferIndex];

            if (pCmd->bindPoint != bufferDesc.bindPoint) {
              bufferDesc.bindPoint = (uint32_t)pCmd->bindPoint;
              if (bufferDesc.isStorageBuffer)
                glShaderStorageBlockBinding(prog.glID, bufferDesc.bufferIndex, bufferDesc.bindPoint);
              else
                glUniformBlockBinding(prog.glID, bufferDesc.bufferIndex, bufferDesc.bindPoint);
            }
          }
        };

        struct SetNamedBufferBinding {
          GLProgram * pProgram;
          int64_t     nameHandle;
          int64_t     nameSize;
          int64_t     bufferIndex;
          int64_t     bindPoint;

          static void execute(SetNamedBufferBinding const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            Span<const char> name = (bfc::Span<const char>)pBuffer->read(pCmd->nameHandle, pCmd->nameSize);

            SetBufferBinding cmd;
            cmd.pProgram   = pCmd->pProgram;
            cmd.bindPoint = pCmd->bindPoint;

            for (int64_t i = 0; i < cmd.pProgram->getBufferCount(); ++i) {
              ProgramBufferDesc desc;
              cmd.pProgram->getBufferDesc(i, &desc);
              if (desc.name == StringView(name.begin(), name.end())) {
                cmd.bufferIndex = i;
                return SetBufferBinding::execute(&cmd, pDevice, pBuffer);
              }
            }
          }
        };

        struct SetTextureBinding {
          GLProgram * pProgram;
          int64_t     textureIndex;
          int64_t     bindPoint;
          static void execute(SetTextureBinding const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            GLProgram &          prog        = *pCmd->pProgram;
            GLProgram::Texture & textureDesc = prog.textures[pCmd->textureIndex];

            if (pCmd->bindPoint != textureDesc.bindPoint) {
              textureDesc.bindPoint = (int32_t)pCmd->bindPoint;
              glProgramUniform1i(prog.glID, textureDesc.glLoc, textureDesc.bindPoint);
            }
          }
        };

        struct SetNamedTextureBinding {
          GLProgram * pProgram;
          int64_t     nameHandle;
          int64_t     nameSize;
          int64_t     textureIndex;
          int64_t     bindPoint;

          static void execute(SetNamedTextureBinding const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            Span<const char> name = (bfc::Span<const char>)pBuffer->read(pCmd->nameHandle, pCmd->nameSize);

            SetTextureBinding cmd;
            cmd.pProgram   = pCmd->pProgram;
            cmd.bindPoint    = pCmd->bindPoint;

            for (int64_t i = 0; i < cmd.pProgram->getTextureCount(); ++i) {
              ProgramTextureDesc desc;
              cmd.pProgram->getTextureDesc(i, &desc);
              if (desc.name == StringView(name.begin(), name.end())) {
                cmd.textureIndex = i;
                return SetTextureBinding::execute(&cmd, pDevice, pBuffer);
              }
            }
          }
        };

        struct Clear {
          RGBAu8 colour;

          static void execute(Clear const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            pDevice->getStateManager()->apply();

            glClearColor(pCmd->colour.r / 255.0f, pCmd->colour.g / 255.0f, pCmd->colour.b / 255.0f, pCmd->colour.a / 255.0f);
            glClearDepth(1.0f);
            glClearStencil(0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
          }
        };

        struct Swap {
          HDC         hDC;
          static void execute(Swap const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            SwapBuffers(pCmd->hDC);
          }
        };

        struct Draw {
          GLsizei elementCount;
          GLint   elementOffset;
          GLenum  primType;
          GLsizei instanceCount;

          static void execute(Draw const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            pDevice->getStateManager()->apply();

            if (pCmd->instanceCount == 1) {
              glDrawArrays(pCmd->primType, pCmd->elementOffset, pCmd->elementCount);
            } else {
              glDrawArraysInstanced(pCmd->primType, pCmd->elementOffset, pCmd->elementCount, pCmd->instanceCount);
            }
          }
        };

        struct DrawIndexed {
          GLsizei  elementCount;
          uint64_t elementOffset;
          GLsizei  instanceCount;
          GLenum   indexType;
          GLenum   primType;

          static void execute(DrawIndexed const * pCmd, GraphicsDevice * pDevice, CommandBuffer const * pBuffer) {
            pDevice->getStateManager()->apply();

            if (pCmd->instanceCount == 1) {
              glDrawElements(pCmd->primType, pCmd->elementCount, pCmd->indexType, (void *)pCmd->elementOffset);
            } else {
              glDrawElementsInstanced(pCmd->primType, pCmd->elementCount, pCmd->indexType, (void *)pCmd->elementOffset, pCmd->instanceCount);
            }
          }
        };
      } // namespace OpenGL
    }   // namespace impl
  }     // namespace graphics
} // namespace bfc
