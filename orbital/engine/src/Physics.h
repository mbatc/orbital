#pragma once

#include "Levels/Level.h"
#include "Levels/LevelSystem.h"
#include "Assets/AssetManager.h"

#include "geometry/Ray.h"

namespace bfc {
  class Mesh;
  class GraphicsDevice;
}

namespace components {
  struct ColliderCube {
  public:
    ColliderCube(bfc::Vec3d const & size = { 1, 1, 1 });

    bfc::Vec3d getSize() const;
    void       setSize(bfc::Vec3d const & size);

    bfc::Ref<void> getImpl() const;

  private:
    bfc::Vec3d m_size;
    bfc::Ref<void> m_pImpl;
  };

  class ColliderSphere {
  public:
    ColliderSphere(double radius = 0.5);

    double getRadius() const;
    void setRadius(double radius);

    bfc::Ref<void> getImpl() const;

  private:
    bfc::Ref<void> m_pImpl;
  };

  class ColliderCapsule {
  public:
    ColliderCapsule(double height = 1, double radius = 0.125);

    double getHeight() const;
    void   setHeight(double height);

    double getRadius() const;
    void   setRadius(double radius);

    void set(double height, double radius);

    bfc::Ref<void> getImpl() const;

  private:
    double         m_height = 0;
    double         m_radius = 0;
    bfc::Ref<void> m_pImpl = nullptr;
  };

  class ColliderMesh {
  public:
    bfc::Ref<void> getImpl() const;

    bfc::Ref<bfc::Mesh> getMesh() const;
    void                setMesh(bfc::GraphicsDevice * pGraphicsDevice, bfc::Ref<bfc::Mesh> const & pMesh);

  private:
    bfc::Ref<void>  m_pImpl;
  };

  class RigidBody {
  public:
    double getMass() const;
    void   setMass(double mass);

  private:
    double m_mass = 1;
  };
}

namespace engine {
  class Physics
    : public ILevelCreated
    , public ILevelUpdate
    , public ILevelActivate
    , public ILevelPause
    , public ILevelStop
    , public ILevelRenderDataCollector {
  public:
    Physics(AssetManager *pAssets);

    virtual void created(Level * pLevel) override;
    virtual void update(Level * pLevel, bfc::Timestamp dt) override;
    virtual void activate(Level * pLevel) override;
    virtual void pause(Level * pLevel) override;
    virtual void stop(Level * pLevel) override;
    virtual void collectRenderData(RenderView * pRenderView, Level const * pLevel) override;

    struct RayCastHit {
      EntityID entity = 0;

      bfc::Vec3d position = bfc::Vec3d(0);
      bfc::Vec3d normal   = bfc::Vec3d(0);
      double     fraction = 0;
    };

    static void rayTrace(Level * pLevel, bfc::geometry::Rayd const & ray, std::function<void(RayCastHit)> const & onHit);

    static std::optional<RayCastHit> rayTrace(Level * pLevel, bfc::geometry::Rayd const & ray);

  private:
    struct LevelData;

    Asset<bfc::Mesh> m_pCube;
    Asset<bfc::Mesh> m_pSphere;
  };
} // namespace engine

namespace bfc {
  template<>
  struct Serializer<components::ColliderCapsule> {
    template<typename Context>
    static SerializedObject write(components::ColliderCapsule const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"height", serialize(o.getHeight())},
        {"radius", serialize(o.getRadius())},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::ColliderCapsule & o, Context const &) {
      double height, radius;
      s.get("height").read(height);
      s.get("radius").read(radius);

      mem::construct(&o, height, radius);

      return true;
    }
  };

  template<>
  struct Serializer<components::ColliderCube> {
    template<typename Context>
    static SerializedObject write(components::ColliderCube const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"size", serialize(o.getSize())},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::ColliderCube & o, Context const &) {

      bfc::Vec3d size = { 1, 1, 1 };
      s.get("size").read(size);

      mem::construct(&o, size);

      return true;
    }
  };

  template<>
  struct Serializer<components::ColliderSphere> {
    template<typename Context>
    static SerializedObject write(components::ColliderSphere const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"radius", serialize(o.getRadius())},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::ColliderSphere & o, Context const &) {

      double radius = 0.5;
      s.get("radius").read(radius);
      mem::construct(&o, radius);

      return true;
    }
  };

  template<>
  struct Serializer<components::ColliderMesh> {
    template<typename Context>
    static SerializedObject write(components::ColliderMesh const & o, Context const &) {
      return SerializedObject::MakeMap({
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::ColliderMesh & o, Context const &) {
      mem::construct(&o);

      return true;
    }
  };

  template<>
  struct Serializer<components::RigidBody> {
    template<typename Context>
    static SerializedObject write(components::RigidBody const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"mass", serialize(o.getMass())},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::RigidBody & o, Context const &) {
      mem::construct(&o);

      double mass = 1;
      s.get("mass").read(mass);

      o.setMass(mass);

      return true;
    }
  };
} // namespace bfc
