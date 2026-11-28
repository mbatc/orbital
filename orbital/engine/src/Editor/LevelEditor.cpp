#include "LevelEditor.h"
#include "Application.h"
#include "Input.h"
#include "platform/FileDialog.h"
#include "platform/Events.h"
#include "platform/Window.h"
#include "Assets/AssetManager.h"
#include "Levels/Level.h"
#include "Levels/LevelManager.h"
#include "Levels/LevelSerializer.h"

#include "Levels/CoreComponents.h"
#include "Editor/Components/CoreComponentEditor.h"

#include "Physics.h"
#include "Editor/Components/PhysicsComponentEditor.h"

#include "Behaviours.h"
#include "Editor/Components/BehavioursComponentEditor.h"

#include "Rendering/Rendering.h"
#include "Rendering/DeferredRenderer.h"

#include "Viewport/LevelEditorViewport.h"
#include "Viewport/GameViewport.h"
#include "ui/Widgets.h"
#include "util/Log.h"

using namespace bfc;

namespace engine {
  template<typename T>
  class DefaultPropertyEditor : public LevelEditor::ComponentEditor<T> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const& pLevel, EntityID entityID, T * pValue) {
      ui::Input("", pValue);
    }
  };

  LevelEditor::LevelEditor()
    : Subsystem(TypeID<LevelEditor>(), "LevelEditor") {}

  bool LevelEditor::init(Application * pApp) {
    auto pLevels     = pApp->findSubsystem<LevelManager>();

    settings.startupLevel     = pApp->addSetting("level-editor/startup-level",     URI::File("game:levels/main.level"));
    settings.camera.fov       = pApp->addSetting("level-editor/camera/fov",        glm::radians(60.0f));
    settings.camera.farPlane  = pApp->addSetting("level-editor/camera/far-plane",  0.1f);
    settings.camera.nearPlane = pApp->addSetting("level-editor/camera/near-plane", 100000.0f);
    settings.camera.position  = pApp->addSetting("level-editor/camera/position",   Vec3d(0, 1, -2));
    settings.camera.ypr       = pApp->addSetting("level-editor/camera/ypr",        Vec3(0));

    // Load the startup level
    URI levelPath = settings.startupLevel.get();
    if (!fs::exists(levelPath)) {
      auto pNewLevel = pLevels->getActiveLevel();

      Filename skyboxPath = "game:skybox/nebula.skybox";
      EntityID testEntity = pNewLevel->create();

      pNewLevel->add<components::Name>  (testEntity).name     = "Environment";
      pNewLevel->add<components::Skybox>(testEntity).pTexture = asset_manager::load<graphics::Texture>(URI::File(skyboxPath));

      components::Transform & sunTransform = pNewLevel->add<components::Transform>(testEntity);
      sunTransform.lookAt(bfc::Vec3d(1, 1, -1));

      components::Light & sun = pNewLevel->add<components::Light>(testEntity);
      sun.ambient             = {0.7f, 0.7f, 0.7f};
      sun.colour              = {0.7f, 0.7f, 0.7f};
      sun.castShadows         = true;

      pLevels->setActiveLevel(pNewLevel);
      pLevels->save(levelPath, *pNewLevel);
    } else {
      pLevels->setActiveLevel(pLevels->load(settings.startupLevel.get()));
    }

    GraphicsDevice * pGraphicDevice = rendering::getDevice();

    // Create an editor viewport and render the active level.
    {
      auto pInitCmdList = pGraphicDevice->createCommandList();

      auto pRenderer = bfc::NewRef<DeferredRenderer>(pInitCmdList.get());
      rendering::registerRenderer(pRenderer);
      m_pEditorViewport = NewRef<LevelEditorViewport>(pRenderer);
      m_pEditorViewport->camera.position = settings.camera.position.get();
      m_pEditorViewport->camera.ypr      = settings.camera.ypr.get();
      auto pGameRenderer = bfc::NewRef<DeferredRenderer>(pInitCmdList.get());
      rendering::registerRenderer(pGameRenderer);
      m_pGameViewport = NewRef<GameViewport>(pGameRenderer);

      pGraphicDevice->submit(std::move(pInitCmdList));
    }
    m_pGameViewport->setLevel(pLevels->getActiveLevel());
    m_pEditorViewport->setLevel(pLevels->getActiveLevel());
    m_pEditorViewportRenderTarget = pGraphicDevice->createRenderTarget(RenderTargetType_Texture);
    m_pGameViewportRenderTarget   = pGraphicDevice->createRenderTarget(RenderTargetType_Texture);
    rendering::registerPlugin(bfc::NewRef<LevelEditorRenderingPlugin>(this));

    m_pViewportListener = m_pEditorViewport->getEvents()->addListener();
    m_pViewportListener->on([=](bfc::events::DroppedFiles const & e) {
      for (Filename const & file : e.files) {
        bfc::URI const virtualUri = fs::toVirtualUri(URI::File(file));

        BFC_LOG_INFO("LevelEditor", "File dropped. URI=%s", virtualUri);

        if (file.extension().equals("level", true)) {
          pLevels->setActiveLevel(pLevels->load(virtualUri));

          if (pLevels->getSimulateState() == SimulateState_Stopped) {
            settings.startupLevel.set(virtualUri);
          }
        } else {
          pLevels->Import(pLevels->getActiveLevel().get(), virtualUri);
        }
      }
    });

    m_pAppListener = pApp->addListener();

    m_pAppListener->on([=](events::OnLevelActivated const & e) {
      BFC_LOG_INFO("LevelEditor", "Level activated. Setting editor viewport level.");
      // TODO: May not want to always sync the editor level with the active level.
      //       e.g. editing a sub-level in an external window?
      m_pEditorViewport->setLevel(e.pLevel);
      m_pGameViewport->setLevel(e.pLevel);
    });

    auto pInitCmdList = pGraphicDevice->createCommandList();
    pInitCmdList->setDebugName("LevelEditor init");

    // Render the editor viewport to the main window.
    activateViewport(m_pEditorViewport);

    // Init ui context rendering.
    m_uiContext.init(pInitCmdList.get());
    m_uiContext.getEvents()->listenTo(rendering::getMainWindow()->getEvents());

    addPropertyEditor<DefaultPropertyEditor<int8_t>>();
    addPropertyEditor<DefaultPropertyEditor<int16_t>>();
    addPropertyEditor<DefaultPropertyEditor<int32_t>>();
    addPropertyEditor<DefaultPropertyEditor<int64_t>>();
    addPropertyEditor<DefaultPropertyEditor<uint8_t>>();
    addPropertyEditor<DefaultPropertyEditor<uint16_t>>();
    addPropertyEditor<DefaultPropertyEditor<uint32_t>>();
    addPropertyEditor<DefaultPropertyEditor<uint64_t>>();

    addPropertyEditor<DefaultPropertyEditor<float>>();
    addPropertyEditor<DefaultPropertyEditor<double>>();

    addPropertyEditor<DefaultPropertyEditor<Vec2>>();
    addPropertyEditor<DefaultPropertyEditor<Vec2d>>();
    addPropertyEditor<DefaultPropertyEditor<Vec2i>>();
    addPropertyEditor<DefaultPropertyEditor<Vec2u>>();
    addPropertyEditor<DefaultPropertyEditor<Vec2i64>>();
    addPropertyEditor<DefaultPropertyEditor<Vec2u64>>();

    addPropertyEditor<DefaultPropertyEditor<Vec3>>();
    addPropertyEditor<DefaultPropertyEditor<Vec3d>>();
    addPropertyEditor<DefaultPropertyEditor<Vec3i>>();
    addPropertyEditor<DefaultPropertyEditor<Vec3u>>();
    addPropertyEditor<DefaultPropertyEditor<Vec3i64>>();
    addPropertyEditor<DefaultPropertyEditor<Vec3u64>>();

    addPropertyEditor<DefaultPropertyEditor<Vec4>>();
    addPropertyEditor<DefaultPropertyEditor<Vec4d>>();
    addPropertyEditor<DefaultPropertyEditor<Vec4i>>();
    addPropertyEditor<DefaultPropertyEditor<Vec4u>>();
    addPropertyEditor<DefaultPropertyEditor<Vec4i64>>();
    addPropertyEditor<DefaultPropertyEditor<Vec4u64>>();

    addPropertyEditor<NameEditor>();
    addPropertyEditor<TransformEditor>();
    addPropertyEditor<CameraEditor>();
    addPropertyEditor<LightEditor>();
    addPropertyEditor<SkyboxEditor>();
    addPropertyEditor<StaticMeshEditor>();
    addPropertyEditor<PostProcessVolumeEditor>();
    addPropertyEditor<PostProcess_TonemapEditor>();
    addPropertyEditor<PostProcess_BloomEditor>();
    addPropertyEditor<PostProcess_SSAOEditor>();
    addPropertyEditor<PostProcess_SSREditor>();

    addPropertyEditor<ColliderCubeEditor>();
    addPropertyEditor<ColliderCapsuleEditor>();
    addPropertyEditor<ColliderSphereEditor>();
    addPropertyEditor<ColliderMeshEditor>();
    addPropertyEditor<RigidBodyEditor>();

    addPropertyEditor<BehavioursEditor>();

    pGraphicDevice->submit(std::move(pInitCmdList));

    return true;
  }

  void LevelEditor::shutdown() {
    settings.camera.position.set(m_pEditorViewport->camera.position);
    settings.camera.ypr.set(m_pEditorViewport->camera.ypr);
    m_pEditorViewport = nullptr;
    m_uiContext.deinit();
  }

  void LevelEditor::loop(Application * pApp) {
    auto pLevels     = pApp->findSubsystem<LevelManager>();

    m_uiContext.beginFrame(rendering::getMainWindow()->getSize());

    drawUI(pLevels);

    ImGui::Render();

    m_pDrawData = ImGui::GetDrawData();

    // Route inputs to active viewport
    routeInputsToActiveViewport(rendering::getMainWindow()->getEvents());

    // Apply camera controls
    m_pEditorViewport->camera.update(pApp->getDeltaTime());
    Keyboard & kbd = m_pEditorViewport->getKeyboard();
    if (kbd.isDown(KeyCode_Control)) {
      if (kbd.isPressed(KeyCode_S)) {
        URI levelPath = settings.startupLevel.get();

        BFC_LOG_INFO("LevelEditor", "Saving level to %s", levelPath);
        LevelSerializer().serialize(levelPath, *pLevels->getActiveLevel());
      }

      if (kbd.isPressed(KeyCode_1)) {
        BFC_LOG_INFO("LevelEditor", "Reloading shaders");
        for (AssetHandle handle : asset_manager::findHandles<bfc::graphics::Program>()) {
          asset_manager::reload(handle);
        }
      }

      if (kbd.isPressed(KeyCode_2)) {
        BFC_LOG_INFO("LevelEditor", "Saving app settings");

        getApp()->saveSettings();
      }
    }

    if (kbd.isPressed(KeyCode_R)) {
      m_manipulator.op = ImGuizmo::ROTATE;
    }

    if (kbd.isPressed(KeyCode_T)) {
      m_manipulator.op = ImGuizmo::TRANSLATE;
    }

    if (kbd.isPressed(KeyCode_Y)) {
      m_manipulator.op = ImGuizmo::SCALE;
    }

    if (kbd.isPressed(KeyCode_U)) {
      m_manipulator.op = ImGuizmo::UNIVERSAL;
    }
  }

  void LevelEditor::routeInputsToActiveViewport(Events * pEvents) {
    bool viewportWantsInput = false;
    if (m_gameViewportWantsInput) {
      activateViewport(m_pGameViewport);
      viewportWantsInput = m_gameViewportWantsInput;
    } else if (m_editorViewportWantsInput) {
      activateViewport(m_pEditorViewport);
      viewportWantsInput = m_editorViewportWantsInput;
    }

    if (!m_pActiveViewport->wantsInputCapture()) {
      if (!viewportWantsInput || ImGui::GetIO().WantCaptureKeyboard) {
        m_pActiveViewport->getEvents()->stopListening(pEvents);
      } else {
        m_pActiveViewport->getEvents()->listenTo(pEvents);
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
        ImGui::PushID((int)((uint64_t)entity & 0x00000000FFFFFFFF));
        ImGui::PushID((int)(((uint64_t)entity >> 32) & 0x00000000FFFFFFFF));

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

  static void discoverAssets(URI const & basePath, bfc::type_index const & assetType) {
    for (auto & uri : fs::walk(basePath)) {
      if (fs::isLeaf(uri)) {
        if (asset_manager::canLoad(uri, assetType))
          asset_manager::add(uri, assetType); // add but don't load
      } else {
        discoverAssets(uri, assetType);
      }
    }
  }

  static void discoverAssets(bfc::type_index const & assetType) {
    for (auto & drive : fs::drives()) {
      discoverAssets(String::format("file:%s:/", drive), assetType);
    }
  }

  bool LevelEditor::drawAssetSelector(StringView const & name, Ref<void> * ppAsset, type_index const & assetType) {
    AssetHandle handle  = asset_manager::find(*ppAsset);
    bool        changed = drawAssetSelector(name, &handle, assetType, *ppAsset != nullptr ? "Unmanaged" : "[ None ]");
    *ppAsset            = asset_manager::load(handle, assetType);
    return changed;
  }

  bool LevelEditor::drawAssetSelector(bfc::StringView const & name, AssetHandle * pHandle, bfc::type_index const & assetType,
                                      bfc::StringView const & emptyPreview) {
    bool        changed      = false;
    bfc::String selectedName = emptyPreview;

    if (*pHandle != InvalidAssetHandle) {
      selectedName = asset_manager::uriOf(*pHandle).str();
    }

    ui::Input(name, &selectedName);

    ImGui::PushID(name.begin(), name.end());
    if (ImGui::IsItemClicked())
      ImGui::OpenPopup("Select Asset");

    if (ImGui::BeginPopup("Select Asset")) {
      if (ImGui::IsWindowAppearing()) {
        discoverAssets(assetType);
      }

      auto handles = asset_manager::findHandles(
        [assetType](URI const & uri, type_index const & type, StringView const & loaderID) { return type == assetType; });

      if (ImGui::Selectable("[ None ]"), *pHandle == InvalidAssetHandle) {
        *pHandle = InvalidAssetHandle;
      }

      for (AssetHandle option : handles) {
        ImGui::PushID((int)(option & 0x00000000FFFFFFFF));
        ImGui::PushID((int)((option >> 32) & 0x00000000FFFFFFFF));

        bfc::String optionName = "[ unnamed ]";
        URI         uri        = asset_manager::uriOf(option);

        if (ImGui::Selectable(uri.c_str(), option == *pHandle)) {
          *pHandle = option;
          changed  = true;
        }

        ImGui::PopID();
        ImGui::PopID();
      }

      ImGui::EndPopup();
    }

    ImGui::PopID();

    return changed;
  }

  void LevelEditor::activateViewport(bfc::Ref<Viewport> pNewViewport) {
    if (m_pActiveViewport == pNewViewport) {
      return;
    }

    BFC_LOG_INFO("LevelEditor", "Mapping input devices from new viewport to the input subsystem");

    if (m_pActiveViewport != nullptr) {
      for (auto [name, pDevice] : m_pActiveViewport->getInputDevices()) {
        inputs::setInputDevice(name, nullptr);
      }
    }

    if (pNewViewport != nullptr) {
      for (auto [name, pDevice] : pNewViewport->getInputDevices()) {
        inputs::setInputDevice(name, pDevice);
      }
    }

    m_pActiveViewport = pNewViewport;
  }

  void LevelEditor::onRenderFrame(bfc::graphics::CommandList * pCmdList, bfc::graphics::RenderTargetRef renderTarget) {
    if (m_pEditorViewportRenderTarget->getSize() != m_editorViewportSize) {
      bfc::graphics::loadTexture2D(pCmdList, &m_pEditorViewportColour, m_editorViewportSize,
                                   PixelFormat_RGBAf16);
      bfc::graphics::loadTexture2D(pCmdList, &m_pEditorViewportDepth, m_editorViewportSize,
                                   DepthStencilFormat_D24S8);
      m_pEditorViewportRenderTarget->attachColour(m_pEditorViewportColour);
      m_pEditorViewportRenderTarget->attachDepth(m_pEditorViewportDepth);
    }

    m_pEditorViewport->setSize(pCmdList, m_pEditorViewportRenderTarget->getSize());
    m_pEditorViewport->render(pCmdList, m_pEditorViewportRenderTarget);

    if (m_pGameViewportRenderTarget->getSize() != m_gameViewportSize) {
      bfc::graphics::loadTexture2D(pCmdList, &m_pGameViewportColour, m_gameViewportSize, PixelFormat_RGBAf16);
      bfc::graphics::loadTexture2D(pCmdList, &m_pGameViewportDepth, m_gameViewportSize, DepthStencilFormat_D24S8);
      m_pGameViewportRenderTarget->attachColour(m_pGameViewportColour);
      m_pGameViewportRenderTarget->attachDepth(m_pGameViewportDepth);
    }

    m_pGameViewport->setSize(pCmdList, m_pGameViewportRenderTarget->getSize());
    m_pGameViewport->render(pCmdList, m_pGameViewportRenderTarget);

    pCmdList->bindRenderTarget(renderTarget);

    if (m_pDrawData != nullptr) {
      m_uiContext.renderDrawData(pCmdList, m_pDrawData);
      m_pDrawData = nullptr;
    }
  }

  void LevelEditor::drawUI(bfc::Ref<LevelManager> const & pLevels) {
    Ref<Level> pLevel = pLevels->getActiveLevel();

    ImGuiID dockspaceId = ImGui::GetID("main-dockspace");
    ImGui::DockSpaceOverViewport(dockspaceId);

    drawLevelPanel(pLevels, pLevel);
    drawEntityProperties(pLevel, m_selected);
    drawEditorSettings();
    drawAssetsPanel(pLevels);
    drawEditorViewportPanel(pLevel);
    drawGameViewportPanel(pLevel);
  }

  void LevelEditor::drawViewportGizmo(bfc::Ref<Level> const & pLevel, EntityID entityID, ImVec2 vpMin, ImVec2 vpMax) {
    auto *pTransform = pLevel->tryGet<components::Transform>(entityID);
    if (pTransform == nullptr || m_pActiveViewport != m_pEditorViewport)
      return;

    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(vpMin.x, vpMin.y, vpMax.x - vpMin.x, vpMax.y - vpMin.y);

    bfc::Mat4 transform = pTransform->globalTransform(pLevel.get());

    if (m_pEditorViewport->manipulate(&transform, m_manipulator.op, m_manipulator.mode))
      pTransform->setGlobalTransform(pLevel.get(), transform);
  }

  void LevelEditor::drawEditorViewportPanel(bfc::Ref<Level> const & pLevel) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Scene");
    m_editorViewportSize = ImGui::GetContentRegionAvail();
    if (m_editorViewportSize.x <= 0) m_editorViewportSize.x = 1;
    if (m_editorViewportSize.y <= 0) m_editorViewportSize.y = 1;
    ImGui::Image(m_pEditorViewportRenderTarget->getColour(0).texture, m_editorViewportSize, ImVec2(0, 1), ImVec2(1, 0));

    m_editorViewportWantsInput = ImGui::IsWindowHovered();

    ImVec2 vpMin = ImGui::GetItemRectMin();
    ImVec2 vpMax = ImGui::GetItemRectMax();

    drawViewportGizmo(pLevel, m_selected, vpMin, vpMax);

    ImGui::End();
    ImGui::PopStyleVar();
  }

  void LevelEditor::drawGameViewportPanel(bfc::Ref<Level> const & pLevel) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Game");
    m_gameViewportSize = ImGui::GetContentRegionAvail();
    if (m_gameViewportSize.x <= 0)
      m_gameViewportSize.x = 1;
    if (m_gameViewportSize.y <= 0)
      m_gameViewportSize.y = 1;
    ImGui::Image(m_pGameViewportRenderTarget->getColour(0).texture, m_gameViewportSize, ImVec2(0, 1), ImVec2(1, 0));
    m_gameViewportWantsInput = ImGui::IsWindowHovered();
    ImGui::End();
    ImGui::PopStyleVar();
  }

  void LevelEditor::drawAssetsPanel(Ref<LevelManager> const & pLevels) {
    ImGui::Begin("Assets");
    for (String const & drive : fs::drives()) {
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

    for (URI const & item : fs::walk(m_selectedAssetPath)) {
      bool isLeaf          = fs::isLeaf(item);
      bool isClicked       = ImGui::Selectable(String(item.path().name()).c_str());
      bool isDoubleClicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

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

  void LevelEditor::drawLevelPanel(Ref<LevelManager> const & pLevels,
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

      if (activateEditorViewport) {
        m_pEditorViewport->setLevel(pLevels->getActiveLevel());
        activateViewport(m_pEditorViewport);
      } else if (activateGameViewport) {
        auto pInitCmdList = rendering::createCommandList();
        pInitCmdList->setDebugName("LevelEditor init game viewport");
        auto pRenderer    = bfc::NewRef<DeferredRenderer>(pInitCmdList.get());
        rendering::registerRenderer(pRenderer);

        auto pGameViewport = NewRef<GameViewport>(pRenderer);
        rendering::getDevice()->submit(std::move(pInitCmdList));

        pGameViewport->setLevel(pLevels->getActiveLevel());
        activateViewport(pGameViewport);
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
        pLevels->setActiveLevel(pLevels->createLevel());
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
        dialog.setFilter({"level"}, {"Orbital Game Level"});
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

      ImGui::Separator();

      if (ImGui::Selectable("Translate", (m_manipulator.op & ImGuizmo::OPERATION::TRANSLATE) > 0)) {
        m_manipulator.op = ImGuizmo::OPERATION::TRANSLATE;
      }

      if (ImGui::Selectable("Rotate", (m_manipulator.op & ImGuizmo::OPERATION::ROTATE) > 0)) {
        m_manipulator.op = ImGuizmo::OPERATION::ROTATE;
      }

      if (ImGui::Selectable("Scale", (m_manipulator.op & ImGuizmo::OPERATION::SCALE) > 0)) {
        m_manipulator.op = ImGuizmo::OPERATION::SCALE;
      }

      if (ImGui::Selectable("Local Space", m_manipulator.mode == ImGuizmo::MODE::LOCAL)) {
        m_manipulator.mode = m_manipulator.mode == ImGuizmo::MODE::LOCAL ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL;
      }

      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();

    for (EntityID entityID : pLevel->entities()) {
      if (pLevel->has<components::Transform>(entityID)) {
        continue;
      }

      auto * pName = pLevel->tryGet<components::Name>(entityID);

      ImGui::PushID((int)((uint64_t)entityID & 0x00000000FFFFFFFF));
      ImGui::PushID((int)(((uint64_t)entityID >> 32) & 0x00000000FFFFFFFF));

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

    ImGui::PushID((int)((uint64_t)entityID & 0x00000000FFFFFFFF));
    ImGui::PushID((int)(((uint64_t)entityID >> 32) & 0x00000000FFFFFFFF));

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
      RuntimeObject component = pStorage->getRuntimeInterface(entityID);
      if (component.isEmpty())
        continue;

      String typeName = ILevelComponentType::findName(type);
      if (typeName.length() == 0) {
        continue;
      }

      ImGui::PushID(typeName.c_str());

      bool visible = true;
      if (ImGui::CollapsingHeader(typeName.c_str(), &visible)) {
        ImGui::Indent();
        drawPropertyEditor(pLevel, entityID, component);
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

  void LevelEditor::drawPropertyEditor(bfc::Ref<Level> const & pLevel, EntityID entityID,
                                       bfc::RuntimeObject const & instance) {
    if (instance.isEmpty())
      return;

    Ref<IPropertyEditor> pEditor = m_propertyEditors.getOr(instance.typeInfo(), nullptr);
    if (pEditor != nullptr) {
      pEditor->_draw(this, pLevel, entityID, instance.data());
    } else {
      for (auto & member : instance.members()) {
        ImGui::Text("%s", member.c_str());
        drawPropertyEditor(pLevel, entityID, instance.get(member));
      }
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
  void LevelEditor::LevelEditorRenderingPlugin::onFrame(bfc::graphics::CommandList *   pCmdList,
                                                        bfc::platform::Window *        pWindow,
                                                        bfc::graphics::RenderTargetRef renderTarget) {
    m_pEditor->onRenderFrame(pCmdList, renderTarget);
  }
} // namespace engine
