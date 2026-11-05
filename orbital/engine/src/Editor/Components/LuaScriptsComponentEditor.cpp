#include "LuaScriptsComponentEditor.h"
#include "Application.h"
#include "ui/Widgets.h"

namespace engine {
  void engine::LuaScriptsEditor::draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                                      components::LuaScripts * pScripts) {
    auto pAssets = pEditor->getApp()->findSubsystem<AssetManager>();
    for (auto& script : pScripts->scripts()) {
      bfc::URI uri = pAssets->uriOf(script.handle());
      bool     keep = true;
      ImGui::CollapsingHeader(bfc::String(uri.path().name()).c_str(), &keep);
      if (!keep) {
        ImGui::OpenPopup("Remove Script");
      }
    }

    if (ImGui::BeginPopup("Remove Script"))
    {
      ImGui::Text("Are you sure?");
      ImGui::NewLine();
      ImGui::Button("Cancel");
      ImGui::SameLine();
      ImGui::Button("Confirm");
      ImGui::EndPopup();
    }

    engine::Asset<ScriptAsset> script;
    pEditor->drawAssetSelector("New Script", &script);
    if (script.handle() != InvalidAssetHandle) {
      pScripts->add(script);
    }
  }
} // namespace engine
