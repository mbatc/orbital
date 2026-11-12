
#include "Editor/LevelEditor.h"
#include "Behaviours.h"

namespace engine {
  class BehavioursEditor : public LevelEditor::ComponentEditor<components::Behaviours> {
  public:
    virtual void draw(LevelEditor * pEditor, bfc::Ref<Level> const & pLevel, EntityID entityID,
                      components::Behaviours * pScripts) override;
  };
} // namespace engine
