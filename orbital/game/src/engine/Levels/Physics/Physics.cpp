#include "Levels/Physics/Physics.h"
#include "Levels/Physics/PhysicsComponents.h"
#include "Levels/CoreComponents.h"
#include "Levels/LevelSystem.h"
#include "Levels/Level.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"

using namespace bfc;

namespace engine {
  class Physics::World {
  public:
    class EntityMotionSynchronizer : public btMotionState {
    public:
      EntityMotionSynchronizer(Level * pLevel, EntityID entity)
        : m_pLevel(pLevel)
        , m_entity(entity) {}

      virtual void getWorldTransform(btTransform & worldTrans) const override {
        components::Transform * pTarget = m_pLevel->tryGet<components::Transform>(m_entity);
        if (pTarget == nullptr) {
          worldTrans.setIdentity();
          return;
        }

        Vec3d origin = pTarget->globalTranslation(m_pLevel);
        Quatd ori    = pTarget->globalOrientation(m_pLevel);

        worldTrans.setOrigin({(btScalar)origin.x, (btScalar)origin.y, (btScalar)origin.z});
        worldTrans.setRotation(btQuaternion((btScalar)ori.x, (btScalar)ori.y, (btScalar)ori.z, (btScalar)ori.w));
      }

      virtual void setWorldTransform(const btTransform & worldTrans) override {
        components::Transform * pTarget = m_pLevel->tryGet<components::Transform>(m_entity);
        if (pTarget == nullptr) {
          return;
        }

        btVector3 origin = worldTrans.getOrigin();
        btQuaternion ori = worldTrans.getRotation();
        pTarget->setGlobalTranslation(m_pLevel, {origin.x(), origin.y(), origin.z()});
        pTarget->setGlobalOrientation(m_pLevel, {ori.w(), ori.x(), ori.y(), ori.z()});
      }

    private:
      Level *  m_pLevel = nullptr;
      EntityID m_entity;
    };

    World(btDefaultCollisionConstructionInfo info = {})
      : collisionConfig(info)
      , solver()
      , broadphase(&pairCache)
      , dispatcher(&collisionConfig)
      , world(&dispatcher, &broadphase, &solver, &collisionConfig)
    {}

    btHashedOverlappingPairCache        pairCache;
    btDefaultCollisionConfiguration     collisionConfig;
    btSequentialImpulseConstraintSolver solver;
    btDbvtBroadphase                    broadphase;
    btCollisionDispatcher               dispatcher;
    btDiscreteDynamicsWorld             world;

    // Bind a rigidbody to an entity in a level.
    // The entity transforms are synchronized with the rigidbody.
    struct RigidBodyBinding {
      static btRigidBody::btRigidBodyConstructionInfo SetMotionState(
        btRigidBody::btRigidBodyConstructionInfo data,
        EntityMotionSynchronizer * pSync) {
        data.m_motionState = pSync;
        return data;
      }

      RigidBodyBinding(Level * pLevel, EntityID entity, btRigidBody::btRigidBodyConstructionInfo const & rigidBodyInfo)
        : motionSync(pLevel, entity)
        , body(SetMotionState(rigidBodyInfo, &motionSync)) {}

      EntityMotionSynchronizer motionSync;
      btRigidBody body;
    };

    // Storage for rigidbodies and colliders.
    Pool<RigidBodyBinding *>  rigidBodies;
    Pool<btCollisionObject *> collisionObjects;

    btRigidBody * getRigidBody(Ref<int64_t> const & handle) const {
      if (!handle || !rigidBodies.isUsed(*handle))
        return nullptr;

      return &rigidBodies[*handle]->body;
    }

    btCollisionObject * getCollider(Ref<int64_t> const & handle) const {
      if (!handle || !collisionObjects.isUsed(*handle))
        return nullptr;

      return collisionObjects[*handle];
    }

    Ref<int64_t> addRigidBody(Level *pLevel, EntityID entity, double mass, Ref<btCollisionShape> const & pShape) {
      if (pShape == nullptr)
        return nullptr;

      btRigidBody::btRigidBodyConstructionInfo info((btScalar)mass, nullptr, pShape.get());

      RigidBodyBinding * pBinding = new RigidBodyBinding(pLevel, entity, info);

      world.addRigidBody(&pBinding->body);

      int64_t index = rigidBodies.emplace(pBinding);

      return Ref<int64_t>(new int64_t(index), [this, pBinding, pShape](int64_t * pIndex) {
        world.removeRigidBody(&pBinding->body);

        // Destroy the collision object when the instance handle is erased.
        rigidBodies.erase(*pIndex);
        delete pIndex;
        delete pBinding;
      });
    }

