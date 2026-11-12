#include "BehavioursComponentEditor.h"
#include "Application.h"
#include "ui/Widgets.h"

namespace engine {
  void BehavioursEditor::draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                                components::Behaviours * pScripts) {
    auto pAssets = pEditor->getApp()->findSubsystem<AssetManager>();
    for (auto & pInstance : pScripts->instances()) {
      const bfc::String identifier = BehaviourClass::Registry::identifierOf(pInstance.get());
      bool keep = true;
      ImGui::CollapsingHeader(identifier.c_str(), &keep);
      if (!keep) {
        ImGui::OpenPopup("Remove Behaviour");
      }
    }

    if (ImGui::BeginPopup("Remove Behaviour")) {
      ImGui::Text("Are you sure?");
      ImGui::NewLine();
      ImGui::Button("Cancel");
      ImGui::SameLine();
      ImGui::Button("Confirm");
      ImGui::EndPopup();
    }

    if (ImGui::BeginMenu("Add Behaviour")) {
      for (bfc::String const & name : BehaviourClass::Registry::identifiers()) {
        bfc::Ref<BehaviourClass> pClass = BehaviourClass::Registry::find(name);
        if (pScripts->has(pClass.get()))
          continue;

        if (ImGui::MenuItem(name.c_str()))
          pScripts->add(pClass.get());
      }
      ImGui::EndMenu();
    }
  }
} // namespace engine
