#pragma once

#include "../Renderer/DeferredRenderer.h"
#include "../Subsystem.h"

namespace bfc {
  class Mesh;
  class Material;
} // namespace bfc

namespace engine {
  class Rendering;
  class Level;

  class LevelManager : public Subsystem {
  public:
    LevelManager();

    virtual bool init(Application * pApp);
    virtual void shutdown() override;
    virtual void loop();

  private:
    // Register the game component types.
    void registerComponentTypes();

    bfc::Ref<Level> m_pActiveLevel = nullptr;
    bfc::Ref<DeferredRenderer> m_pRenderer;
    bfc::Ref<bfc::EventListener>    m_pInputs    = nullptr;
    Rendering * m_pRendering = nullptr;
  };
} // namespace engine
