#include "CoreComponentEditor.h"
#include "Application.h"
#include "Assets/AssetManager.h"
#include "ui/Widgets.h"

using namespace bfc;

namespace engine {
  void NameEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID, components::Name * pComponent) {
    ui::Input("Name", &pComponent->name);
  }

  void TransformEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID, components::Transform * pComponent) {
    Vec3d translation = pComponent->translation();
    Vec3d scale       = pComponent->scale();
    Vec3d ypr         = pComponent->ypr();

    ui::Input("Translation", &translation);
    ui::Input("Scale", &scale);
    ui::Input("Yaw/Pitch/Roll", &ypr);

    pComponent->setTranslation(translation);
    pComponent->setScale(scale);
    pComponent->setYpr(ypr);
  }

  void CameraEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID, components::Camera * pComponent) {
    ui::Input("Near Plane", &pComponent->nearPlane);
    ui::Input("Far Plane", &pComponent->farPlane);

    float fovDegs = glm::degrees(pComponent->fov);
    ui::Input("Field of View", &fovDegs);
    pComponent->fov = glm::radians(fovDegs);

    ui::Input("Viewport Position", &pComponent->viewportPosition);
    ui::Input("Viewport Size", &pComponent->viewportSize);
  }

  void LightEditor::draw(LevelEditor * pEditor, Ref<Level> const& pLevel, EntityID entityID, components::Light * pComponent) {
    ui::Input("Strength", &pComponent->strength);
    ImGui::ColorPicker3("Colour", &pComponent->colour.x);
    ImGui::Checkbox("Cast Shadows", &pComponent->castShadows);

    switch (pComponent->type) {
    case components::LightType_Sun:
      ImGui::ColorPicker3("Ambient", &pComponent->ambient.x);
      break;
    case components::LightType_Spot:
      ImGui::SliderAngle("Inner Angle", &pComponent->innerConeAngle, 0.0f, pComponent->innerConeAngle);
      ImGui::SliderAngle("Outer Angle", &pComponent->outerConeAngle, pComponent->innerConeAngle, 180.0f);
    case components::LightType_Point: // fallthrough
      ui::Label("Attenuation");
      ui::Separator();
      ui::Input("Constant", &pComponent->attenuation.x);
      ui::Input("Linear", &pComponent->attenuation.y);
      ui::Input("Exponential", &pComponent->attenuation.z);
      break;
    }
  }

  void SkyboxEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID, components::Skybox * pComponent) {
    AssetManager *pAssets = pEditor->getApp()->findSubsystem<AssetManager>().get();

    LevelEditor::drawAssetSelector("Texture", &pComponent->pTexture, pAssets);
  }

  void StaticMeshEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID, components::StaticMesh * pComponent) {
    AssetManager * pAssets = pEditor->getApp()->findSubsystem<AssetManager>().get();

    ui::Input("Cast Shadows", &pComponent->castShadows);

    LevelEditor::drawAssetSelector("Mesh", &pComponent->pMesh, pAssets);

    ui::Separator();
    ui::Label("Materials");

    for (int64_t i = 0; i < pComponent->materials.size(); ++i) {
      ImGui::PushID((int)i);
      LevelEditor::drawAssetSelector("Material %lld", &pComponent->materials[i], pAssets);
      ImGui::PopID();
    }
  }

  void PostProcessVolumeEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID, components::PostProcessVolume * pComponent) {
    LevelEditor::drawEntitySelector("Effects", &pComponent->effects, pLevel.get());
    ImGui::Checkbox("Enable",   &pComponent->enabled);
    ImGui::Checkbox("Infinite", &pComponent->infinite);
    ui::Input("Min", &pComponent->extents.min);
    ui::Input("Max", &pComponent->extents.max);
  }

  
  void PostProcess_TonemapEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID, components::PostProcess_Tonemap * pComponent) {
    ui::Input("Exposure", &pComponent->exposure);
  }

  void PostProcess_BloomEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID, components::PostProcess_Bloom * pComponent) {
    AssetManager * pAssets = pEditor->getApp()->findSubsystem<AssetManager>().get();
    ui::Input("Filter Radius", &pComponent->filterRadius);
    ui::Input("Strength", &pComponent->strength);
    ui::Input("Threshold", &pComponent->threshold);
    LevelEditor::drawAssetSelector("Dirt", &pComponent->dirt, pAssets);
    ui::Input("Dirt Intensity", &pComponent->dirtIntensity);
  }

  void PostProcess_SSAOEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID, components::PostProcess_SSAO * pComponent) {
    ui::Input("Bias", &pComponent->bias);
    ui::Input("Strength", &pComponent->strength);
    ui::Input("Radius", &pComponent->radius);
  }

  void PostProcess_SSREditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID, components::PostProcess_SSR * pComponent) {
    ui::Input("Max Distance", &pComponent->maxDistance);
    ui::Input("Resolution", &pComponent->resolution);
    ui::Input("Steps", &pComponent->steps);
    ui::Input("Thickness", &pComponent->thickness);
  }
} // namespace engine
