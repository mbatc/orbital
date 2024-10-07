#pragma once

#include "Level.h"
#include "math/MathTypes.h"
#include "util/UUID.h"

namespace bfc {
  class Mesh;
}

namespace components {
  struct Name {
    bfc::String name;
  };

  struct ID {
    bfc::UUID uuid;
  };

  class Transform {
  public:
    bfc::Vec3d translation() const;
    bfc::Quatd orientation() const;
    bfc::Vec3d scale() const;

    bfc::Vec3d globalTranslation(engine::Level const * pLevel) const;
    bfc::Quatd globalOrientation(engine::Level const * pLevel) const;
    bfc::Vec3d globalScale(engine::Level const * pLevel) const;

    bfc::Vec3d parentTranslation(engine::Level const * pLevel) const;
    bfc::Quatd parentOrientation(engine::Level const * pLevel) const;
    bfc::Vec3d parentScale(engine::Level const * pLevel) const;

    engine::EntityID parent() const;

    void setTranslation(bfc::Vec3d const & translation);
    void setOrientation(bfc::Vec3d const & orientation);
    void setScale(bfc::Vec3d const & scale);

    void setGlobalTranslation(engine::Level const * pLevel, bfc::Vec3d const & translation);
    void setGlobalOrientation(engine::Level const * pLevel, bfc::Vec3d const & orientation);
    void setGlobalScale(engine::Level const * pLevel, bfc::Vec3d const & scale);

    void setParent(engine::Level const * pLevel, engine::EntityID const & entityID) const;

  private:
    engine::EntityID m_parent;

    bfc::Vec3d m_translation = {0, 0, 0};
    bfc::Quatd m_orientation = glm::identity<bfc::Quatd>();
    bfc::Vec3d m_scale       = {1, 1, 1};
  };

  struct Light {};

  struct StaticMesh {
    bool                castShadows;
    bfc::Ref<bfc::Mesh> pMesh;
  };
} // namespace components
