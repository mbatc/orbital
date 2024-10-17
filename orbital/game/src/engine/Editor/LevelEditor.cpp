#include "LevelEditor.h"
#include "Application.h"
#include "Viewport/LevelEditorViewport.h"
#include "Rendering/Rendering.h"
#include "Levels/Level.h"
#include "Levels/LevelManager.h"
#include "Levels/LevelSerializer.h"
#include "Assets/AssetManager.h"
#include "util/Log.h"

#include "platform/Events.h"
#include "platform/Window.h"

using namespace bfc;

namespace engine {
  LevelEditor::LevelEditor()
    : Subsystem(TypeID<LevelEditor>(), "LevelEditor") {}

  bool LevelEditor::init(Application * pApp) {
    Ref<Rendering>    pRendering = pApp->findSubsystem<Rendering>();
    Ref<LevelManager> pLevels    = pApp->findSubsystem<LevelManager>();
    Ref<AssetManager> pAssets    = pApp->findSubsystem<AssetManager>();

    // Create an editor viewport and render the active level.
    m_pViewport = NewRef<LevelEditorViewport>(pRendering->getDevice(), pAssets.get());
    m_pViewport->setLevel(pLevels->getActiveLevel());

    m_pViewportListener = m_pViewport->getEvents()->addListener();
    m_pViewportListener->on<events::DroppedFiles>([pLevels](events::DroppedFiles const & e) {
      for (Filename const& file : e.files) {
        pLevels->Import(pLevels->getActiveLevel().get(), URI::File(file));
      }
      return true;
    });

    // Render the editor viewport to the main window.
    pRendering->setMainViewport(m_pViewport);

    return true;
  }

  void LevelEditor::shutdown() {
    m_pViewport = nullptr;
  }

  void LevelEditor::loop(Application * pApp) {
    Ref<LevelManager> pLevels = pApp->findSubsystem<LevelManager>();
    Ref<AssetManager> pAssets = pApp->findSubsystem<AssetManager>();
    // Apply camera controls
    m_pViewport->camera.update(pApp->getDeltaTime());

    Keyboard & kbd = m_pViewport->getKeyboard();

    if (kbd.isDown(KeyCode_Control)) {
      if (kbd.isPressed(KeyCode_S)) {
        URI levelPath = pLevels->settings.startupLevel.get();

        BFC_LOG_INFO("LevelEditor", "Saving level to %s", levelPath);
        LevelSerializer(pAssets.get()).serialize(levelPath, *pLevels->getActiveLevel());
      }

      if (kbd.isPressed(KeyCode_P)) {
        if (m_pEditorLevel) {
          BFC_LOG_INFO("LevelEditor", "Stop playing. Restoring editor level.");
          pLevels->setActiveLevel(m_pEditorLevel);
          m_pEditorLevel = nullptr;
        } else {
          BFC_LOG_INFO("LevelEditor", "Start playing. Backing-up editor level.");
          m_pEditorLevel           = pLevels->getActiveLevel();
          Ref<Level> pPlayingLevel = NewRef<Level>();
          m_pEditorLevel->copyTo(pPlayingLevel.get(), true);

          pLevels->setActiveLevel(pPlayingLevel);
        }

        m_pViewport->setLevel(pLevels->getActiveLevel());
      }
    }

    m_pViewport->getKeyboard().update(pApp->getDeltaTime());
    m_pViewport->getMouse().update(pApp->getDeltaTime());
  }
} // namespace engine
