#include "LevelEditor.h"
#include "Application.h"
#include "Input.h"
#include "Assets/AssetManager.h"
#include "Levels/CoreComponents.h"
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
  static void drawComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID, const void * pComponent);

  template<typename T>
  void tryDrawComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID);

  LevelEditor::LevelEditor()
    : Subsystem(TypeID<LevelEditor>(), "LevelEditor") {}

  bool LevelEditor::init(Application * pApp) {
    Ref<Rendering>    pRendering = pApp->findSubsystem<Rendering>();
    Ref<Input>        pInputs    = pApp->findSubsystem<Input>();
    Ref<LevelManager> pLevels    = pApp->findSubsystem<LevelManager>();
    Ref<AssetManager> pAssets    = pApp->findSubsystem<AssetManager>();

    // Create an editor viewport and render the active level.
    m_pEditorViewport = NewRef<LevelEditorViewport>(pRendering->getDevice(), pAssets.get());
    m_pEditorViewport->setLevel(pLevels->getActiveLevel());

    m_pViewportListener = m_pEditorViewport->getEvents()->addListener();
    m_pViewportListener->on([pLevels](bfc::events::DroppedFiles const & e) {
      for (Filename const & file : e.files) {
        pLevels->Import(pLevels->getActiveLevel().get(), URI::File(file));
      }
    });

    m_pAppListener = pApp->addListener();

    m_pAppListener->on([=](events::OnRenderViewport e) {
      if (m_pDrawData != nullptr && e.isMainViewport) {
        m_uiContext.renderDrawData(m_pDrawData);
        m_pDrawData = nullptr;
      }
    });

    // Render the editor viewport to the main window.
    pRendering->setMainViewport(m_pEditorViewport);

    // Init ui context rendering.
    m_uiContext.init(pRendering->getDevice());
    m_uiContext.getEvents()->listenTo(pRendering->getMainWindow()->getEvents());

    return true;
  }

  void LevelEditor::shutdown() {
    m_pEditorViewport = nullptr;
  }

  void LevelEditor::loop(Application * pApp) {
    m_pRendering = pApp->findSubsystem<Rendering>();
    m_pAssets    = pApp->findSubsystem<AssetManager>();
    m_pLevels    = pApp->findSubsystem<LevelManager>();

    m_uiContext.beginFrame(m_pRendering->getMainWindow()->getSize());

    drawUI(m_pLevels);

    ImGui::Render();
    m_pDrawData = ImGui::GetDrawData();

    if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard) {
      m_pEditorViewport->getEvents()->stopListening(m_pRendering->getMainWindow()->getEvents());
    } else {
      m_pEditorViewport->getEvents()->listenTo(m_pRendering->getMainWindow()->getEvents());
    }

    // Apply camera controls
    m_pEditorViewport->camera.update(pApp->getDeltaTime());

    Keyboard & kbd = m_pEditorViewport->getKeyboard();

    if (kbd.isDown(KeyCode_Control)) {
      if (kbd.isPressed(KeyCode_S)) {
        URI levelPath = m_pLevels->settings.startupLevel.get();

        BFC_LOG_INFO("LevelEditor", "Saving level to %s", levelPath);
        LevelSerializer(m_pAssets.get()).serialize(levelPath, *m_pLevels->getActiveLevel());
      }
    }
  }

  void LevelEditor::drawUI(Ref<LevelManager> const & pLevels) {
    Ref<Level> pLevel = pLevels->getActiveLevel();

    ImGui::Begin("Editor UI", 0, ImGuiWindowFlags_MenuBar);
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
      bool activateGameViewport = pLevels->getSimulateState() == SimulateState_Stopped;
      bool activateEditorViewport = desiredState == SimulateState_Stopped;

      pLevels->setSimulateState(desiredState);

      for (auto [name, pDevice] : m_pRendering->getMainViewport()->getInputDevices()) {
        pLevels->getApp()->findSubsystem<Input>()->setInputDevice(name, nullptr);
      }

      if (activateEditorViewport) {
        m_pEditorViewport->setLevel(pLevels->getActiveLevel());
        m_pRendering->setMainViewport(m_pEditorViewport);
      } else if (activateGameViewport) {
        auto pGameViewport = NewRef<GameViewport>(m_pRendering->getDevice(), m_pAssets.get());
        pGameViewport->setLevel(pLevels->getActiveLevel());
        m_pRendering->setMainViewport(pGameViewport);
      }

      for (auto [name, pDevice] : m_pRendering->getMainViewport()->getInputDevices()) {
        pLevels->getApp()->findSubsystem<Input>()->setInputDevice(name, pDevice);
      }
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
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();

    for (EntityID entityID : pLevel->entities()) {
      if (pLevel->has<components::Transform>(entityID)) {
        continue;
      }

      auto * pName = pLevel->tryGet<components::Name>(entityID);

      if (ImGui::Selectable(pName ? pName->name.c_str() : "[ Unnamed ]", m_selected == entityID, ImGuiSelectableFlags_SpanAvailWidth)) {
        if (m_selected == entityID)
          m_selected = InvalidEntity;
        else
          m_selected = entityID;
      }
      
    }

    ImGui::Separator();
    bfc::Vector<EntityID> rootEntities;
    for (auto & [component] : pLevel->getView<components::Transform>()) {
      if (component.parent() == InvalidEntity) {
        rootEntities.pushBack(pLevel->toEntity(&component));
      }
    }

    for (EntityID id : rootEntities) {
      drawTransformTree(pLevel, id);
    }

    ImGui::End();

    if (m_selected != InvalidEntity) {
      ImGui::Begin("Properties");
      if (ImGui::BeginMenu("Add Component")) {
        drawAddComponentMenu(pLevel, m_selected);
        ImGui::EndMenu();
      }
      drawComponentProperties(pLevel, m_selected);
      ImGui::End();
    }
  }

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

    if (open) {
      for (EntityID child : transform.children()) {
        drawTransformTree(pLevel, child);
      }

      ImGui::TreePop();
    }
    ImGui::PopID();
    ImGui::PopID();
  }

  void LevelEditor::drawComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID) {
    tryDrawComponentProperties<components::Name>(pLevel, entityID);
    tryDrawComponentProperties<components::Transform>(pLevel, entityID);
    tryDrawComponentProperties<components::Camera>(pLevel, entityID);
    tryDrawComponentProperties<components::StaticMesh>(pLevel, entityID);
    tryDrawComponentProperties<components::Light>(pLevel, entityID);
    tryDrawComponentProperties<components::Skybox>(pLevel, entityID);
    tryDrawComponentProperties<components::PostProcessVolume>(pLevel, entityID);
    tryDrawComponentProperties<components::PostProcess_Tonemap>(pLevel, entityID);
    tryDrawComponentProperties<components::PostProcess_SSAO>(pLevel, entityID);
    tryDrawComponentProperties<components::PostProcess_SSR>(pLevel, entityID);
    tryDrawComponentProperties<components::PostProcess_Bloom>(pLevel, entityID);
  }

  void LevelEditor::drawAddComponentMenu(bfc::Ref<Level> const & pLevel, EntityID targetEntityID) {
    for (auto & name : ILevelComponentType::names()) {
      Ref<ILevelComponentType>    pInterface  = ILevelComponentType::find(name);
      Ref<ILevelComponentStorage> pComponents = pLevel->components().getOr(pInterface->type(), nullptr);

      if (pComponents != nullptr && pComponents->exists(targetEntityID))
        continue;

      if (ImGui::Selectable(name.c_str())) {
        LevelSerializer serializer(m_pAssets.get());
        pInterface->read(&serializer, SerializedObject::Empty(), *pLevel, targetEntityID);
      }
    }
  }

  void drawComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID, const void * pComponent) {
    ImGui::Text("Not implemented");
  }

  template<typename T>
  void tryDrawComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID) {
    String typeName = ILevelComponentType::findName(bfc::TypeID<T>());

    ImGui::PushID(typeName.c_str());
    if (auto * pComponent = pLevel->tryGet<T>(entityID)) {
      bool visible = true;
      if (ImGui::CollapsingHeader(typeName.c_str(), &visible)) {
        drawComponentProperties(pLevel, entityID, pComponent);
      }

      if (!visible)
        ImGui::OpenPopup("Confirm Remove Component?");
    }

    if (ImGui::BeginPopupModal("Confirm Remove Component?")) {
      ImGui::Text("Are you sure you want to remove this component");
      if (ImGui::Button("Cancel"))
        ImGui::CloseCurrentPopup();

      if (ImGui::Button("Yes")) {
        pLevel->erase<T>(entityID);
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::PopID();
  }

  void drawComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID, components::Name * pComponent) {
    bfc::ui::Input("Name", &pComponent->name);
  }

  void drawComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID, components::Transform * pTransform) {
    Vec3d translation = pTransform->translation();
    Vec3d scale       = pTransform->scale();
    Vec3d ypr         = pTransform->ypr();

    bfc::ui::Input("Translation", &translation);
    bfc::ui::Input("Scale", &scale);
    bfc::ui::Input("Yaw/Pitch/Roll", &ypr);

    pTransform->setTranslation(translation);
    pTransform->setScale(scale);
    pTransform->setYpr(ypr);
  }

  void drawComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID, components::Camera * pCamera) {
    bfc::ui::Input("Near Plane", &pCamera->nearPlane);
    bfc::ui::Input("Far Plane", &pCamera->farPlane);

    float fovDegs = glm::degrees(pCamera->fov);
    bfc::ui::Input("Field of View", &fovDegs);
    pCamera->fov = glm::radians(fovDegs);

    bfc::ui::Input("Viewport Position", &pCamera->viewportPosition);
    bfc::ui::Input("Viewport Size", &pCamera->viewportSize);
  }
} // namespace engine
