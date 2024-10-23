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
  LevelEditor::LevelEditor()
    : Subsystem(TypeID<LevelEditor>(), "LevelEditor") {}

  bool LevelEditor::init(Application * pApp) {
    m_pAssets    = pApp->findSubsystem<AssetManager>();
    m_pRendering = pApp->findSubsystem<Rendering>();
    m_pLevels    = pApp->findSubsystem<LevelManager>();

    Ref<Input> pInputs = pApp->findSubsystem<Input>();

    settings.startupLevel = pApp->addSetting("level-manager/startup-level", URI::File("game:levels/main.level"));


    // Load the startup level
    URI levelPath = settings.startupLevel.get();
    if (!m_pAssets->getFileSystem()->exists(levelPath)) {
      auto pNewLevel = m_pLevels->getActiveLevel();

      Filename skyboxPath = "game:skybox/nebula.skybox";
      EntityID testEntity = pNewLevel->create();

      pNewLevel->add<components::Name>(testEntity).name       = "Environment";
      pNewLevel->add<components::Skybox>(testEntity).pTexture = m_pAssets->load<bfc::Texture>(URI::File(skyboxPath));

      components::Transform & sunTransform = pNewLevel->add<components::Transform>(testEntity);
      sunTransform.lookAt(bfc::Vec3d(1, 1, -1));

      components::Light & sun = pNewLevel->add<components::Light>(testEntity);
      sun.ambient             = {0.7f, 0.7f, 0.7f};
      sun.colour              = {0.7f, 0.7f, 0.7f};
      sun.castShadows         = true;

      m_pLevels->save(levelPath, *pNewLevel);
    }

    m_pLevels->setActiveLevel(m_pLevels->load(settings.startupLevel.get()));
    
    // Create an editor viewport and render the active level.
    m_pEditorViewport = NewRef<LevelEditorViewport>(m_pRendering->getDevice(), m_pAssets.get());
    m_pEditorViewport->setLevel(m_pLevels->getActiveLevel());

    m_pViewportListener = m_pEditorViewport->getEvents()->addListener();
    m_pViewportListener->on([=](bfc::events::DroppedFiles const & e) {
      for (Filename const & file : e.files) {
        m_pLevels->Import(m_pLevels->getActiveLevel().get(), URI::File(file));
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
    m_pRendering->setMainViewport(m_pEditorViewport);

    // Init ui context rendering.
    m_uiContext.init(m_pRendering->getDevice());
    m_uiContext.getEvents()->listenTo(m_pRendering->getMainWindow()->getEvents());

    registerComponentEditors();

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
        URI levelPath = settings.startupLevel.get();

        BFC_LOG_INFO("LevelEditor", "Saving level to %s", levelPath);
        LevelSerializer(m_pAssets.get()).serialize(levelPath, *m_pLevels->getActiveLevel());
      }
    }
  }

  bool LevelEditor::drawEntitySelector(bfc::StringView const & name, EntityID * pEntityID, Level * pLevel) {
    bfc::String selectedName = "";
    if (*pEntityID != InvalidEntity) {
      if (components::Name * pName = pLevel->tryGet<components::Name>(*pEntityID))
        selectedName = pName->name;
      else
        selectedName = "[ unnamed ]";
    }

    ui::Input(name, &selectedName);
    if (ImGui::IsItemClicked())
      ImGui::OpenPopup("Select Entity");

    if (ImGui::BeginPopup("Select Entity")) {
      for (EntityID entity : pLevel->entities()) {
        ImGui::PushID((int)(entity & 0x00000000FFFFFFFF));
        ImGui::PushID((int)((entity >> 32) & 0x00000000FFFFFFFF));

        bfc::String optionName = "[ unnamed ]";
        if (components::Name * pName = pLevel->tryGet<components::Name>(entity))
          optionName = pName->name;

        if (ImGui::Selectable(optionName.c_str(), entity == *pEntityID))
          *pEntityID = entity;

        ImGui::PopID();
        ImGui::PopID();
      }

      ImGui::EndPopup();
    }

    return false;
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

    if (ImGui::BeginMenu("File")) {
      if (ImGui::Selectable("Load Startup Level")) {
        BFC_LOG_INFO("LevelManager", "Reloading level");

        Ref<Level> pLoaded = pLevels->load(settings.startupLevel.get());
        if (pLoaded != nullptr) {
          pLevel->clear();
          pLoaded->copyTo(pLevel.get(), true);
        }
      }

      // if (ImGui::Selectable("Save Level")) {
      // 
      // }

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
      if (ImGui::Selectable("Copy Selected")) {
        m_selected = pLevel->copy(m_selected);
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
      drawEntityComponentProperties(pLevel, m_selected);
      ImGui::End();
    }
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

  class NameEditor : public LevelEditor::ComponentEditor<components::Name> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::Name * pComponent) override {
      bfc::ui::Input("Name", &pComponent->name);
    }
  };

  class TransformEditor : public LevelEditor::ComponentEditor<components::Transform> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::Transform * pTransform) override {
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
  };

  class CameraEditor : public LevelEditor::ComponentEditor<components::Camera> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, components::Camera * pCamera) override {
      bfc::ui::Input("Near Plane", &pCamera->nearPlane);
      bfc::ui::Input("Far Plane", &pCamera->farPlane);

      float fovDegs = glm::degrees(pCamera->fov);
      bfc::ui::Input("Field of View", &fovDegs);
      pCamera->fov = glm::radians(fovDegs);

      bfc::ui::Input("Viewport Position", &pCamera->viewportPosition);
      bfc::ui::Input("Viewport Size", &pCamera->viewportSize);
    }
  };

  void LevelEditor::registerComponentEditors() {
    addComponentEditor<TransformEditor>();
    addComponentEditor<CameraEditor>();
    addComponentEditor<NameEditor>();
  }
} // namespace engine
