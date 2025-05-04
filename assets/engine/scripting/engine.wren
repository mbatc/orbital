
foreign class EngineAPI {
  foreign construct new()

  foreign _addComponent(componentType)
  foreign _addSystem(systemType)

  addComponent(componentType) {
    if (_components == null) {
      _components = []
    }

    _components.add(componentType)
    _addComponent(systemType)
  }

  addSystem(systemType) {
    _addSystem(systemType)
  }
}
var Engine = EngineAPI.new()

