#include "math/MathTypes.h"
#include "btBulletDynamicsCommon.h"
#include "Levels/Level.h"

namespace bfc {
  class MeshData;
}

namespace components {
  struct RigidBody {
    double mass = 1;

    bfc::Ref<int64_t> instanceHandle = nullptr;
  };

  struct Collider {
    bfc::Ref<btCollisionShape> pShape;

    bfc::Ref<int64_t> instanceHandle = nullptr;
  };

  // Generate a collider based on mesh data
  struct Collider_Mesh {
    bfc::Ref<bfc::MeshData> pMesh; // TODO: Need to add a mesh collider asset type.
  };

  // Generate a collider based on cube geometry parameters
  struct Collider_Cube {
    bfc::Vec3d size   = { 1, 1, 1 };
  };

  // Generate a collider based on sphere geometry parameters
  struct Collider_Sphere {
    double     radius = 0.5;
  };

  // Generate a collider based on capsule geometry parameters
  struct Collider_Capsule {
    double radius = 0.25;
    double length = 0.5;
  };
}

namespace engine {
  template<>
  struct engine::LevelComponentSerializer<components::Collider> {
    inline static bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, components::Collider const & o) {
      return bfc::SerializedObject::MakeMap({{"shape", pSerializer->writeAsset(o.pShape) }});
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & s, Level & level, EntityID entity, components::Collider & o) {
      bfc::mem::construct(&o.instanceHandle);

      pSerializer->readAsset(s.get("shape"), o.pShape);
      return true;
    }
  };

  template<>
  struct engine::LevelComponentSerializer<components::Collider_Mesh> {
    inline static bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, components::Collider_Mesh const & o) {
      return bfc::SerializedObject::MakeMap({{"mesh", pSerializer->writeAsset(o.pMesh)}});
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & s, Level & level, EntityID entity, components::Collider_Mesh & o) {
      pSerializer->readAsset(s.get("mesh"), o.pMesh);
      return true;
    }
  };
}

namespace bfc {
  template<>
  struct Serializer<components::RigidBody> {
    inline static SerializedObject write(components::RigidBody const & o) {
      return SerializedObject::MakeMap({{"mass", serialize(o.mass)}});
    }

    inline static bool read(SerializedObject const & s, components::RigidBody & o) {
      mem::construct(&o);

      s.get("mass").read(o.mass);
      return true;
    }
  };

  template<>
  struct Serializer<components::Collider_Cube> {
    inline static SerializedObject write(components::Collider_Cube const & o) {
      return SerializedObject::MakeMap({{"size", bfc::serialize(o.size)}});
    }

    inline static bool read(SerializedObject const & s, components::Collider_Cube & o) {
      s.get("size").read(o.size);
      return true;
    }
  };

  template<>
  struct Serializer<components::Collider_Sphere> {
    inline static SerializedObject write(components::Collider_Sphere const & o) {
      return SerializedObject::MakeMap({{"radius", bfc::serialize(o.radius)}});
    }

    inline static bool read(SerializedObject const & s, components::Collider_Sphere & o) {
      s.get("radius").read(o.radius);
      return true;
    }
  };

  template<>
  struct Serializer<components::Collider_Capsule> {
    inline static SerializedObject write(components::Collider_Capsule const & o) {
      return SerializedObject::MakeMap({{"length", bfc::serialize(o.length)}, {"radius", bfc::serialize(o.radius)}});
    }

    inline static bool read(SerializedObject const & s, components::Collider_Capsule & o) {
      s.get("length").read(o.length);
      s.get("radius").read(o.radius);
      return true;
    }
  };
} // namespace bfc
