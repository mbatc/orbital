#pragma once

#include "../core/Core.h"
#include "../core/Pool.h"
#include "../math/MathTypes.h"

namespace bfc {
  class BFC_API MeshSkeleton {
  public:
    struct Transform {
      Vec3d position    = Vec3d(0);
      Vec3d scale       = Vec3d(1);
      Quatd orientation = glm::identity<Quatd>();
    };

    struct Bone {
      String  name;
      int64_t id;
      int64_t parent;
    };

    struct Pose {
      int64_t id;
      String name;
      Vector<Transform> transforms;
    };

    MeshSkeleton();

    /// Get the root bone ID (this is always 0)
    inline constexpr int64_t getRootID() {
      return 0;
    }

    /// Get the ID of all the bones in the skeleton.
    Vector<int64_t> getBones() const;

    /// Add a new bone to the skeleton.
    int64_t addBone(StringView const & name, int64_t parent = 0);

    /// Remove a bone from the skeleton.
    /// If the bone has children, their parent will be set to the parent of the removed bone.
    bool removeBone(int64_t boneID);

    /// Get the parent of a bone.
    int64_t getParent(int64_t boneID) const;

    /// Get the name of a bone.
    StringView getBoneName(int64_t boneID) const;

    /// Set the name of a bone.
    void setBoneName(int64_t boneID, StringView const & name);

    /// Set the parent of a bone.
    void setBoneParent(int64_t boneID, int64_t parent);

    /// Add a new pose to the skeleton.
    /// @param name The name of the pose.
    /// @returns The ID of the new pose. This can be used to access/modify the pose.
    int64_t addPose(StringView const & name);

    /// Remove a pose from the skeleton.
    /// @param poseID The id of the pose to remove.
    bool removePose(int64_t poseID);

    /// Get the name of a pose.
    /// @param poseID The pose to get the name of.
    StringView getPoseName(int64_t poseID) const;

    /// Set the name of a pose.
    /// @param poseID The pose to set the name of.
    /// @param name   The new name for the pose.
    void setPoseName(int64_t poseID, StringView const & name);

    /// Set the position of a bone within a pose.
    /// @param poseID The ID of the pose to set the position in.
    /// @param boneID The ID of the bone to set the position for.
    /// @param position The position to set the bone to.
    void setBonePosition(int64_t poseID, int64_t boneID, Vec3d const & position);

    /// Set the scale of a bone within a pose.
    /// @param poseID The ID of the pose to set the scale in.
    /// @param boneID The ID of the bone to set the scale for.
    /// @param scale The scale to set the bone to.
    void setBoneScale(int64_t poseID, int64_t boneID, Vec3d const & scale);

    /// Set the orientation of a bone within a pose.
    /// @param poseID The ID of the pose to set the orientation in.
    /// @param boneID The ID of the bone to set the orientation for.
    /// @param ori The orientation to set the bone to.
    void setBoneOrientation(int64_t poseID, int64_t boneID, Quatd const & ori);

    /// Set the local transform of a bone within a pose.
    /// @param poseID The ID of the pose to set the orientation in.
    /// @param boneID The ID of the bone to set the orientation for.
    /// @param transform The transform to set the bone to.
    void setBoneLocalTransform(int64_t poseID, int64_t boneID, Mat4d const & transform);

    /// Set the global transform of a bone within a pose.
    /// @param poseID The ID of the pose to set the orientation in.
    /// @param boneID The ID of the bone to set the orientation for.
    /// @param transform The transform to set the bone to.
    void setBoneGlobalTransform(int64_t poseID, int64_t boneID, Mat4d const & transform);

    /// Evaluate the local transform of a bone in a pose.
    Mat4d evaluateLocalTransform(int64_t poseID, int64_t boneID) const;

    /// Evaluate the global transform of a bone in a pose.
    Mat4d evaluateGlobalTransform(int64_t poseID, int64_t boneID) const;

    /// Evaluate the global transform of a bone's parent.
    Mat4d evaluateParentTransform(int64_t poseID, int64_t boneID) const;

    friend BFC_API int64_t write(Stream * pStream, MeshSkeleton const * pValue, int64_t count);
    friend BFC_API int64_t read(Stream * pStream, MeshSkeleton * pValue, int64_t count);

  private:
    Pool<Bone> m_bones;
    Pool<Pose> m_poses;
  };

  BFC_API int64_t write(Stream * pStream, MeshSkeleton::Bone const * pValue, int64_t count);
  BFC_API int64_t read(Stream * pStream, MeshSkeleton::Bone * pValue, int64_t count);

  BFC_API int64_t write(Stream * pStream, MeshSkeleton::Pose const * pValue, int64_t count);
  BFC_API int64_t read(Stream * pStream, MeshSkeleton::Pose * pValue, int64_t count);

  BFC_API int64_t write(Stream * pStream, MeshSkeleton::Transform const * pValue, int64_t count);
  BFC_API int64_t read(Stream * pStream, MeshSkeleton::Transform * pValue, int64_t count);
}
