#include "LevelSystem.h"
#include "core/Vector.h"

namespace engine {
  struct {
    bfc::Vector<bfc::Ref<ILevelActivate>>   activators;
    bfc::Vector<bfc::Ref<ILevelDeactivate>> deactivators;
    bfc::Vector<bfc::Ref<ILevelPlay>>       players;
    bfc::Vector<bfc::Ref<ILevelPause>>      pausers;
    bfc::Vector<bfc::Ref<ILevelStop>>       stoppers;
    bfc::Vector<bfc::Ref<ILevelUpdate>>     updaters;
  } static s_systems;

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
} // namespace engine
