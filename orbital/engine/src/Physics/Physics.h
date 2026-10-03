#pragma once

#include "Levels/Level.h"
#include "Levels/LevelSystem.h"
#include "Assets/AssetManager.h"

#include "geometry/Box.h"
#include "geometry/Sphere.h"

namespace bfc {
  class Mesh;
}

namespace components {
  struct ColliderCube {
    bfc::Vec3d size = bfc::Vec3d(0.5);
  };

  struct ColliderSphere {
    double radius = 0.5;
  };

  struct ColliderCapsule {
    double height = 1;
    double radius = 0.125;
  };

  struct ColliderMesh {
  };

  struct RigidBody {
    double mass = 1;
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

    // static void rayTrace(Level * pLevel);

  private:
    Asset<bfc::Mesh> m_pCube;
    Asset<bfc::Mesh> m_pSphere;
  };
} // namespace engine

namespace bfc {
  template<>
  struct Reflect<components::ColliderCapsule> {
    static inline constexpr auto get() {
      return makeReflection<components::ColliderCapsule>(
        BFC_REFLECT(components::ColliderCapsule, height),
        BFC_REFLECT(components::ColliderCapsule, radius)
      );
    }
  };

  template<>
  struct Serializer<components::ColliderCapsule> {
    template<typename Context>
    static SerializedObject write(components::ColliderCapsule const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"height", serialize(o.height)},
        {"radius", serialize(o.radius)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::ColliderCapsule & o, Context const &) {
      mem::construct(&o);

      s.get("height").read(o.height);
      s.get("radius").read(o.radius);

      return true;
    }
  };
  
  template<>
  struct Reflect<components::ColliderCube> {
    static inline constexpr auto get() {
      return makeReflection<components::ColliderCube>(BFC_REFLECT(components::ColliderCube, size));
    }
  };

  template<>
  struct Serializer<components::ColliderCube> {
    template<typename Context>
    static SerializedObject write(components::ColliderCube const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"size", serialize(o.size)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::ColliderCube & o, Context const &) {
      mem::construct(&o);

      s.get("size").read(o.size);

      return true;
    }
  };

  template<>
  struct Reflect<components::ColliderSphere> {
    static inline constexpr auto get() {
      return makeReflection<components::ColliderSphere>(BFC_REFLECT(components::ColliderSphere, radius));
    }
  };

  template<>
  struct Serializer<components::ColliderSphere> {
    template<typename Context>
    static SerializedObject write(components::ColliderSphere const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"radius", serialize(o.radius)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::ColliderSphere & o, Context const &) {
      mem::construct(&o);

      s.get("radius").read(o.radius);

      return true;
    }
  };
  

  // template<>
  // struct Reflect<components::ColliderMesh> {
  //   static inline constexpr auto get() {
  //     return makeReflection<components::ColliderMesh>(BFC_REFLECT(components::ColliderMesh, mass)
  //     );
  //   }
  // };

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
  struct Reflect<components::RigidBody> {
    static inline constexpr auto get() {
      return makeReflection<components::RigidBody>(
        BFC_REFLECT(components::RigidBody, mass)
      );
    }
  };

  template<>
  struct Serializer<components::RigidBody> {
    template<typename Context>
    static SerializedObject write(components::RigidBody const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"mass", serialize(o.mass)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::RigidBody & o, Context const &) {
      mem::construct(&o);

      s.get("mass").read(o.mass);

      return true;
    }
  };
} // namespace bfc
