#include "LevelSystem.h"
#include "Level.h"
#include "core/Vector.h"

namespace engine {
  struct {
    // Scene behaviour extensions
    bfc::Vector<bfc::Ref<ILevelCreated>>    creators;
    bfc::Vector<bfc::Ref<ILevelDestroyed>>  destroyers;
    bfc::Vector<bfc::Ref<ILevelActivate>>   activators;
    bfc::Vector<bfc::Ref<ILevelDeactivate>> deactivators;
    bfc::Vector<bfc::Ref<ILevelPlay>>       players;
    bfc::Vector<bfc::Ref<ILevelPause>>      pausers;
    bfc::Vector<bfc::Ref<ILevelStop>>       stoppers;
    bfc::Vector<bfc::Ref<ILevelUpdate>>     updaters;

    // Rendering extensions
    bfc::Vector<bfc::Ref<ILevelRenderDataCollector>> renderDataCollectors;
  } static s_systems;

  void registerLevelCreated(bfc::Ref<ILevelCreated> const & pCreator) {
    if (!s_systems.creators.contains(pCreator))
      s_systems.creators.pushBack(pCreator);
  }

  void registerLevelDestroyed(bfc::Ref<ILevelDestroyed> const & pDestroyer) {
    if (!s_systems.destroyers.contains(pDestroyer))
      s_systems.destroyers.pushBack(pDestroyer);
  }

  void registerLevelActivate(bfc::Ref<ILevelActivate> const & pActivator) {
    if (!s_systems.activators.contains(pActivator))
      s_systems.activators.pushBack(pActivator);
  }

  void registerLevelDeactivate(bfc::Ref<ILevelDeactivate> const & pDeactivator) {
    if (!s_systems.deactivators.contains(pDeactivator))
      s_systems.deactivators.pushBack(pDeactivator);
  }

  void registerLevelPlay(bfc::Ref<ILevelPlay> const & pPlayer) {
    if (!s_systems.players.contains(pPlayer))
      s_systems.players.pushBack(pPlayer);
  }

  void registerLevelPause(bfc::Ref<ILevelPause> const & pPauser) {
    if (!s_systems.pausers.contains(pPauser))
      s_systems.pausers.pushBack(pPauser);
  }

  void registerLevelStop(bfc::Ref<ILevelStop> const & pStopper) {
    if (!s_systems.stoppers.contains(pStopper))
      s_systems.stoppers.pushBack(pStopper);
  }

  void registerLevelUpdate(bfc::Ref<ILevelUpdate> const & pUpdater) {
    if (!s_systems.updaters.contains(pUpdater))
      s_systems.updaters.pushBack(pUpdater);
  }

  void registerLevelRenderDataCollector(bfc::Ref<ILevelRenderDataCollector> const & pCollector) {
    if (!s_systems.renderDataCollectors.contains(pCollector))
      s_systems.renderDataCollectors.pushBack(pCollector);
  }

  bfc::Ref<Level> createLevel() {
    auto pLevel = bfc::Ref<Level>(new Level, [](Level * pLevel) {
      for (auto const & pSystem : s_systems.destroyers)
        pSystem->destroyed(pLevel);

      delete pLevel;
    });

    for (auto const & pSystem : s_systems.creators)
      pSystem->created(pLevel.get());

    return pLevel;
  }

  void playLevel(Level * pLevel) {
    for (auto const & pSystem : s_systems.players)
      pSystem->play(pLevel);
  }

  void pauseLevel(Level * pLevel) {
    for (auto const & pSystem : s_systems.pausers)
      pSystem->pause(pLevel);
  }

  void stopLevel(Level * pLevel) {
    for (auto const & pSystem : s_systems.stoppers)
      pSystem->stop(pLevel);
  }

  void updateLevel(Level * pLevel, bfc::Timestamp dt) {
    for (auto const & pSystem : s_systems.updaters)
      pSystem->update(pLevel, dt);
  }

  void activateLevel(Level * pLevel) {
    for (auto const & pSystem : s_systems.activators)
      pSystem->activate(pLevel);
  }

  void deactivateLevel(Level * pLevel) {
    for (auto const & pSystem : s_systems.deactivators)
      pSystem->deactivate(pLevel);
  }

  void collectRenderData(RenderView * pRenderView, Level const * pLevel) {
    for (auto const & pSystem : s_systems.renderDataCollectors)
      pSystem->collectRenderData(pRenderView, pLevel);
  }
} // namespace engine
