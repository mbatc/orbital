import "file:engine:/scripting/engine" for Engine

class VehicleCameraController {
  construct new() {

  }
}

class PlayerControlSystem {
  construct new() {}

  // Level system interface
  activate(level) {
    System.print("Level was activated")
  }

  deactivate(level) {
    System.print("Level was deactivated")
  }

  play(level, dt) {
    System.print("Level was played")
  }

  pause(level) {
    System.print("Level was paused")
  }

  stop(level) {
    System.print("Level was stopped")
  }
  update(level) {
    System.print("Update from wren")
  }

  // Level rendering interface
  collectRenderData(renderView, level) { null }
}

Engine.addComponent({
  "module": "PlayerControl",
  "type": VehicleCameraController
})
Engine.addSystem(PlayerControlSystem)
