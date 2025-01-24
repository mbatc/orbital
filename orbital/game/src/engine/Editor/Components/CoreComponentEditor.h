#include "Editor/LevelEditor.h"
#include "Levels/CoreComponents.h"

namespace engine {
  class NameEditor : public LevelEditor::ComponentEditor<components::Name> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::Name * pComponent) override;
  };

  class TransformEditor : public LevelEditor::ComponentEditor<components::Transform> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::Transform * pTransform) override;
  };

  class CameraEditor : public LevelEditor::ComponentEditor<components::Camera> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::Camera * pCamera) override;
  };

  class LightEditor : public LevelEditor::ComponentEditor<components::Light> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::Light * pCamera) override;
  };

  class SkyboxEditor : public LevelEditor::ComponentEditor<components::Skybox> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::Skybox * pCamera) override;
  };

  class StaticMeshEditor : public LevelEditor::ComponentEditor<components::StaticMesh> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::StaticMesh * pCamera) override;
  };

  class PostProcessVolumeEditor : public LevelEditor::ComponentEditor<components::PostProcessVolume> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::PostProcessVolume * pCamera) override;
  };

  class PostProcess_TonemapEditor : public LevelEditor::ComponentEditor<components::PostProcess_Tonemap> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::PostProcess_Tonemap * pCamera) override;
  };

  class PostProcess_BloomEditor : public LevelEditor::ComponentEditor<components::PostProcess_Bloom> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::PostProcess_Bloom * pCamera) override;
  };

  class PostProcess_SSAOEditor : public LevelEditor::ComponentEditor<components::PostProcess_SSAO> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::PostProcess_SSAO * pCamera) override;
  };

  class PostProcess_SSREditor : public LevelEditor::ComponentEditor<components::PostProcess_SSR> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::PostProcess_SSR * pCamera) override;
  };
}
