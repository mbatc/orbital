#include "Physics.h"
#include "../../../../vendor/bullet3/src/btBulletDynamicsCommon.h"
#include "Levels/CoreComponents.h"

namespace engine {
  static btVector3 ToBt(bfc::Vector3<btScalar> const & v) {
    return {(btScalar)v.x, (btScalar)v.y, (btScalar)v.z};
  }

  static bfc::Vector3<btScalar> FromBt(btVector3 const & v) {
    return {v.x(), v.y(), v.z()};
  }

  static btMatrix3x3 ToBt(bfc::Matrix3<btScalar> const & m) {
    // Column Major -> Row Major
    return btMatrix3x3(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2]);
  }

  static bfc::Matrix3<btScalar> FromBt(btMatrix3x3 const & m) {
    // Row Major -> Column Major
    return bfc::Matrix3<btScalar>(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2]);
  }

  static btTransform ToBt(Level * pLevel, components::Transform const & t) {
    return btTransform(ToBt(glm::mat3_cast(t.globalOrientation(pLevel))), ToBt(t.globalTranslation(pLevel)));
  }

  namespace internal {
    // TODO: Implement trait to enable static addresses for level components so we can use btCollisionShape as the component
    struct PhysicsShape {
      bfc::Ref<btCollisionShape> shape;
    };

    // TODO: Implement trait to enable static addresses for level components so we can use btCollisionObject as the component
    struct PhysicsCollider {
      bfc::Ref<btCollisionObject> obj;
    };

    struct PhysicsMotionSync : btMotionState {
      Level *      pLevel;
      EntityID     entity;
      virtual void getWorldTransform(btTransform & worldTrans) const {
        auto pTransform = pLevel->tryGet<components::Transform>(entity);
        if (pTransform == nullptr)
          return;

        worldTrans = ToBt(pLevel, *pTransform);
      }

      virtual void setWorldTransform(const btTransform & worldTrans) {
        auto pTransform = pLevel->tryGet<components::Transform>(entity);
        if (pTransform == nullptr)
          return;

        bfc::Vec3d const translation = FromBt(worldTrans.getOrigin());
        bfc::Mat3d const basis       = FromBt(worldTrans.getBasis());

        pTransform->setGlobalTranslation(pLevel, translation);
        pTransform->setGlobalOrientation(pLevel, glm::quat_cast(basis));
      }
    };

    // TODO: Implement trait to enable static addresses for level components so we can use btRigidBody as the component
    struct PhysicsRigidBody {
      bfc::Ref<btCollisionShape>        shape; ///< Shape used by the rigidbody
      bfc::Ref<btRigidBody>             body;
      bfc::Ref<PhysicsMotionSync>       sync;
      bfc::Ref<btDiscreteDynamicsWorld> world;
    };
  } // namespace internal

  template<>
  struct LevelComponent_OnPreErase<internal::PhysicsRigidBody> {
    inline static void onPreErase(internal::PhysicsRigidBody * pComponent, Level * pLevel) {
      pComponent->world->removeRigidBody(pComponent->body.get());
    }
  };

  void Physics::created(Level * pLevel) {
    auto pCollisionConfiguration = pLevel->addData<btDefaultCollisionConfiguration>();
    auto pDispatcher             = pLevel->addData<btCollisionDispatcher>(pCollisionConfiguration.get());
    auto pOverlappingPairCache   = pLevel->addData<btDbvtBroadphase>();
    auto pSolver                 = pLevel->addData<btSequentialImpulseConstraintSolver>();

    pLevel->addData<btDiscreteDynamicsWorld>(pDispatcher.get(), pOverlappingPairCache.get(), pSolver.get(),
                                             pCollisionConfiguration.get());
  }

  void Physics::update(Level * pLevel, bfc::Timestamp dt) {
    auto pWorld = pLevel->getData<btDiscreteDynamicsWorld>();

    pWorld->stepSimulation((btScalar)dt.secs());
  }

  void Physics::activate(Level * pLevel) {
    auto pWorld = pLevel->getData<btDiscreteDynamicsWorld>();

    for (auto & [sphere] : pLevel->getView<components::ColliderSphere>()) {
      auto                   entityId = pLevel->toEntity(&sphere);
      internal::PhysicsShape shape;
      shape.shape = bfc::NewRef<btSphereShape>((btScalar)sphere.radius);
      pLevel->replace<internal::PhysicsShape>(entityId, shape);
    }

    for (auto & [cube] : pLevel->getView<components::ColliderCube>()) {
      auto                   entityId = pLevel->toEntity(&cube);
      internal::PhysicsShape shape;
      shape.shape = bfc::NewRef<btBoxShape>(ToBt(cube.size / 2.0));
      pLevel->replace<internal::PhysicsShape>(entityId, shape);
    }

    for (auto & [capsule] : pLevel->getView<components::ColliderCapsule>()) {
      auto                   entityId = pLevel->toEntity(&capsule);
      internal::PhysicsShape shape;
      shape.shape = bfc::NewRef<btCapsuleShape>((btScalar)capsule.radius, (btScalar)capsule.height);
      pLevel->replace<internal::PhysicsShape>(entityId, shape);
    }

    for (auto & [shape, body, transform] :
         pLevel->getView<internal::PhysicsShape, components::RigidBody, components::Transform>()) {
      auto entityId = pLevel->toEntity(&transform);

      internal::PhysicsRigidBody rigidbody;
      rigidbody.shape        = shape.shape;
      rigidbody.sync         = bfc::NewRef<internal::PhysicsMotionSync>();
      rigidbody.sync->pLevel = pLevel;
      rigidbody.sync->entity = entityId;
      rigidbody.world        = pWorld;

      btRigidBody::btRigidBodyConstructionInfo params((btScalar)body.mass, rigidbody.sync.get(), shape.shape.get());
      rigidbody.body = bfc::NewRef<btRigidBody>(params);
      rigidbody.body->setWorldTransform(ToBt(pLevel, transform));

      pWorld->addRigidBody(rigidbody.body.get());
      pLevel->replace<internal::PhysicsRigidBody>(entityId, rigidbody);
    }
  }

  void Physics::pause(Level * pLevel) {}

  void Physics::stop(Level * pLevel) {}

  void Physics::collectRenderData(RenderView * pRenderView, Level const * pLevel) {
    // Debug render info
  }
} // namespace engine
