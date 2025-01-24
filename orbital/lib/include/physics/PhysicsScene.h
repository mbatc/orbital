#pragma once

#include "core/Memory.h"
#include "util/Delegate.h"
#include "math/MathTypes.h"


namespace bfc {
  class PhysicsScene;
  class Constraint {

  };

  class RigidBody {
  public:
    struct Impl;

    RigidBody(double mass);

    ~RigidBody();

    PhysicsScene * getScene() const;

    Mat4d getTransform() const;

    void setTransform(Mat4d const & transform);

    Impl * getImpl() const;

  private:
    Ref<Impl>      m_pImpl  = nullptr;
    PhysicsScene * m_pScene = nullptr;
  };

  class PhysicsScene {
  public:
    struct Impl;

    PhysicsScene();

    bool addRigidBody(Ref<RigidBody> const & body);
    bool removeRigidBody(Ref<RigidBody> const & body);

    Ref<Constraint> addConstraint(Ref<RigidBody> bodyA, Ref<RigidBody> bodyB);

    Impl * getImpl() const;

  private:
    Ref<Impl> m_pImpl = nullptr;
  };
}
