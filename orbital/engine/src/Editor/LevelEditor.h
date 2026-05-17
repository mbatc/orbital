#pragma once

#include "Levels/LevelComponents.h"
#include "Subsystem.h"
#include "ui/Context.h"
#include "util/Settings.h"

namespace bfc {
  // class EventListener;
}

namespace engine {
  class EditorCamera;
  class Rendering;
  class Level;
  class LevelEditorViewport;
  class LevelManager;
  class VirtualFileSystem;

  class LevelEditor : public Subsystem {
  public:
    struct {
      bfc::Setting<bfc::URI> startupLevel;

      struct {
        bfc::Setting<float> fov;
        bfc::Setting<float> nearPlane;
        bfc::Setting<float> farPlane;
      } camera;
    } settings;

    LevelEditor();

    virtual bool init(Application * pApp) override;
    virtual void shutdown() override;
    virtual void loop(Application * pApp) override;

    // Component editors
    class IComponentEditor {
    public:
      virtual bfc::type_index type()                                                                                                   = 0;
      virtual void            _draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, void const * pComponent) = 0;
    };

    template<typename T>
    class ComponentEditor : public IComponentEditor {
    public:
      virtual bfc::type_index type() override final {
        return bfc::TypeID<T>();
      }

      virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, T * pComponent) = 0;

    private:
      virtual void _draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID, void const * pComponent) override final {
        draw(pEditor, pLevel, entityID, (T *)pComponent);
      }
    };

    template<typename Editor, typename... Args>
    void addComponentEditor(Args &&... args) {
      bfc::Ref<IComponentEditor> pEditor = bfc::NewRef<Editor>(std::forward<Args>(args)...);
      m_componentEditors.add(pEditor->type(), pEditor);
    }

    static bool drawEntitySelector(bfc::StringView const & name, EntityID * pEntityID, Level * pLevel);

    template<typename T>
    static bool drawAssetSelector(bfc::StringView const & name, bfc::Ref<T> * ppAsset, AssetManager * pManager, VirtualFileSystem * pFileSystem) {
      bfc::Ref<void> pOpaque = *ppAsset;
      bool           result  = drawAssetSelector(name, &pOpaque, bfc::TypeID<T>(), pManager, pFileSystem);
      *ppAsset               = std::static_pointer_cast<T>(pOpaque);
      return result;
    }

    template<typename T>
    static bool drawAssetSelector(bfc::StringView const & name, engine::Asset<T> * pAsset, AssetManager * pManager, VirtualFileSystem * pFileSystem) {
      AssetHandle handle = pAsset->handle();
      bool        result = false;
      if (handle == InvalidAssetHandle && pAsset->instance() != nullptr) {
        bfc::Ref<T> pInstance = pAsset->instance();
        result                = drawAssetSelector(name, &pInstance, pManager, pFileSystem);
        pAsset->assign(pManager, pInstance);
      } else {
        result = drawAssetSelector(name, &handle, bfc::TypeID<T>(), pManager, pFileSystem);
        pAsset->assign(pManager, handle);
      }
      return result;
    }

    static bool drawAssetSelector(bfc::StringView const & name, bfc::Ref<void> * ppAsset, bfc::type_index const & assetType, AssetManager * pManager,
                                  VirtualFileSystem * pFileSystem);
    static bool drawAssetSelector(bfc::StringView const & name, AssetHandle * pHandle, bfc::type_index const & assetType, AssetManager * pManager,
                                  VirtualFileSystem * pFileSystem,
                                  bfc::StringView const & emptyPreview = "[ None ]");

  private:
    void drawUI(bfc::Ref<LevelManager> const & pLevels, bfc::Ref<AssetManager> const & pAssets, bfc::Ref<Rendering> const & pRendering,
                bfc::Ref<VirtualFileSystem> const & pFileSystem);

    void drawAssetsPanel(bfc::Ref<VirtualFileSystem> const & pFileSystem, bfc::Ref<LevelManager> const & pLevels);
    void drawLevelPanel(bfc::Ref<LevelManager> const & pLevels, bfc::Ref<AssetManager> const & pAssets, bfc::Ref<Rendering> const & pRendering,
                        bfc::Ref<Level> const & pLevel);
    void drawEntityProperties(bfc::Ref<Level> const & pLevel, EntityID entityID);
    void drawEditorSettings();
    void drawCameraProperties(EditorCamera * pCamera);

    void drawTransformTree(bfc::Ref<Level> const & pLevel, EntityID entityID);
    void drawEntityComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID);
    void drawAddComponentMenu(bfc::Ref<Level> const & pLevel, EntityID targetEntityID);

    bfc::Ref<bfc::EventListener> m_pViewportListener;
    bfc::Ref<bfc::EventListener> m_pAppListener;

    bfc::Ref<LevelEditorViewport> m_pEditorViewport;

    bfc::Map<bfc::type_index, bfc::Ref<IComponentEditor>> m_componentEditors;

    bfc::URI m_selectedAssetPath;

    EntityID m_selected = InvalidEntity;

    bfc::ui::Context m_uiContext;
    ImDrawData *     m_pDrawData = nullptr;
  };
} // namespace engine
