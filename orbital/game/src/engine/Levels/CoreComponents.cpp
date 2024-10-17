#include "CoreComponents.h"
#include "util/Log.h"

namespace components {
  bfc::Vec3d Transform::applyToPoint(bfc::Mat4d const & transform, bfc::Vec3d const & point) {
    return (transform * bfc::Vec4d(point, 1));
  }

  bfc::Vec3d Transform::applyToDirection(bfc::Mat4d const & transform, bfc::Vec3d const & direction) {
    return (transform * bfc::Vec4d(direction, 0));
  }

  bfc::Vec3d Transform::translation() const {
    return m_translation;
  }

  bfc::Quatd Transform::orientation() const {
    return m_orientation;
  }

  bfc::Vec3d Transform::ypr() const {
    return bfc::math::quatToYpr(orientation());
  }

  bfc::Vec3d Transform::scale() const {
    return m_scale;
  }

  bfc::Mat4d Transform::transform() const {
    return glm::translate(m_translation) * glm::toMat4(m_orientation) * glm::scale(m_scale);
  }

  bfc::Mat4d Transform::transformInverse() const {
    return glm::inverse(transform());
  }

  bfc::Vec3d Transform::forward() const {
    return applyToDirection(transform(), bfc::math::forward<double>);
  }

  bfc::Vec3d Transform::right() const {
    return applyToDirection(transform(), bfc::math::right<double>);
  }

  bfc::Vec3d Transform::up() const {
    return applyToDirection(transform(), bfc::math::up<double>);
  }

  bfc::Vec3d Transform::globalTranslation(engine::Level const * pLevel) const {
    Transform t;
    t.setTransform(globalTransform(pLevel));
    return t.translation();
  }

  bfc::Quatd Transform::globalOrientation(engine::Level const * pLevel) const {
    Transform t;
    t.setTransform(globalTransform(pLevel));
    return t.orientation();
  }

  bfc::Vec3d Transform::globalYpr(engine::Level const * pLevel) const {
    return bfc::math::quatToYpr(globalOrientation(pLevel));
  }

  bfc::Vec3d Transform::globalScale(engine::Level const * pLevel) const {
    Transform t;
    t.setTransform(globalTransform(pLevel));
    return t.scale();
  }

  bfc::Mat4d Transform::globalTransform(engine::Level const * pLevel) const {
    return parentTransform(pLevel) * transform();
  }

  bfc::Mat4d Transform::globalTransformInverse(engine::Level const * pLevel) const {
    return glm::inverse(globalTransform(pLevel));
  }

  bfc::Vec3d Transform::globalForward(engine::Level const * pLevel) const {
    return applyToDirection(globalTransform(pLevel), bfc::math::forward<double>);
  }

  bfc::Vec3d Transform::globalRight(engine::Level const * pLevel) const {
    return applyToDirection(globalTransform(pLevel), bfc::math::right<double>);
  }

  bfc::Vec3d Transform::globalUp(engine::Level const * pLevel) const {
    return applyToDirection(globalTransform(pLevel), bfc::math::up<double>);
  }

  bfc::Vec3d Transform::parentTranslation(engine::Level const * pLevel) const {
    const Transform * pParentTransform = pLevel->tryGet<Transform>(m_parent);

    return pParentTransform != nullptr ? pParentTransform->globalTranslation(pLevel) : bfc::Vec3d(0);
  }

  bfc::Quatd Transform::parentOrientation(engine::Level const * pLevel) const {
    const Transform * pParentTransform = pLevel->tryGet<Transform>(m_parent);

    return pParentTransform != nullptr ? pParentTransform->globalOrientation(pLevel) : glm::identity<bfc::Quatd>();
  }

  bfc::Vec3d Transform::parentYpr(engine::Level const * pLevel) const {
    const Transform * pParentTransform = pLevel->tryGet<Transform>(m_parent);

    return pParentTransform != nullptr ? pParentTransform->globalYpr(pLevel) : bfc::Vec3d(0);
  }