    Ref<int64_t> addCollider(Ref<btCollisionShape> const & pShape) {
      if (pShape == nullptr)
        return nullptr;

      btCollisionObject * pCollider = new btCollisionObject();
      pCollider->setCollisionShape(pShape.get());

      world.addCollisionObject(pCollider);

      int64_t index = collisionObjects.emplace(pCollider);

      return Ref<int64_t>(new int64_t(index), [this, pCollider, pShape](int64_t * pIndex) {
        world.removeCollisionObject(pCollider);

        // Destroy the collision object when the instance handle is erased.
        collisionObjects.erase(*pIndex);
        delete pIndex;
        delete pCollider;
      });
    }
  };

  class LevelPhysicsSystem
    : public ILevelUpdate
    , public ILevelPlay
    , public ILevelStop {
  public:
    virtual void update(Level * pLevel, bfc::Timestamp dt) override {
      Ref<Physics::World> pPhysicsData = pLevel->getData<Physics::World>();

      pPhysicsData->world.stepSimulation((btScalar)dt.secs());
    }

    virtual void play(Level * pLevel) override {
      Ref<Physics::World> pPhysicsData = pLevel->addData<Physics::World>();
      if (pPhysicsData == nullptr) {
        return;
      }

      for (auto & [cube] : pLevel->getView<components::Collider_Cube>()) {
        EntityID               entity   = pLevel->toEntity(&cube);
        components::Collider & collider = pLevel->add<components::Collider>(entity);

        Vec3d halfSize  = cube.size / 2.0;
        collider.pShape = NewRef<btBoxShape>(btVector3((btScalar)halfSize.x, (btScalar)halfSize.y, (btScalar)halfSize.z));
      }

      for (auto & [sphere] : pLevel->getView<components::Collider_Sphere>()) {
        EntityID               entity   = pLevel->toEntity(&sphere);
        components::Collider & collider = pLevel->add<components::Collider>(entity);

        collider.pShape = NewRef<btSphereShape>((btScalar)sphere.radius);
      }

      for (auto & [capsule] : pLevel->getView<components::Collider_Capsule>()) {
        EntityID       entity = pLevel->toEntity(&capsule);
        components::Collider &collider = pLevel->add<components::Collider>(entity);

        collider.pShape = NewRef<btCapsuleShape>((btScalar)capsule.radius, (btScalar)capsule.length);
      }

      for (auto & [transform, rigidbody, collider] : pLevel->getView<components::Transform, components::RigidBody, components::Collider>()) {
        rigidbody.instanceHandle = nullptr; // Release previous rigidbody
        rigidbody.instanceHandle = pPhysicsData->addRigidBody(pLevel, pLevel->toEntity(&rigidbody), rigidbody.mass, collider.pShape);
      }

      for (auto & [transform, collider] : pLevel->getView<components::Transform, components::Collider>()) {
        EntityID entity = pLevel->toEntity(&transform);
        if (pLevel->has<components::RigidBody>(entity)) {
          continue;
        }

        collider.instanceHandle  = nullptr; // Release previous rigidbody
        collider.instanceHandle = pPhysicsData->addCollider(collider.pShape);
      }
    }

    virtual void stop(Level * pLevel) override {

    }
  };

  Physics::Physics()
    : Subsystem(bfc::TypeID<Physics>(), "physics")
  {}

  bool Physics::init(Application * pApp) {
    registerLevelSystem<LevelPhysicsSystem>();

    registerComponentType<components::RigidBody>("physics.rigidbody");
    registerComponentType<components::Collider>("physics.collider");
    registerComponentType<components::Collider_Capsule>("physics.collider.capsule");
    registerComponentType<components::Collider_Mesh>("physics.collider.mesh");
    registerComponentType<components::Collider_Sphere>("physics.collider.sphere");
    registerComponentType<components::Collider_Cube>("physics.collider.cube");
    return true;
  }

  void Physics::loop(Application * pApp) {

  }
} // namespace engine
