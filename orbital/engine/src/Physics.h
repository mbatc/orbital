#pragma once

#include "Levels/Level.h"
#include "Levels/LevelSystem.h"
#include "Assets/AssetManager.h"
#include "Levels/LevelSerializer.h"

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
    void                setMesh(bfc::Ref<bfc::Mesh> const & pMesh);

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
    Physics();

    virtual void created(Level * pLevel) override;
    virtual void update(Level * pLevel, bfc::Timestamp dt) override;
    virtual void activate(Level * pLevel) override;
    virtual void pause(Level * pLevel) override;
    virtual void stop(Level * pLevel) override;
    virtual void collectRenderData(RenderView * pRenderView, Level const * pLevel) override;

    struct RayCastHit {
      EntityID entity = EntityID::Invalid;

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
  struct Serialization<components::ColliderCapsule> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, components::ColliderCapsule const & o) {
      return SerializedObject::MakeMap({
        {"height", ar(o.getHeight())},
        {"radius", ar(o.getRadius())},
      });
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, components::ColliderCapsule & o) {
      double height, radius;
      ar(s.get("height"), height);
      ar(s.get("radius"), radius);

      mem::construct(&o, height, radius);

      return true;
    }
  };

  template<>
  struct Serialization<components::ColliderCube> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, components::ColliderCube const & o) {
      return SerializedObject::MakeMap({
        {"size", ar(o.getSize())},
      });
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, components::ColliderCube & o) {

      bfc::Vec3d size = { 1, 1, 1 };
      ar(s.get("size"), size);

      mem::construct(&o, size);

      return true;
    }
  };

  template<>
  struct Serialization<components::ColliderSphere> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, components::ColliderSphere const & o) {
      return SerializedObject::MakeMap({
        {"radius", ar(o.getRadius())},
      });
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, components::ColliderSphere & o) {

      double radius = 0.5;
      ar(s.get("radius"), radius);
      mem::construct(&o, radius);

      return true;
    }
  };

  template<>
  struct Serialization<components::ColliderMesh> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, components::ColliderMesh const & o) {
      return SerializedObject::MakeMap({{"mesh", ar(o.getMesh()) }});
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, components::ColliderMesh & o) {
      mem::construct(&o);
      bfc::Uninitialized<bfc::Ref<bfc::Mesh>> pMesh;
      if (!ar(s.get("mesh"), pMesh)) {
        mem::destruct(&o);
        return false;
      }
      o.setMesh(std::move(pMesh.take()));
      return true;
    }
  };

  template<>
  struct Serialization<components::RigidBody> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, components::RigidBody const & o) {
      return SerializedObject::MakeMap({
        {"mass", ar(o.getMass())},
      });
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, components::RigidBody & o) {
      mem::construct(&o);

      double mass = 1;
      ar(s.get("mass"), mass);

      o.setMass(mass);

      return true;
    }
  };
} // namespace bfc
