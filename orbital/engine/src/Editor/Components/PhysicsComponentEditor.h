#pragma once

#include "Editor/LevelEditor.h"
#include "Physics/Physics.h"

namespace engine {
  class ColliderCapsuleEditor : public LevelEditor::ComponentEditor<components::ColliderCapsule> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                      components::ColliderCapsule * pTransform) override;
  };

  class ColliderCubeEditor : public LevelEditor::ComponentEditor<components::ColliderCube> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                      components::ColliderCube * pTransform) override;
  };

  class ColliderSphereEditor : public LevelEditor::ComponentEditor<components::ColliderSphere> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                      components::ColliderSphere * pTransform) override;
  };

  class ColliderMeshEditor : public LevelEditor::ComponentEditor<components::ColliderMesh> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                      components::ColliderMesh * pTransform) override;
  };

  class RigidBodyEditor : public LevelEditor::ComponentEditor<components::RigidBody> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                      components::RigidBody * pTransform) override;
  };
}

