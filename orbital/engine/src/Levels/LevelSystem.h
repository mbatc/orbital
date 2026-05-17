#pragma once

#include "core/Core.h"
#include "core/Timestamp.h"

namespace engine {
  class Level;

  class ILevelActivate {
  public:
    virtual void activate(Level * pLevel) = 0;
  };

  class ILevelDeactivate {
  public:
    virtual void deactivate(Level * pLevel) = 0;
  };

  class ILevelPlay {
  public:
    virtual void play(Level * pLevel) = 0;
  };

  class ILevelPause {
  public:
    virtual void pause(Level * pLevel) = 0;
  };

  class ILevelStop {
  public:
    virtual void stop(Level * pLevel) = 0;
  };

  class ILevelUpdate {
  public:
    virtual void update(Level * pLevel, bfc::Timestamp dt) = 0;
  };

  class RenderView;
  class RenderView;
  class ILevelRenderDataCollector {
  public:
    virtual void collectRenderData(RenderView * pRenderView, Level const * pLevel) = 0;
  };

  void registerLevelActivate(bfc::Ref<ILevelActivate> const & pActivator);
  void registerLevelDeactivate(bfc::Ref<ILevelDeactivate> const & pDeactivator);
  void registerLevelPlay(bfc::Ref<ILevelPlay> const & pPlayer);
  void registerLevelPause(bfc::Ref<ILevelPause> const & pPauser);
  void registerLevelStop(bfc::Ref<ILevelStop> const & pStopper);
  void registerLevelUpdate(bfc::Ref<ILevelUpdate> const & pUpdater);
  void registerLevelRenderDataCollector(bfc::Ref<ILevelRenderDataCollector> const & pCollector);

  template<typename T, typename... Args>
  void registerLevelSystem(Args &&... args) {
    bfc::Ref<T> pSystem = bfc::NewRef<T>(std::forward<Args>(args)...);

    if constexpr (std::is_base_of_v<ILevelActivate, T>)
      registerLevelActivate(pSystem);

    if constexpr (std::is_base_of_v<ILevelDeactivate, T>)
      registerLevelDeactivate(pSystem);

    if constexpr (std::is_base_of_v<ILevelPlay, T>)
      registerLevelPlay(pSystem);

    if constexpr (std::is_base_of_v<ILevelPause, T>)
      registerLevelPause(pSystem);

    if constexpr (std::is_base_of_v<ILevelStop, T>)
      registerLevelStop(pSystem);

    if constexpr (std::is_base_of_v<ILevelUpdate, T>)
      registerLevelUpdate(pSystem);

    if constexpr (std::is_base_of_v<ILevelRenderDataCollector, T>)
      registerLevelRenderDataCollector(pSystem);
  }

  void playLevel(Level * pLevel);
  void pauseLevel(Level * pLevel);
  void stopLevel(Level * pLevel);
  void updateLevel(Level * pLevel, bfc::Timestamp dt);
  void activateLevel(Level * pLevel);
  void deactivateLevel(Level * pLevel);

  void collectRenderData(RenderView * pRenderView, Level const * pLevel);
}
