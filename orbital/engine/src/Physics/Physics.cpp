#include "Physics.h"
#include "../../../../vendor/bullet3/src/btBulletDynamicsCommon.h"
#include "Levels/CoreComponents.h"
#include "Assets/BuiltinAssets.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderData.h"
#include "Rendering/Renderables.h"

namespace {
  btVector3 ToBt(bfc::Vector3<btScalar> const & v) {
    return {(btScalar)v.x, (btScalar)v.y, (btScalar)v.z};
  }

  bfc::Vector3<btScalar> FromBt(btVector3 const & v) {
    return {v.x(), v.y(), v.z()};
  }

  btMatrix3x3 ToBt(bfc::Matrix3<btScalar> const & m) {
    // Column Major -> Row Major
    return btMatrix3x3(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2]);
  }

  bfc::Matrix3<btScalar> FromBt(btMatrix3x3 const & m) {
    // Row Major -> Column Major
    return bfc::Matrix3<btScalar>(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2]);
  }

  btTransform ToBt(engine::Level * pLevel, components::Transform const & t) {
    return btTransform(ToBt(glm::mat3_cast(t.globalOrientation(pLevel))), ToBt(t.globalTranslation(pLevel)));
  }
}

namespace components {
  ColliderCube::ColliderCube(bfc::Vec3d const & size) {
    setSize(size);
  }

  bfc::Vec3d ColliderCube::getSize() const {
    return m_size;
  }

  void ColliderCube::setSize(bfc::Vec3d const & size) {
    m_pImpl = bfc::NewRef<btBoxShape>(ToBt(size / 2.0));
    m_size  = size;
  }

  bfc::Ref<void> ColliderCube::getImpl() const {
    return m_pImpl;
  }

  ColliderSphere::ColliderSphere(double radius) {
    m_pImpl = bfc::NewRef<btSphereShape>((btScalar)radius);
  }

  double ColliderSphere::getRadius() const {
    auto pShape = std::static_pointer_cast<btSphereShape>(m_pImpl);

    return pShape->getRadius();
  }

  void ColliderSphere::setRadius(double radius) {
    m_pImpl = bfc::NewRef<btSphereShape>((btScalar)radius);
  }

  bfc::Ref<void> ColliderSphere::getImpl() const {
    return m_pImpl;
  }

  ColliderCapsule::ColliderCapsule(double height, double radius) {
    set(height, radius);
  }

  double ColliderCapsule::getHeight() const {
    return m_height;
  }

  void ColliderCapsule::setHeight(double height) {
    set(height, m_radius);
  }

  double ColliderCapsule::getRadius() const {
    return m_radius;
  }

  void ColliderCapsule::setRadius(double radius) {
    set(m_height, radius);
  }

  void ColliderCapsule::set(double height, double radius) {
    m_height = height;
    m_radius = radius;
    m_pImpl  = bfc::NewRef<btCapsuleShape>((btScalar)m_radius, (btScalar)m_height);
  }

  bfc::Ref<void> ColliderCapsule::getImpl() const {
    return m_pImpl;
  }

  double RigidBody::getMass() const {
    return m_mass;
  }

  void RigidBody::setMass(double mass) {
    m_mass = mass;
  }
} // namespace components

namespace engine {
  namespace internal {
    // class BulletMeshInterface : public btStridingMeshInterface {
    // public:
    //   BulletMeshInterface()
    //     : {
    // 
    //   }
    // 
    //   virtual void getLockedVertexIndexBase(unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride,
    //     unsigned char** indexbase, int& indexstride, int& numfaces,
    //     PHY_ScalarType& indicestype, int subpart = 0) {
    //     *vertexbase = (unsigned char*)mesh->positions.begin();
    //     *indexbase  = mesh->triangles;
    //   }
    // 
    //   virtual void getLockedReadOnlyVertexIndexBase(const unsigned char** vertexbase, int& numverts, PHY_ScalarType& type,
    //     int& stride, const unsigned char** indexbase, int& indexstride,
    //     int& numfaces, PHY_ScalarType& indicestype, int subpart = 0) const {
    // 
    //   }
    // 
    //   virtual void unLockVertexBase(int subpart) {
    // 
    //   }
    // 
    //   virtual void unLockReadOnlyVertexBase(int subpart) const {
    // 
    //   }
    // 
    //   virtual int getNumSubParts() const {
    //   }
    // 
    //   virtual void preallocateVertices(int numverts) {
    // 
    //   }
    // 
    //   virtual void preallocateIndices(int numindices) {
    // 
    //   }
    // 
    //   bfc::Ref<bfc::MeshData> mesh;
    // };

    // TODO: Implement trait to enable static addresses for level components so we can use btCollisionShape as the component
    struct PhysicsShape {
      bfc::Ref<btCollisionShape> shape;
    };

    // TODO: Implement trait to enable static addresses for level components so we can use btCollisionObject as the component
    struct PhysicsCollider {
      bfc::Ref<btCollisionShape>        shape; ///< Shape used by the collider
      bfc::Ref<btCollisionObject>       obj;
    };

