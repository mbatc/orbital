#include "LevelEditor.h"
#include "Application.h"
#include "Input.h"
#include "platform/FileDialog.h"
#include "Assets/AssetManager.h"
#include "Levels/CoreComponents.h"
#include "Editor/Components/CoreComponentEditor.h"
#include "Levels/Level.h"
#include "Levels/LevelManager.h"
#include "Levels/LevelSerializer.h"
#include "Rendering/Rendering.h"
#include "Viewport/LevelEditorViewport.h"
#include "platform/Events.h"
#include "platform/Window.h"
#include "ui/Widgets.h"
#include "util/Log.h"
#include "Viewport/GameViewport.h"

using namespace bfc;

namespace engine {
  LevelEditor::LevelEditor()
    : Subsystem(TypeID<LevelEditor>(), "LevelEditor") {}

  bool LevelEditor::init(Application * pApp) {
    auto pAssets    = pApp->findSubsystem<AssetManager>();
    auto pRendering = pApp->findSubsystem<Rendering>();
    auto pLevels    = pApp->findSubsystem<LevelManager>();

    Ref<Input> pInputs = pApp->findSubsystem<Input>();

    settings.startupLevel     = pApp->addSetting("level-editor/startup-level",     URI::File("game:levels/main.level"));
    settings.camera.fov       = pApp->addSetting("level-editor/camera/fov",        glm::radians(60.0f));
    settings.camera.farPlane  = pApp->addSetting("level-editor/camera/far-plane",  0.1f);
    settings.camera.nearPlane = pApp->addSetting("level-editor/camera/near-plane", 100000.0f);

    // Load the startup level
    URI levelPath = settings.startupLevel.get();
    if (!pAssets->getFileSystem()->exists(levelPath)) {
      auto pNewLevel = pLevels->getActiveLevel();

      Filename skyboxPath = "game:skybox/nebula.skybox";
      EntityID testEntity = pNewLevel->create();

      pNewLevel->add<components::Name>  (testEntity).name     = "Environment";
      pNewLevel->add<components::Skybox>(testEntity).pTexture = pAssets->load<bfc::Texture>(URI::File(skyboxPath));

      components::Transform & sunTransform = pNewLevel->add<components::Transform>(testEntity);
      sunTransform.lookAt(bfc::Vec3d(1, 1, -1));

      components::Light & sun = pNewLevel->add<components::Light>(testEntity);
      sun.ambient             = {0.7f, 0.7f, 0.7f};
      sun.colour              = {0.7f, 0.7f, 0.7f};
      sun.castShadows         = true;

      pLevels->save(levelPath, *pNewLevel);
    }

    pLevels->setActiveLevel(pLevels->load(settings.startupLevel.get()));
    
    // Create an editor viewport and render the active level.
    m_pEditorViewport = NewRef<LevelEditorViewport>(pRendering->getDevice(), pAssets.get());
    m_pEditorViewport->setLevel(pLevels->getActiveLevel());

    m_pViewportListener = m_pEditorViewport->getEvents()->addListener();
    m_pViewportListener->on([=](bfc::events::DroppedFiles const & e) {
      for (Filename const & file : e.files) {
        pLevels->Import(pLevels->getActiveLevel().get(), URI::File(file));
      }
    });

    m_pAppListener = pApp->addListener();

    m_pAppListener->on([=](events::OnRenderViewport const & e) {
      if (m_pDrawData != nullptr && e.isMainViewport) {
        m_uiContext.renderDrawData(m_pDrawData);
        m_pDrawData = nullptr;
      }
    });

    m_pAppListener->on([=](events::OnLevelActivated const & e) {
      // TODO: May not want to always sync the editor level with the active level.
      //       e.g. editing a sub-level in an external window?
      m_pEditorViewport->setLevel(e.pLevel);
    });

    // Render the editor viewport to the main window.
    pRendering->setMainViewport(m_pEditorViewport);

    // Init ui context rendering.
    m_uiContext.init(pRendering->getDevice());
    m_uiContext.getEvents()->listenTo(pRendering->getMainWindow()->getEvents());

    addComponentEditor<NameEditor>();
    addComponentEditor<TransformEditor>();
    addComponentEditor<CameraEditor>();
    addComponentEditor<LightEditor>();
    addComponentEditor<SkyboxEditor>();
    addComponentEditor<StaticMeshEditor>();
    addComponentEditor<PostProcessVolumeEditor>();
    addComponentEditor<PostProcess_TonemapEditor>();
    addComponentEditor<PostProcess_BloomEditor>();
    addComponentEditor<PostProcess_SSAOEditor>();
    addComponentEditor<PostProcess_SSREditor>();

    return true;
  }

