#pragma once

#include "DeferredRenderer/DeferredRenderer.h"
#include "Subsystem.h"

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
    virtual void loop();

  private:
    bfc::Ref<Level> m_pActiveLevel = nullptr;

    bfc::Ref<DeferredRenderer> m_pRenderer;
    bfc::Ref<bfc::Mesh>        m_pMesh;

    bfc::Vector<bfc::Ref<bfc::Material>> m_materials;

    Rendering * m_pRendering = nullptr;
  };
} // namespace engine
