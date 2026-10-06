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

  engine::EntityID getAssignedEntityID(btCollisionObject const * obj){
    return engine::Level::toEntityID((uint32_t)obj->getUserIndex(), (uint32_t)obj->getUserIndex2());
  }

  void assignEntityID(btCollisionObject * obj, engine::EntityID const & id) {
    obj->setUserIndex(engine::Level::indexOf(id));
    obj->setUserIndex2(engine::Level::versionOf(id));
  }

  PHY_ScalarType ToScalarType(bfc::DataType dt) {
    switch (dt) {
    case bfc::DataType_Float32: return PHY_FLOAT;
    case bfc::DataType_Float64: return PHY_DOUBLE;
    case bfc::DataType_Int32:   return PHY_INTEGER;
    case bfc::DataType_UInt32:  return PHY_INTEGER;
    case bfc::DataType_Int16:   return PHY_SHORT;
    case bfc::DataType_UInt16:  return PHY_SHORT;
    case bfc::DataType_UInt8:   return PHY_UCHAR;
    }
    return PHY_UCHAR;
  }

  class BulletMeshInterface : public btStridingMeshInterface {
  public:
    BulletMeshInterface() = default;
    BulletMeshInterface(bfc::GraphicsDevice * pDevice, bfc::Ref<bfc::Mesh> pMesh, int64_t subMesh)
      : m_pDevice(pDevice)
      , m_mesh(pMesh)
      , m_subMesh(subMesh) {}

    virtual void getLockedVertexIndexBase(unsigned char ** vertexbase, int & numverts, PHY_ScalarType & type, int & stride,
                                          unsigned char ** indexbase, int & indexstride, int & numfaces,
                                          PHY_ScalarType & indicestype, int subpart = 0) {
      return lock(bfc::MapAccess_ReadWrite, (void **)vertexbase, numverts, type, stride, (void **)indexbase, indexstride,
                  numfaces, indicestype, subpart);
    }

    virtual void getLockedReadOnlyVertexIndexBase(const unsigned char ** vertexbase, int & numverts, PHY_ScalarType & type,
                                                  int & stride, const unsigned char ** indexbase, int & indexstride,
                                                  int & numfaces, PHY_ScalarType & indicestype, int subpart = 0) const {
      return lock(bfc::MapAccess_Read, (void **)vertexbase, numverts, type, stride, (void **)indexbase, indexstride,
                  numfaces, indicestype, subpart);
    }

    void lock(bfc::MapAccess access, void ** vertexbase, int & numverts, PHY_ScalarType & type, int & stride,
              void ** indexbase, int & indexstride, int & numfaces, PHY_ScalarType & indicestype, int subpart = 0) const {
      auto pCmdList = m_pDevice->createCommandList();

      auto pVA    = m_mesh->getVertexArray();
      auto layout = pVA->getLayout();

      std::future<void *> mappedVertexBuffer;
      std::future<void *> mappedIndexBuffer;

      int64_t positionOffset = 0;
      for (int64_t i = 0; i < layout.getAttributeCount(); ++i) {
        if (!layout.getAttributeSemantic(i).equals("POSITION0", true))
          continue;
        auto & attribute = layout.getAttributeLayout(i);
        pVA->getVertexBuffer(attribute.slot);

        type           = ToScalarType(attribute.dataType);
        stride         = (int)attribute.stride;
        positionOffset = attribute.offset;

        m_pLockedVertexBuffer = pVA->getVertexBuffer(attribute.slot);
        mappedVertexBuffer    = pCmdList->map(m_pLockedVertexBuffer, access);
        break;
      }

      auto &  sm              = m_mesh->getSubMesh(m_subMesh);
      int64_t indexSize       = bfc::getDataTypeSize(pVA->getIndexType());
      int64_t indexBufferSize = indexSize * sm.elmCount;

      m_pLockedIndexBuffer = pVA->getIndexBuffer();
      mappedIndexBuffer    = pCmdList->map(m_pLockedIndexBuffer, sm.elmOffset * indexSize, indexBufferSize, access);

      m_pDevice->submit(std::move(pCmdList));

      numverts    = (int)sm.elmCount;
      numfaces    = numverts / 3;
      indicestype = ToScalarType(pVA->getIndexType());
      indexstride = (int)indexSize;

      *vertexbase = (unsigned char *)mappedVertexBuffer.get() + positionOffset;
      *indexbase  = (unsigned char *)mappedIndexBuffer.get();
    }

    virtual void unLockVertexBase(int subpart) {
      auto pCmdList = m_pDevice->createCommandList();
      pCmdList->unmap(m_pLockedIndexBuffer);
      pCmdList->unmap(m_pLockedVertexBuffer);
      m_pDevice->submit(std::move(pCmdList));
      m_pLockedVertexBuffer = nullptr;
      m_pLockedIndexBuffer  = nullptr;
    }

    virtual void unLockReadOnlyVertexBase(int subpart) const {
      auto pCmdList = m_pDevice->createCommandList();
      pCmdList->unmap(m_pLockedIndexBuffer);
      pCmdList->unmap(m_pLockedVertexBuffer);
      m_pDevice->submit(std::move(pCmdList));
      m_pLockedVertexBuffer = nullptr;
      m_pLockedIndexBuffer  = nullptr;
    }

    virtual int getNumSubParts() const {
      return 1;
    }

    virtual void preallocateVertices(int numverts) {
      numverts;
    }

    virtual void preallocateIndices(int numindices) {
      numindices;
    }

    bfc::GraphicsDevice * m_pDevice = nullptr;
    bfc::Ref<bfc::Mesh>   m_mesh;
    int64_t               m_subMesh = 0;

    mutable bfc::graphics::BufferRef m_pLockedVertexBuffer;
    mutable bfc::graphics::BufferRef m_pLockedIndexBuffer;
  };

  class CompoundMeshShape : public btCompoundShape {
  public:
    CompoundMeshShape(bfc::GraphicsDevice * pGraphicsDevice, bfc::Ref<bfc::Mesh> pMesh)
      : btCompoundShape(true, (int)pMesh->getSubmeshCount()) {
      const int64_t numSubMeshes = pMesh->getSubmeshCount();
      m_meshes.reserve(numSubMeshes);
      m_bvhs.reserve(numSubMeshes);

      btTransform identity;
      identity.setIdentity();
      for (int64_t i = 0; i < numSubMeshes; ++i) {
        m_meshes.pushBack(BulletMeshInterface(pGraphicsDevice, pMesh, i));
        m_bvhs.pushBack(bfc::NewRef<btBvhTriangleMeshShape>(&m_meshes.back(), true));

        addChildShape(identity, m_bvhs.back().get());
      }
    }

    bfc::Ref<bfc::Mesh> getMesh() const {
      return m_pSourceMesh;
    }

  private:
    bfc::Ref<bfc::Mesh>                 m_pSourceMesh;
    bfc::Vector<BulletMeshInterface>    m_meshes;
    bfc::Vector<bfc::Ref<btBvhTriangleMeshShape>> m_bvhs;
  };

  // TODO: Implement trait to enable static addresses for level components so we can use btCollisionShape as the component
  struct PhysicsShape {
    bfc::Ref<btCollisionShape> shape;
  };

  // TODO: Implement trait to enable static addresses for level components so we can use btCollisionObject as the component
  struct PhysicsCollider {
    bfc::Ref<btCollisionShape>  shape; ///< Shape used by the collider
    bfc::Ref<btCollisionObject> obj;
  };

  struct PhysicsMotionSync : public btMotionState {
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
    bfc::Ref<btCollisionShape>  shape; ///< Shape used by the rigidbody
    bfc::Ref<btRigidBody>       body;
    bfc::Ref<PhysicsMotionSync> sync;
  };
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

  bfc::Ref<void> ColliderMesh::getImpl() const {
    return m_pImpl;
  }

  bfc::Ref<bfc::Mesh> ColliderMesh::getMesh() const {
    auto pShape = std::static_pointer_cast<CompoundMeshShape>(m_pImpl);
    if (pShape == nullptr)
      return nullptr;
    else
      return pShape->getMesh();
  }

  void ColliderMesh::setMesh(bfc::GraphicsDevice *pGraphicsDevice, bfc::Ref<bfc::Mesh> const & pMesh) {
    if (getMesh() == pMesh)
      return;

    m_pImpl = bfc::NewRef<CompoundMeshShape>(pGraphicsDevice, pMesh);
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
  struct Physics::LevelData {
    bfc::Map<EntityID, bfc::Ref<btCollisionShape>> entityShapes;
    bfc::Map<EntityID, PhysicsCollider>  entityColliders;
    bfc::Map<EntityID, PhysicsRigidBody> entityBodies;
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

      for (auto & [mesh] : pLevel->getView<components::ColliderMesh>()) {
        auto entityId = pLevel->toEntity(&mesh);

        bfc::Ref<btCollisionShape> pExistingShape;
        bfc::Ref<btCollisionShape> pNewShape = std::static_pointer_cast<CompoundMeshShape>(mesh.getImpl());

        if (!entityShapes.tryGet(entityId, &pExistingShape) || pExistingShape != pNewShape) {
          entityShapes.addOrSet(entityId, pNewShape);
          staleShapes.pushBack(entityId);
        }
      }

      bfc::Vector<EntityID> erasedEntities;
      for (auto & [entityId, pShape] : entityShapes) {
        if (!pLevel->contains(entityId) || pShape == nullptr) {
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
          PhysicsRigidBody &rigidbody = entityBodies.getOrAdd(entityId);
          if (rigidbody.body == nullptr) {
            rigidbody.shape        = pShape;
            rigidbody.sync         = bfc::NewRef<PhysicsMotionSync>();
            rigidbody.sync->pLevel = pLevel;
            rigidbody.sync->entity = entityId;

            btRigidBody::btRigidBodyConstructionInfo params((btScalar)pBody->getMass(), rigidbody.sync.get(), pShape.get());
            pShape->calculateLocalInertia((btScalar)pBody->getMass(), params.m_localInertia);

            rigidbody.body = bfc::NewRef<btRigidBody>(params);
            rigidbody.body->setWorldTransform(ToBt(pLevel, *pTransform));

            assignEntityID(rigidbody.body.get(), entityId);
            pWorld->addRigidBody(rigidbody.body.get());
          }
        } else {
          PhysicsCollider & collider = entityColliders.getOrAdd(entityId);
          if (collider.obj == nullptr) {
            collider.obj = bfc::NewRef<btCollisionObject>();
            collider.obj->setCollisionShape(pShape.get());
            collider.obj->setWorldTransform(ToBt(pLevel, *pTransform));
            collider.shape = pShape;

            pWorld->addCollisionObject(collider.obj.get());
            assignEntityID(collider.obj.get(), entityId);
            entityColliders.addOrSet(entityId, collider);
          }
        }
      }

      for (auto & entityId : staleShapes) {
        auto pShape = entityShapes[entityId];

        if (pShape != nullptr) {
          if (auto * pBody = pLevel->tryGet<components::RigidBody>(entityId)) {
            PhysicsRigidBody & rigidbody = entityBodies[entityId];
            btVector3                    inertia;
            pShape->calculateLocalInertia((btScalar)pBody->getMass(), inertia);
            rigidbody.body->setMassProps((btScalar)pBody->getMass(), inertia);
            rigidbody.body->setCollisionShape(pShape.get());
          } else {
            PhysicsCollider & collider = entityColliders.getOrAdd(entityId);
            collider.obj->setCollisionShape(pShape.get());
          }
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

  void Physics::rayTrace(Level * pLevel, bfc::geometry::Rayd const & ray, std::function<void(RayCastHit)> const & onHit) {
    struct CustomRayResultCallback : public btCollisionWorld::RayResultCallback {
      CustomRayResultCallback(bfc::geometry::Rayd const & ray, std::function<void(Physics::RayCastHit)> const & onHit)
        : m_ray(ray)
        , m_onHit(onHit) {}

      std::function<void(Physics::RayCastHit)> m_onHit;

      bfc::geometry::Rayd m_ray;

      virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult & rayResult, bool normalInWorldSpace) {
        m_collisionObject = rayResult.m_collisionObject;

        btVector3 hitNormalWorld = {};
        hitNormalWorld.setZero();
        if (normalInWorldSpace) {
          hitNormalWorld = rayResult.m_hitNormalLocal;
        } else {
          /// need to transform normal into worldspace
          hitNormalWorld = m_collisionObject->getWorldTransform().getBasis() * rayResult.m_hitNormalLocal;
        }

        Physics::RayCastHit hit = {};
        hit.entity   = getAssignedEntityID(m_collisionObject);
        hit.normal   = FromBt(hitNormalWorld);
        hit.position = m_ray.at(rayResult.m_hitFraction);
        hit.fraction = rayResult.m_hitFraction;

        m_onHit(hit);

        return m_closestHitFraction;
      }
    };

    auto pData = pLevel->getData<LevelData>();

    pData->pWorld->rayTest(ToBt(ray.start), ToBt(ray.end()), CustomRayResultCallback(ray, onHit));
  }

  std::optional<Physics::RayCastHit> Physics::rayTrace(Level * pLevel, bfc::geometry::Rayd const & ray) {
    btCollisionWorld::ClosestRayResultCallback closest(ToBt(ray.start), ToBt(ray.end()));

    auto pData = pLevel->getData<LevelData>();

    pData->pWorld->rayTest(ToBt(ray.start), ToBt(ray.end()), closest);

    if (!closest.hasHit()) {
      return std::nullopt;
    }

    Physics::RayCastHit hit;
    hit.normal   = FromBt(closest.m_hitNormalWorld);
    hit.position = FromBt(closest.m_hitPointWorld);
    hit.fraction = closest.m_closestHitFraction;
    hit.entity   = getAssignedEntityID(closest.m_collisionObject);
    return hit;
  }
} // namespace engine