  void LevelEditor::shutdown() {
    m_pEditorViewport = nullptr;
  }

  void LevelEditor::loop(Application * pApp) {
    auto pRendering  = pApp->findSubsystem<Rendering>();
    auto pAssets     = pApp->findSubsystem<AssetManager>();
    auto pLevels     = pApp->findSubsystem<LevelManager>();
    auto pFileSystem = pApp->findSubsystem<VirtualFileSystem>();

    m_uiContext.beginFrame(pRendering->getMainWindow()->getSize());

    drawUI(pLevels, pAssets, pRendering, pFileSystem);

    ImGui::Render();
    m_pDrawData = ImGui::GetDrawData();

    if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard) {
      m_pEditorViewport->getEvents()->stopListening(pRendering->getMainWindow()->getEvents());
    } else {
      m_pEditorViewport->getEvents()->listenTo(pRendering->getMainWindow()->getEvents());
    }

    // Apply camera controls
    m_pEditorViewport->camera.update(pApp->getDeltaTime());

    Keyboard & kbd = m_pEditorViewport->getKeyboard();

    if (kbd.isDown(KeyCode_Control)) {
      if (kbd.isPressed(KeyCode_S)) {
        URI levelPath = settings.startupLevel.get();

        BFC_LOG_INFO("LevelEditor", "Saving level to %s", levelPath);
        LevelSerializer(pAssets.get()).serialize(levelPath, *pLevels->getActiveLevel());
      }

      if (kbd.isPressed(KeyCode_1)) {
        // Reload all the shaders
        BFC_LOG_INFO("LevelEditor", "Reloading shaders");
        for (AssetHandle handle : pAssets->findHandles<bfc::Shader>()) {
          pAssets->reload(handle);
        }
      }
    }
  }

  bool LevelEditor::drawEntitySelector(bfc::StringView const & name, EntityID * pEntityID, Level * pLevel) {
    bool        changed      = false;
    bfc::String selectedName = "";
    if (*pEntityID != InvalidEntity) {
      if (components::Name * pName = pLevel->tryGet<components::Name>(*pEntityID))
        selectedName = pName->name;
      else
        selectedName = "[ unnamed ]";
    }

    ui::Input(name, &selectedName);
    ImGui::PushID(name.begin(), name.end());
    if (ImGui::IsItemClicked())
      ImGui::OpenPopup("Select Entity");

    if (ImGui::BeginPopup("Select Entity")) {
      for (EntityID entity : pLevel->entities()) {
        ImGui::PushID((int)(entity & 0x00000000FFFFFFFF));
        ImGui::PushID((int)((entity >> 32) & 0x00000000FFFFFFFF));

        bfc::String optionName = "[ unnamed ]";
        if (components::Name * pName = pLevel->tryGet<components::Name>(entity))
          optionName = pName->name;

        if (ImGui::Selectable(optionName.c_str(), entity == *pEntityID)) {
          *pEntityID = entity;
          changed    = true;
        }

        ImGui::PopID();
        ImGui::PopID();
      }

      ImGui::EndPopup();
    }
    ImGui::PopID();

    return changed;
  }

  bool LevelEditor::drawAssetSelector(StringView const & name, Ref<void> * ppAsset, type_index const & assetType, AssetManager * pManager) {
    bool        changed      = false;
    bfc::String selectedName = "[ None ]";

    AssetHandle handle = pManager->find(*ppAsset);

    if (handle != InvalidAssetHandle) {
      selectedName = pManager->uriOf(handle).str();
    }

    ui::Input(name, &selectedName);

    ImGui::PushID(name.begin(), name.end());
    if (ImGui::IsItemClicked())
      ImGui::OpenPopup("Select Asset");

    if (ImGui::BeginPopup("Select Asset")) {
      auto handles = pManager->findHandles([assetType](URI const & uri, type_index const & type, StringView const & loaderID) {
        return type == assetType;
      });

      for (AssetHandle option : handles) {
        ImGui::PushID((int)(handle & 0x00000000FFFFFFFF));
        ImGui::PushID((int)((handle >> 32) & 0x00000000FFFFFFFF));

        bfc::String optionName = "[ unnamed ]";
        URI uri = pManager->uriOf(handle);

        if (ImGui::Selectable(uri.c_str(), option == handle)) {
          handle  = option;
          changed = true;
        }

        ImGui::PopID();
        ImGui::PopID();
      }

      ImGui::EndPopup();
    }

    ImGui::PopID();

    if (changed) {
      *ppAsset = pManager->load(handle, assetType);
    }

    return changed;
  }

  void LevelEditor::drawUI(bfc::Ref<LevelManager> const & pLevels, bfc::Ref<AssetManager> const & pAssets, bfc::Ref<Rendering> const & pRendering,
                           bfc::Ref<VirtualFileSystem> const & pFileSystem) {
    Ref<Level> pLevel = pLevels->getActiveLevel();

    drawLevelPanel(pLevels, pAssets, pRendering, pLevel);
    drawEntityProperties(pLevel, m_selected);
    drawEditorSettings();
    drawAssetsPanel(pFileSystem, pLevels);
  }

  void LevelEditor::drawAssetsPanel(Ref<VirtualFileSystem> const & pFileSystem, Ref<LevelManager> const & pLevels) {
    ImGui::Begin("Assets");
    for (String const & drive : pFileSystem->drives()) {
      if (ImGui::Selectable(drive.c_str())) {
        m_selectedAssetPath = URI::File(String::format("%s:/", drive));
      }
    }

    ImGui::Separator();

    if (!m_selectedAssetPath.pathView().empty() && m_selectedAssetPath.pathView() != "/") {
      if (ImGui::Selectable("..")) {
        m_selectedAssetPath = m_selectedAssetPath.resolveRelativeReference("../");
        m_selectedAssetPath = m_selectedAssetPath.withPath(m_selectedAssetPath.path().getDirect());
      }
    }

    for (URI const & item : pFileSystem->walk(m_selectedAssetPath)) {
      bool isLeaf          = pFileSystem->isLeaf(item);
      bool isClicked       = ImGui::Selectable(String(item.path().name()).c_str());
      bool isDoubleClicked = isClicked && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

      if (!isLeaf && isClicked) {
        m_selectedAssetPath = item;
      }

      if (isLeaf && isDoubleClicked) {
        if (item.path().extension() == "level") {
          pLevels->setActiveLevel(pLevels->load(item));
        } else {
          pLevels->Import(pLevels->getActiveLevel().get(), item);
        }
      }
    }

    ImGui::End();
  }

  void LevelEditor::drawLevelPanel(Ref<LevelManager> const & pLevels, Ref<AssetManager> const & pAssets, Ref<Rendering> const & pRendering,
                                   Ref<Level> const & pLevel) {
    ImGui::Begin("Level", 0, ImGuiWindowFlags_MenuBar);
    ImGui::BeginMenuBar();

    auto desiredState = pLevels->getSimulateState();
    switch (desiredState) {
    case SimulateState_Stopped: {
      if (ImGui::MenuItem("Play"))
        desiredState = SimulateState_Playing;
    } break;
    case SimulateState_Playing: {
      if (ImGui::MenuItem("Pause"))
        desiredState = SimulateState_Paused;
      if (ImGui::MenuItem("Stop"))
        desiredState = SimulateState_Stopped;
    } break;
    case SimulateState_Paused: {
      if (ImGui::MenuItem("Play"))
        desiredState = SimulateState_Playing;
      if (ImGui::MenuItem("Stop"))
        desiredState = SimulateState_Stopped;
    } break;
    }

    if (desiredState != pLevels->getSimulateState()) {
      bool activateGameViewport   = pLevels->getSimulateState() == SimulateState_Stopped;
      bool activateEditorViewport = desiredState == SimulateState_Stopped;

      pLevels->setSimulateState(desiredState);

      for (auto [name, pDevice] : pRendering->getMainViewport()->getInputDevices()) {
        pLevels->getApp()->findSubsystem<Input>()->setInputDevice(name, nullptr);
      }

      if (activateEditorViewport) {
        m_pEditorViewport->setLevel(pLevels->getActiveLevel());
        pRendering->setMainViewport(m_pEditorViewport);
      } else if (activateGameViewport) {
        auto pGameViewport = NewRef<GameViewport>(pRendering->getDevice(), pAssets.get());
        pGameViewport->setLevel(pLevels->getActiveLevel());
        pRendering->setMainViewport(pGameViewport);
      }

      for (auto [name, pDevice] : pRendering->getMainViewport()->getInputDevices()) {
        pLevels->getApp()->findSubsystem<Input>()->setInputDevice(name, pDevice);
      }
    }

    if (ImGui::BeginMenu("File")) {
      if (ImGui::Selectable("Load Startup Level")) {
        BFC_LOG_INFO("LevelManager", "Reloading level");

        Ref<Level> pLoaded = pLevels->load(settings.startupLevel.get());
        if (pLoaded != nullptr) {
          // TODO: pLevel should be modified? Or this menu item should not be in the "level panel" if it is supposed to be 
          //       used with any arbitrary Level
          pLevels->setActiveLevel(pLoaded);
        }
      }

      if (ImGui::Selectable("New Level")) {
        pLevels->setActiveLevel(bfc::NewRef<Level>());
      }

      {
        ImGui::BeginDisabled(!pLevel->sourceUri.has_value());
        if (ImGui::Selectable("Save Level")) {
          pLevels->save(pLevel->sourceUri.value(), *pLevel);
        }
        ImGui::EndDisabled();
      }

      if (ImGui::Selectable("Save Level As")) {
        FileDialog dialog;
        dialog.setFilter({".level"}, {"Orbital Game Level"});
        if (pLevel->sourceUri.has_value())
          dialog.setFile(pLevel->sourceUri.value().path());

        if (dialog.save()) {
          auto paths = dialog.getSelected();

          pLevels->save(URI::File(paths.front()), *pLevel);
          pLevel->sourceUri = URI::File(paths.front());
        }
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::BeginMenu("Create")) {
        EntityID newEntity;
        if (ImGui::Selectable("Empty")) {
          newEntity = pLevel->create();
        }

        if (ImGui::Selectable("Transform")) {
          newEntity                         = pLevel->create();
          components::Transform & transform = pLevel->add<components::Transform>(newEntity);
          transform.setParent(pLevel.get(), m_selected);
        }

        ImGui::EndMenu();
      }

      ImGui::BeginDisabled(m_selected == InvalidEntity);
      if (ImGui::BeginMenu("Selection")) {
        if (ImGui::Selectable("Copy")) {
          m_selected = pLevel->copy(m_selected);
        }

        if (ImGui::Selectable("Delete")) {
          pLevel->remove(m_selected);
        }
        ImGui::EndMenu();
      }
      ImGui::EndDisabled();

      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();

    for (EntityID entityID : pLevel->entities()) {
      if (pLevel->has<components::Transform>(entityID)) {
        continue;
      }

      auto * pName = pLevel->tryGet<components::Name>(entityID);
      ImGui::PushID((int)(entityID & 0x00000000FFFFFFFF));
      ImGui::PushID((int)((entityID >> 32) & 0x00000000FFFFFFFF));

      if (ImGui::Selectable(pName ? pName->name.c_str() : "[ Unnamed ]", m_selected == entityID, ImGuiSelectableFlags_SpanAvailWidth)) {
        if (m_selected == entityID)
          m_selected = InvalidEntity;
        else
          m_selected = entityID;
      }

      ImGui ::PopID();
      ImGui ::PopID();
    }

    ImGui::Separator();
    bfc::Vector<EntityID> rootEntities;
    for (auto & [component] : pLevel->getView<components::Transform>()) {
      if (!pLevel->contains(component.parent())) {
        rootEntities.pushBack(pLevel->toEntity(&component));
      }
    }

    for (EntityID id : rootEntities) {
      drawTransformTree(pLevel, id);
    }

    ImGui::End();
  }

  void LevelEditor::drawEntityProperties(bfc::Ref<Level> const & pLevel, EntityID entityID) {
    ImGui::Begin("Properties", 0, ImGuiWindowFlags_MenuBar);
    if (entityID != InvalidEntity) {
      ImGui::BeginMenuBar();

      if (ImGui::BeginMenu("Add Component")) {
        drawAddComponentMenu(pLevel, entityID);
        ImGui::EndMenu();
      }

      ImGui::EndMenuBar();

      drawEntityComponentProperties(pLevel, entityID);
    } else {
    
    }
    ImGui::End();
  }

  void LevelEditor::drawEditorSettings() {
    ImGui::Begin("Editor");

    if (ImGui::BeginTabBar("EditorTabs")) {
      if (ImGui::BeginTabItem("Camera")) {
        drawCameraProperties(&m_pEditorViewport->camera);

        m_pEditorViewport->camera.setFOV(settings.camera.fov.get());
        m_pEditorViewport->camera.setFarPlane(settings.camera.nearPlane.get());
        m_pEditorViewport->camera.setNearPlane(settings.camera.farPlane.get());

        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    }

    ImGui::End();
  }

  void LevelEditor::drawCameraProperties(EditorCamera * pCamera) {
    float fov       = glm::degrees(settings.camera.fov.get());
    float nearPlane = settings.camera.nearPlane.get();
    float farPlane  = settings.camera.farPlane.get();

    ui::Input("Field of View", &fov);
    ui::Input("Near Plane",    &nearPlane);
    ui::Input("Far Plane",     &farPlane);

    ui::Input("Speed", &pCamera->speedMultiplier);

    settings.camera.fov.set(glm::radians(fov));
    settings.camera.farPlane.set(farPlane);
    settings.camera.nearPlane.set(nearPlane);
  }

  struct DnDEntityID {
    EntityID id;
  };

  void LevelEditor::drawTransformTree(Ref<Level> const & pLevel, EntityID entityID) {
    auto & transform = pLevel->get<components::Transform>(entityID);
    auto * pName     = pLevel->tryGet<components::Name>(entityID);

    ImGui::PushID((int)(entityID & 0x00000000FFFFFFFF));
    ImGui::PushID((int)((entityID >> 32) & 0x00000000FFFFFFFF));

    int flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (m_selected == entityID)
      flags |= ImGuiTreeNodeFlags_Selected;
    if (transform.children().size() == 0)
      flags |= ImGuiTreeNodeFlags_Leaf;

    bool open = ImGui::TreeNodeEx(pName ? pName->name.c_str() : "[ Unnamed ]", flags);
    if (ImGui::IsItemClicked()) {
      if (m_selected == entityID)
        m_selected = InvalidEntity;
      else
        m_selected = entityID;
    }


    if (ui::BeginDragDropSource()) {
      ui::SetDragDropPayload(DnDEntityID{ entityID });
      ui::EndDragDropSource();
    }

    if (ui::BeginDragDropTarget()) {
      std::optional<DnDEntityID> dnd = ui::AcceptDragDropPayload<DnDEntityID>();
      if (dnd.has_value() && dnd->id != entityID) {
        transform.addChild(pLevel.get(), dnd->id);
      }
      ui::EndDragDropTarget();
    }

    if (open) {
      for (EntityID child : transform.children()) {
        drawTransformTree(pLevel, child);
      }

      ImGui::TreePop();
    }
    ImGui::PopID();
    ImGui::PopID();
  }

  void LevelEditor::drawEntityComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID) {
    for (auto& [type, pStorage] : pLevel->components()) {
      void * pComponent = pStorage->getOpaque(entityID);

      Ref<IComponentEditor> pEditor = m_componentEditors.getOr(type, nullptr);

      if (pComponent == nullptr)
        continue;

      String typeName = ILevelComponentType::findName(type);
      if (typeName.length() == 0) {
        continue;
      }

      ImGui::PushID(typeName.c_str());

      bool visible = true;
      if (ImGui::CollapsingHeader(typeName.c_str(), &visible)) {
        ImGui::Indent();
        if (pEditor != nullptr)
          pEditor->_draw(this, pLevel, entityID, pComponent);
        else
          ImGui::Text("No editor implemented");

        ImGui::Unindent();
      }

      if (!visible)
        ImGui::OpenPopup("Confirm Remove Component?");

      if (ImGui::BeginPopupModal("Confirm Remove Component?")) {
        ImGui::Text("Are you sure you want to remove this component");
        if (ImGui::Button("Cancel"))
          ImGui::CloseCurrentPopup();

        if (ImGui::Button("Yes")) {
          pStorage->erase(entityID);
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      ImGui::PopID();
    }
  }

  void LevelEditor::drawAddComponentMenu(bfc::Ref<Level> const & pLevel, EntityID targetEntityID) {
    for (auto & name : ILevelComponentType::names()) {
      Ref<ILevelComponentType>    pInterface  = ILevelComponentType::find(name);
      Ref<ILevelComponentStorage> pComponents = pLevel->components().getOr(pInterface->type(), nullptr);
      if (pComponents != nullptr && pComponents->exists(targetEntityID))
        continue;

      if (ImGui::Selectable(name.c_str())) {
        pInterface->addComponent(pLevel.get(), targetEntityID);
      }
    }
  }
} // namespace engine