    struct PhysicsMotionSync : btMotionState {
      engine::Level *  pLevel;
      engine::EntityID entity;
      bfc::Mat4d       previousTransform;

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

        previousTransform = pTransform->globalTransform(pLevel);
      }
    };

    struct PhysicsRigidBody {
      bfc::Ref<btCollisionShape>        shape; ///< Shape used by the rigidbody
      bfc::Ref<btRigidBody>             body;
      bfc::Ref<PhysicsMotionSync>       sync;
    };
  } // namespace internal

  struct Physics::LevelData {
    bfc::Map<EntityID, bfc::Ref<btCollisionShape>> entityShapes;
    bfc::Map<EntityID, internal::PhysicsCollider>  entityColliders;
    bfc::Map<EntityID, internal::PhysicsRigidBody> entityBodies;
    bfc::Vector<EntityID> staleShapes;

    bfc::Ref<btDefaultCollisionConfiguration>     pCollisionConfiguration;
    bfc::Ref<btCollisionDispatcher>               pDispatcher;
    bfc::Ref<btDbvtBroadphase>                    pOverlappingPairCache;
    bfc::Ref<btSequentialImpulseConstraintSolver> pSolver;
    bfc::Ref<btDiscreteDynamicsWorld>             pWorld;


    void synchronize(Level * pLevel) {
      staleShapes.clear();

      for (auto & [sphere] : pLevel->getView<components::ColliderSphere>()) {
        auto entityId = pLevel->toEntity(&sphere);

        bfc::Ref<btCollisionShape> pExistingShape;
        bfc::Ref<btCollisionShape> pNewShape = std::static_pointer_cast<btCollisionShape>(sphere.getImpl());

        if (!entityShapes.tryGet(entityId, &pExistingShape) || pExistingShape != pNewShape) {
          entityShapes.addOrSet(entityId, pNewShape);
          staleShapes.pushBack(entityId);
        }
      }

      for (auto & [cube] : pLevel->getView<components::ColliderCube>()) {
        auto entityId = pLevel->toEntity(&cube);

        bfc::Ref<btCollisionShape> pExistingShape;
        bfc::Ref<btCollisionShape> pNewShape = std::static_pointer_cast<btCollisionShape>(cube.getImpl());

        if (!entityShapes.tryGet(entityId, &pExistingShape) || pExistingShape != pNewShape) {
          entityShapes.addOrSet(entityId, pNewShape);
          staleShapes.pushBack(entityId);
        }
      }

      for (auto & [capsule] : pLevel->getView<components::ColliderCapsule>()) {
        auto entityId = pLevel->toEntity(&capsule);

        bfc::Ref<btCollisionShape> pExistingShape;
        bfc::Ref<btCollisionShape> pNewShape = std::static_pointer_cast<btCollisionShape>(capsule.getImpl());

        if (!entityShapes.tryGet(entityId, &pExistingShape) || pExistingShape != pNewShape) {
          entityShapes.addOrSet(entityId, pNewShape);
          staleShapes.pushBack(entityId);
        }
      }

      bfc::Vector<EntityID> erasedEntities;
      for (auto & [entityId, pShape] : entityShapes) {
        if (!pLevel->contains(entityId)) {
          erasedEntities.pushBack(entityId);
          continue;
        }

        auto * pTransform = pLevel->tryGet<components::Transform>(entityId);
        if (pTransform == nullptr) {
          erasedEntities.pushBack(entityId);
          continue;
        }

        pShape->setLocalScaling(ToBt(pTransform->globalScale(pLevel)));

        if (auto * pBody = pLevel->tryGet<components::RigidBody>(entityId)) {
          internal::PhysicsRigidBody &rigidbody = entityBodies.getOrAdd(entityId);
          if (rigidbody.body == nullptr) {
            rigidbody.shape        = pShape;
            rigidbody.sync         = bfc::NewRef<internal::PhysicsMotionSync>();
            rigidbody.sync->pLevel = pLevel;
            rigidbody.sync->entity = entityId;

            btRigidBody::btRigidBodyConstructionInfo params((btScalar)pBody->getMass(), rigidbody.sync.get(), pShape.get());
            pShape->calculateLocalInertia((btScalar)pBody->getMass(), params.m_localInertia);

            rigidbody.body = bfc::NewRef<btRigidBody>(params);
            rigidbody.body->setWorldTransform(ToBt(pLevel, *pTransform));

            pWorld->addRigidBody(rigidbody.body.get());
          }
        } else {
          internal::PhysicsCollider & collider = entityColliders.getOrAdd(entityId);
          if (collider.obj == nullptr) {
            collider.obj = bfc::NewRef<btCollisionObject>();
            collider.obj->setCollisionShape(pShape.get());
            collider.obj->setWorldTransform(ToBt(pLevel, *pTransform));
            collider.shape = pShape;

            pWorld->addCollisionObject(collider.obj.get());
            entityColliders.addOrSet(entityId, collider);
          }
        }
      }

      for (auto & entityId : staleShapes) {
        auto pShape = entityShapes[entityId];

        if (auto * pBody = pLevel->tryGet<components::RigidBody>(entityId)) {
          internal::PhysicsRigidBody & rigidbody = entityBodies[entityId];
          btVector3                    inertia;
          pShape->calculateLocalInertia((btScalar)pBody->getMass(), inertia);
          rigidbody.body->setMassProps((btScalar)pBody->getMass(), inertia);
          rigidbody.body->setCollisionShape(pShape.get());
        } else {
          internal::PhysicsCollider & collider = entityColliders.getOrAdd(entityId);
          collider.obj->setCollisionShape(pShape.get());
        }
      }

      for (auto & entityId : erasedEntities) {
        if (entityBodies.contains(entityId)) {
          pWorld->removeRigidBody(entityBodies[entityId].body.get());
        }

        if (entityColliders.contains(entityId)) {
          pWorld->removeCollisionObject(entityColliders[entityId].obj.get());
        }

        entityBodies.erase(entityId);
        entityColliders.erase(entityId);
        entityShapes.erase(entityId);
      }

      // For stale transforms
      for (auto & [entityId, shape] : entityShapes) {
        auto &transform = pLevel->get<components::Transform>(entityId);
        shape->setLocalScaling(ToBt(transform.globalScale(pLevel)));
      }

      for (auto & [entityId, collider] : entityColliders) {
        auto & transform = pLevel->get<components::Transform>(entityId);
        collider.obj->setWorldTransform(ToBt(pLevel, transform));
      }

      for (auto & [entityId, body] : entityBodies) {
        auto & transform = pLevel->get<components::Transform>(entityId);

        if (body.sync->previousTransform != transform.globalTransform(pLevel)) {
          body.body->setWorldTransform(ToBt(pLevel, transform));
          body.body->activate();
        }
      }
    }
  };

  Physics::Physics(engine::AssetManager * pAssets)
    : m_pCube(pAssets, builtin_assets::mesh::cube)
    , m_pSphere(pAssets, builtin_assets::mesh::sphere)
  {}

  void Physics::created(Level * pLevel) {
    auto pData = pLevel->addData<LevelData>();
    pData->pCollisionConfiguration = bfc::NewRef<btDefaultCollisionConfiguration>();
    pData->pDispatcher             = bfc::NewRef<btCollisionDispatcher>(pData->pCollisionConfiguration.get());
    pData->pOverlappingPairCache   = bfc::NewRef<btDbvtBroadphase>();
    pData->pSolver                 = bfc::NewRef<btSequentialImpulseConstraintSolver>();

    pData->pWorld = bfc::NewRef<btDiscreteDynamicsWorld>(pData->pDispatcher.get(), pData->pOverlappingPairCache.get(),
                                                         pData->pSolver.get(), pData->pCollisionConfiguration.get());
  }

  void Physics::update(Level * pLevel, bfc::Timestamp dt) {
    auto pData = pLevel->getData<LevelData>();

    pData->synchronize(pLevel);

    pData->pWorld->stepSimulation((btScalar)dt.secs());
  }

  void Physics::activate(Level * pLevel) {
    auto pData  = pLevel->getData<LevelData>();

    pData->synchronize(pLevel);
  }

  void Physics::pause(Level * pLevel) {}

  void Physics::stop(Level * pLevel) {}

  void Physics::collectRenderData(RenderView * pRenderView, Level const * pLevel) {
    auto pRenderData = pRenderView->pRenderData;
    auto pSphere     = m_pSphere.instance();
    auto pCube       = m_pCube.instance();

    RenderableStorage<StaticMeshRenderable> & meshes = pRenderData->renderables<StaticMeshRenderable>();

    for (auto & [transform, sphere] : pLevel->getView<components::Transform, components::ColliderSphere>()) {
      StaticMeshRenderable mesh(transform.globalTransform(pLevel) * bfc::math::scale(sphere.getRadius() * 2), *pSphere, 0);
      mesh.primitiveType = bfc::PrimitiveType_Line;
      meshes.pushBack(mesh);
    }

    for (auto & [transform, cube] : pLevel->getView<components::Transform, components::ColliderCube>()) {
      StaticMeshRenderable mesh(transform.globalTransform(pLevel) * bfc::math::scale(cube.getSize()), *pCube, 0);
      mesh.primitiveType = bfc::PrimitiveType_Line;
      meshes.pushBack(mesh);
    }

    for (auto & [transform, capsule] : pLevel->getView<components::Transform, components::ColliderCapsule>()) {
      const bfc::Vec3d     offset = bfc::math::up<double> * (capsule.getHeight() / 2 - capsule.getRadius());
      StaticMeshRenderable top(transform.globalTransform(pLevel) * bfc::math::translation(offset) * bfc::math::scale(capsule.getRadius() * 2), *pSphere, 0);
      top.primitiveType = bfc::PrimitiveType_Line;
      meshes.pushBack(top);
      
      StaticMeshRenderable bottom(transform.globalTransform(pLevel) * bfc::math::translation(-offset) *
                                    bfc::math::scale(capsule.getRadius() * 2),
                                  *pSphere, 0);
      bottom.primitiveType = bfc::PrimitiveType_Line;
      meshes.pushBack(bottom);
    }
  }
} // namespace engine
