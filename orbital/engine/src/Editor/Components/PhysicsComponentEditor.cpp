#include "PhysicsComponentEditor.h"
#include "Application.h"
#include "ui/Widgets.h"

namespace engine {
  void ColliderCapsuleEditor::draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                                   components::ColliderCapsule * pCollider) {
    double height = pCollider->getHeight();
    double radius = pCollider->getRadius();

    bool changed = false;
    changed |= bfc::ui::Input("Height", &height);
    changed |= bfc::ui::Input("Radius", &radius);

    if (changed)
      pCollider->set(height, radius);
  }

  void ColliderCubeEditor::draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                                components::ColliderCube * pCollider) {
    bfc::Vec3d size = pCollider->getSize();

    bool changed = false;
    changed |= bfc::ui::Input("Size", &size);
    
    if (changed)
      pCollider->setSize(size);
  }

  void ColliderSphereEditor::draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                                  components::ColliderSphere * pCollider) {
    double radius = pCollider->getRadius();

    bool changed = false;
    changed |= bfc::ui::Input("Radius", &radius);

    if (changed)
      pCollider->setRadius(radius);
  }

  void ColliderMeshEditor::draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                                components::ColliderMesh * pCollider) {
    bfc::Ref<bfc::Mesh> pMesh = pCollider->getMesh();

    LevelEditor::drawAssetSelector("Mesh", &pMesh);

    pCollider->setMesh(rendering::getDevice(), pMesh);
  }

  void RigidBodyEditor::draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                             components::RigidBody * pRigidBody) {
    double mass = pRigidBody->getMass();

    bool changed = false;
    changed |= bfc::ui::Input("Mass", &mass);

    if (changed)
      pRigidBody->setMass(mass);
  }
} // namespace engine