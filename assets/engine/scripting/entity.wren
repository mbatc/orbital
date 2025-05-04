
class Entity {
  construct new(level, id) {
    _id = id
    _level = level
  }

  id { _id }

  #!serialize=false
  level {_level}

  get(ComponentType) {
    return _level.get(ComponentType, _id)
  }

  has(ComponentType) {
    return _level.has(ComponentType, _id)
  }
}
