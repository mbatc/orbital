#include "CoreComponents.h"

namespace components {
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

  bfc::Vec3d Transform::parentTranslation(engine::Level const * pLevel) const {
    const Transform * pParentTransform = pLevel->tryGet<Transform>(m_parent);

    return pParentTransform != nullptr ? pParentTransform->globalTranslation(pLevel) : bfc::Vec3d(0);
  }

  bfc::Quatd Transform::parentOrientation(engine::Level const * pLevel) const {
    const Transform * pParentTransform = pLevel->tryGet<Transform>(m_parent);

    return pParentTransform != nullptr ? pParentTransform->globalOrientation(pLevel) : glm::identity<bfc::Quatd>();
  }

  bfc::Vec3d Transform::parentYpr(engine::Level const * pLevel) const {
    return bfc::Vec3d();
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

  void Transform::setGlobalTranslation(engine::Level const * pLevel, bfc::Vec3d const & translation) {

  }

  void Transform::setGlobalOrientation(engine::Level const * pLevel, bfc::Quatd const & orientation) {

  }

  void Transform::setGlobalYpr(engine::Level const * pLevel, bfc::Vec3d const & ypr) {
    setGlobalOrientation(pLevel, bfc::math::yprToQuat(ypr));
  }

  void Transform::setGlobalScale(engine::Level const * pLevel, bfc::Vec3d const & scale) {

  }

  void Transform::setGlobalTransform(engine::Level const * pLevel, bfc::Mat4d const & transform) {
  }

  void Transform::setParent(engine::Level const * pLevel, engine::EntityID const & entityID) const {

  }
} // namespace components
