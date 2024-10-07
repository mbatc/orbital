#include "mesh/MeshSkeleton.h"
#include "core/Stream.h"

#define VERIFY_POSE(poseID) BFC_ASSERT(m_poses.isUsed(poseID), "%lld is not a valid pose ID", poseID);
#define VERIFY_BONE(boneID) BFC_ASSERT(m_bones.isUsed(boneID), "%lld is not a valid bone ID", boneID);
#define VERIFY_POSE_AND_BONE(poseID, boneID) VERIFY_POSE(poseID); VERIFY_BONE(boneID);

namespace bfc {
  MeshSkeleton::MeshSkeleton() {
    m_bones.emplace();
  }

  Vector<int64_t> MeshSkeleton::getBones() const {
    Vector<int64_t> ret;
    ret.reserve(m_bones.size());
    for (Bone const & bone : m_bones)
      ret.pushBack(bone.id);
    return ret;
  }

  int64_t MeshSkeleton::addBone(StringView const & name, int64_t parent) {
    BFC_ASSERT(m_bones.isUsed(parent), "%lld is not a valid bone", parent);

    int64_t id = m_bones.emplace();

    Bone & bone = m_bones[id];
    bone.name   = name;
    bone.parent = parent;
    bone.id     = id;

    return id;
  }

  bool MeshSkeleton::removeBone(int64_t boneID) {
    BFC_ASSERT(boneID != 0, "You cannot remove bone 0 as this is the root bone.");
    if (!m_bones.isUsed(boneID))
      return false;

    Bone const & bone = m_bones[boneID];
    for (Bone & child : m_bones) {
      if (child.parent == boneID) {
        child.parent = bone.parent;
      }
    }

    m_bones.erase(boneID);

    return true;
  }

  int64_t MeshSkeleton::getParent(int64_t boneID) const {
    VERIFY_BONE(boneID);

    return m_bones[boneID].parent;
  }

  StringView MeshSkeleton::getBoneName(int64_t boneID) const {
    VERIFY_BONE(boneID);

    return m_bones[boneID].name;
  }

  void MeshSkeleton::setBoneName(int64_t boneID, StringView const & name) {
    VERIFY_BONE(boneID);

    m_bones[boneID].name = name;
  }

  void MeshSkeleton::setBoneParent(int64_t boneID, int64_t parent) {
    BFC_ASSERT(boneID != 0, "You cannot set the parent of bone 0 as this is the root bone.");

    VERIFY_BONE(boneID);
    VERIFY_BONE(parent);

    m_bones[boneID].parent = parent;
  }

  int64_t MeshSkeleton::addPose(StringView const & name) {
    int64_t id = m_poses.emplace();

    Pose & newPose = m_poses[id];

    newPose.id   = id;
    newPose.name = name;
    newPose.transforms.resize(m_bones.capacity());

    return id;
  }

  bool MeshSkeleton::removePose(int64_t poseID) {
    return m_bones.erase(poseID);
  }

  StringView MeshSkeleton::getPoseName(int64_t poseID) const {
    VERIFY_POSE(poseID);

    return m_poses[poseID].name;
  }

  void MeshSkeleton::setPoseName(int64_t poseID, StringView const & name) {
    VERIFY_POSE(poseID);

     m_poses[poseID].name = name;
  }

  void MeshSkeleton::setBonePosition(int64_t poseID, int64_t boneID, Vec3d const & position) {
    VERIFY_POSE_AND_BONE(poseID, boneID);

    Pose & pose = m_poses[poseID];
    pose.transforms.resize(m_bones.capacity());
    pose.transforms[boneID].position = position;
  }

  void MeshSkeleton::setBoneScale(int64_t poseID, int64_t boneID, Vec3d const & scale) {
    VERIFY_POSE_AND_BONE(poseID, boneID);

    Pose & pose = m_poses[poseID];
    pose.transforms.resize(m_bones.capacity());
    pose.transforms[boneID].scale = scale;
  }

  void MeshSkeleton::setBoneOrientation(int64_t poseID, int64_t boneID, Quatd const & ori) {
    VERIFY_POSE_AND_BONE(poseID, boneID);

    Pose & pose = m_poses[poseID];
    pose.transforms.resize(m_bones.capacity());
    pose.transforms[boneID].orientation = ori;
  }