  bfc::Vec3d Transform::parentScale(engine::Level const * pLevel) const {
    const Transform * pParentTransform = pLevel->tryGet<Transform>(m_parent);

    return pParentTransform != nullptr ? pParentTransform->globalScale(pLevel) : bfc::Vec3d(1);
  }

  bfc::Mat4d Transform::parentTransform(engine::Level const * pLevel) const {
    const Transform *pParentTransform = pLevel->tryGet<Transform>(m_parent);
    if (pParentTransform == nullptr) {
      return glm::identity<bfc::Mat4d>();
    }

    return pParentTransform->globalTransform(pLevel);
  }

  engine::EntityID Transform::parent() const {
    return m_parent;
  }

  void Transform::setTranslation(bfc::Vec3d const & translation) {
    m_translation = translation;
  }

  void Transform::setOrientation(bfc::Quatd const & orientation) {
    m_orientation = orientation;
  }

  void Transform::setYpr(bfc::Vec3d const & ypr) {
    setOrientation(bfc::math::yprToQuat(ypr));
  }

  void Transform::setScale(bfc::Vec3d const & scale) {
    m_scale = scale;
  }

  void Transform::setTransform(bfc::Mat4d const & transform) {
    bfc::Vec3d skew;
    bfc::Vec4d perspective;

    glm::decompose(transform, m_scale, m_orientation, m_translation, skew, perspective);
  }

  void Transform::lookAt(bfc::Vec3d const & direction, bfc::Vec3d const & up) {
    setOrientation(glm::quatLookAt(direction, up));
  }

  void Transform::setGlobalTranslation(engine::Level const * pLevel, bfc::Vec3d const & translation) {
    Transform t;
    t.setTransform(globalTransform(pLevel));
    t.setTranslation(translation);
    setGlobalTransform(pLevel, t.transform());
  }

  void Transform::setGlobalOrientation(engine::Level const * pLevel, bfc::Quatd const & orientation) {
    Transform t;
    t.setTransform(globalTransform(pLevel));
    t.setOrientation(orientation);
    setGlobalTransform(pLevel, t.transform());
  }

  void Transform::setGlobalYpr(engine::Level const * pLevel, bfc::Vec3d const & ypr) {
    setGlobalOrientation(pLevel, bfc::math::yprToQuat(ypr));
  }

  void Transform::setGlobalScale(engine::Level const * pLevel, bfc::Vec3d const & scale) {
    Transform t;
    t.setTransform(globalTransform(pLevel));
    t.setScale(scale);
    setGlobalTransform(pLevel, t.transform());
  }

  void Transform::setGlobalTransform(engine::Level const * pLevel, bfc::Mat4d const & transform) {
    Transform const * pParentTransform = pLevel->tryGet<Transform>(parent());

    setTransform(pParentTransform->globalTransformInverse(pLevel) * transform);
  }

  void Transform::setGlobalLookAt(engine::Level const * pLevel, bfc::Vec3d const & direction, bfc::Vec3d const & up) {
    setGlobalOrientation(pLevel, glm::quatLookAt(direction, up));
  }

  void Transform::setParent(engine::Level * pLevel, engine::EntityID const & entityID) {
    engine::EntityID self = pLevel->toEntity(this);
    if (self == engine::InvalidEntity) {
      BFC_LOG_WARNING("Transform", "Component level instance mismatched in setParent()");
      return;
    }

    Transform * pOldParent = pLevel->tryGet<Transform>(m_parent);
    Transform * pNewParent = pLevel->tryGet<Transform>(entityID);

    if (pOldParent == pNewParent) {
      return;
    }

    if (pOldParent != nullptr) {
      pOldParent->m_children.eraseValue(self);
      m_parent = engine::InvalidEntity;
    }

    if (pNewParent != nullptr) {
      pNewParent->m_children.pushBack(self);
      m_parent = entityID;
    }
  }
  bfc::Mat4 Camera::projectionMat(float aspect) const {
    return glm::perspective(fov, aspect, nearPlane, farPlane);
  }
} // namespace components
