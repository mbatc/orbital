#include "Renderables.h"
#include "mesh/Mesh.h"

namespace engine {
  MeshRenderable::MeshRenderable(bfc::Mat4d const & modelMatrix, bfc::Mesh const & mesh, int64_t subMeshIndex, bfc::Ref<bfc::Material> pMaterial,
                                         bfc::Ref<bfc::graphics::Program> pProgram)
    : MeshRenderable(modelMatrix, MeshRenderable::calcNormalMatrix(modelMatrix), mesh, subMeshIndex, pMaterial, pProgram) {}

  MeshRenderable::MeshRenderable(bfc::Mat4d const & _modelMatrix, bfc::Mat4d const & _normalMatrix, bfc::Mesh const & mesh, int64_t subMeshIndex,
                                         bfc::Ref<bfc::Material> pMaterial, bfc::Ref<bfc::graphics::Program> pProgram) {
    auto const & sm = mesh.getSubMesh(subMeshIndex);

    bfc::geometry::Boxf bounds = sm.bounds;
    bounds.transform(_modelMatrix);

    this->elementOffset = sm.elmOffset;
    this->elementCount  = sm.elmCount;
    this->modelMatrix   = _modelMatrix;
    this->normalMatrix  = _normalMatrix;
    this->vertexArray   = mesh.getVertexArray();
    this->bounds        = bounds;
    this->shader        = pProgram;
    this->primitiveType = bfc::PrimitiveType_Triangle;

    if (pMaterial == nullptr) {
      this->materialBuffer = bfc::InvalidGraphicsResource;
      for (auto & texture : this->materialTextures) {
        texture = bfc::InvalidGraphicsResource;
      }
    } else {
      this->materialBuffer = *pMaterial;
      for (auto & [i, texture] : bfc::enumerate(pMaterial->textures)) {
        if (texture != nullptr) {
          this->materialTextures[i] = texture;
        }
      }
    }
  }

  bfc::Mat4d MeshRenderable::calcNormalMatrix(bfc::Mat4d const & modelMatrix) {
    return bfc::renderer::calcNormalMatrix(glm::inverse(modelMatrix));
  }
} // namespace engine
