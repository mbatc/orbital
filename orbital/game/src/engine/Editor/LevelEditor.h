#pragma once

#include "util/Settings.h"
#include "ui/Context.h"
#include "Subsystem.h"
#include "Levels/LevelComponents.h"

namespace bfc {
  // class EventListener;
}

namespace engine {
  class Rendering;
  class Level;
  class LevelEditorViewport;
  class LevelManager;
  class LevelEditor : public Subsystem {
  public:
    struct {
      bfc::Setting<bfc::URI> startupLevel;
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
    void addComponentEditor(Args&&... args) {
      bfc::Ref<IComponentEditor> pEditor = bfc::NewRef<Editor>(std::forward<Args>(args)...);
      m_componentEditors.add(pEditor->type(), pEditor);
    }

    static bool drawEntitySelector(bfc::StringView const & name, EntityID *pEntityID, Level *pLevel);

  private:
    void drawUI(bfc::Ref<LevelManager> const & pLevels);
    void drawTransformTree(bfc::Ref<Level> const & pLevel, EntityID entityID);
    void drawEntityComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID);
    void drawAddComponentMenu(bfc::Ref<Level> const & pLevel, EntityID targetEntityID);

    void registerComponentEditors();

    bfc::Ref<bfc::EventListener>  m_pViewportListener;
    bfc::Ref<bfc::EventListener>  m_pAppListener;

    bfc::Ref<LevelEditorViewport> m_pEditorViewport;
    bfc::Ref<LevelManager>        m_pLevels    = nullptr;
    bfc::Ref<AssetManager>        m_pAssets    = nullptr;
    bfc::Ref<Rendering>           m_pRendering = nullptr;

    bfc::Map<bfc::type_index, bfc::Ref<IComponentEditor>> m_componentEditors;

    EntityID m_selected = InvalidEntity;

    bfc::ui::Context m_uiContext;
    ImDrawData *     m_pDrawData = nullptr;
  };
}
