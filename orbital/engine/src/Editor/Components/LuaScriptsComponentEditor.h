
#include "Editor/LevelEditor.h"
#include "LuaScripting.h"

namespace engine {
  class LuaScriptsEditor : public LevelEditor::ComponentEditor<components::LuaScripts> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                      components::LuaScripts * pScripts) override;
  };
} // namespace engine