  void MeshSkeleton::setBoneLocalTransform(int64_t poseID, int64_t boneID, Mat4d const & transform) {
    VERIFY_POSE_AND_BONE(poseID, boneID);

    Pose & pose = m_poses[poseID];
    pose.transforms.resize(m_bones.capacity());
    Transform & t = pose.transforms[boneID];

    Vec3d skew;
    Vec4d perspective;
    glm::decompose(transform, t.scale, t.orientation, t.position, skew, perspective);
  }

  void MeshSkeleton::setBoneGlobalTransform(int64_t poseID, int64_t boneID, Mat4d const & transform) {
    VERIFY_POSE_AND_BONE(poseID, boneID);

    Pose & pose = m_poses[poseID];
    pose.transforms.resize(m_bones.capacity());
    Transform & t = pose.transforms[boneID];

    Vec3d skew;
    Vec4d perspective;
    glm::decompose(transform, t.scale, t.orientation, t.position, skew, perspective);
  }

  Mat4d MeshSkeleton::evaluateLocalTransform(int64_t poseID, int64_t boneID) const {
    VERIFY_POSE_AND_BONE(poseID, boneID);

    Pose const & pose = m_poses[poseID];
    if (boneID >= pose.transforms.size()) {
      return glm::identity<Mat4d>();
    }

    Transform const & transform = pose.transforms[boneID];
    return glm::translate(transform.position) * glm::toMat4(transform.orientation) * glm::scale(transform.scale);
  }

  Mat4d MeshSkeleton::evaluateGlobalTransform(int64_t poseID, int64_t boneID) const {
    VERIFY_POSE_AND_BONE(poseID, boneID);

    Pose const & pose = m_poses[poseID];
    Bone const & bone = m_bones[poseID];

    if (bone.parent != -1) {
      return evaluateGlobalTransform(poseID, bone.parent) * evaluateLocalTransform(poseID, boneID);
    } else {
      return evaluateLocalTransform(poseID, boneID);
    }
  }

  Mat4d MeshSkeleton::evaluateParentTransform(int64_t poseID, int64_t boneID) const {
    VERIFY_POSE_AND_BONE(poseID, boneID);

    Pose const & pose = m_poses[poseID];
    Bone const & bone = m_bones[poseID];

    if (bone.parent == -1) {
      return glm::identity<Mat4d>();
    } else {
      return evaluateGlobalTransform(poseID, bone.parent);
    }
  }

  int64_t write(Stream * pStream, MeshSkeleton const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].m_bones) && pStream->write(pValue[i].m_poses)))
        return i;
    }
    return count;
  }

  int64_t read(Stream * pStream, MeshSkeleton * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].m_bones) && pStream->read(&pValue[i].m_poses)))
        return i;
    }
    return count;
  }

  int64_t write(Stream * pStream, MeshSkeleton::Bone const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].id) && pStream->write(pValue[i].name) && pStream->write(pValue[i].parent)))
        return i;
    }
    return count;
  }

  int64_t read(Stream * pStream, MeshSkeleton::Bone * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].id) && pStream->read(&pValue[i].name) && pStream->read(&pValue[i].parent)))
        return i;
    }
    return count;
  }


  int64_t write(Stream * pStream, MeshSkeleton::Pose const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].id) && pStream->write(pValue[i].name) && pStream->write(pValue[i].transforms)))
        return i;
    }
    return count;
  }

  int64_t read(Stream * pStream, MeshSkeleton::Pose * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].id) && pStream->read(&pValue[i].name) && pStream->read(&pValue[i].transforms)))
        return i;
    }
    return count;
  }

  int64_t write(Stream * pStream, MeshSkeleton::Transform const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].position) && pStream->write(pValue[i].orientation) && pStream->write(pValue[i].scale)))
        return i;
    }
    return count;
  }

  int64_t read(Stream * pStream, MeshSkeleton::Transform * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].position) && pStream->read(&pValue[i].orientation) && pStream->read(&pValue[i].scale)))
        return i;
    }
    return count;
  }
} // namespace bfc
