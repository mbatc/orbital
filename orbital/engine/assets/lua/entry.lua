
-- Patch require for relative imports and virtual filenames
Engine = {}
Engine.null = {}

function Engine.startup()
  Engine.Game = require("game:lua.package")
end

function Engine.shutdown()
  for name, type in pairs(Engine.detail.registeredBehaviours) do
    Engine.detail.removeBehaviour(name)
  end
end

function Engine.bindEntity(component, levelHandle, entityId)
  component.entity = Entity(levelHandle, entityId)
  component.level  = Level(levelHandle)
end

function Engine.addBehaviour(type)
  local registered = Engine.detail.registeredBehaviours
  local name       = type.__name

  if registered[name] ~= nil then
    Engine.error(("Behaviour name already registered: %s"):format(name)
  else
    -- TODO: check success? Other systems/languages might add behaviours
    Engine.detail.addBehaviour(name, type)
    registered[name] = type
  end
end

function Engine.removeBehaviour(name)
  local registered = Engine.detail.registeredBehaviours
  local name       = type.__name
  if registered[name] == nil then
    Engine.error(("Behaviour name is not registered: %s"):format(name)
  else
    Engine.detail.removeBehaviour(name)
    registered[name] = nil
  end
end

function Engine.isBehaviour(type)
  return Engine.detail.registeredBehaviours[type.__name] ~= nil 
end

Engine.detail = {
  require_stack = {},

  bindToEntity = function(level, entityId)

  end,

  -- methods provided by the runtime
  addBehaviour = {},
  
  removeBehaviour = {},
}

Engine.detail.require = require

function require(name)
  local original_path = package.path

  uri = Engine.resolveModuleURI(name)

  print(Engine.detail.require_stack)
  print(#Engine.detail.require_stack)
  
  if (#Engine.detail.require_stack == 0) then
    print("Patch package.path")
    package.path = "?.lua;" .. package.path
  else
    uri = Engine.resolveRelativeReference(Engine.detail.require_stack[#Engine.detail.require_stack], uri)
  end

  table.insert(Engine.detail.require_stack, uri)
  loaded = Engine.detail.require(uri)
  table.remove(Engine.detail.require_stack)

  if (#Engine.detail.require_stack == 0) then
    package.path = original_path
  end

  return loaded
end

-- Core utilities
local function BasicDeepCopy(orig)
    if type(orig) ~= "table" then return orig end
  
    local copy = {}
    for k, v in pairs(orig) do
        copy[k] = BasicDeepCopy(v)
    end

    return copy
end

-- Instantiate a class type.
-- Creates fields and links the class metatable.
-- Additional initialization may be provided by the class type.
  
-- Declare a class type.
-- @param fields member fields to create for an instance
function Class(meta)
  local type = meta.static or {}
  
  type._meta = {
    public = meta.public,
    new = meta.new,
    name = meta.name
  }

  type.__name = meta.name
  type.__index = type
  type.__newindex = function (t, k, v)
    if type[k] ~= nil then
        type[k] = v
    else
        rawset(t, k, v)
    end
  end
  
  local function create(type, args)
    local instance = nil
    if type._meta.public ~= nil then
      instance = BasicDeepCopy(type._meta.public)
    else
      instance = {}
    end
    setmetatable(instance, type)
  
    return instance
  end

  local newable = {}
  if type._meta.new ~= nil then
    newable.__call = function(self, ...)
      instance = create(type)
      type._meta.new(instance, ...)
      return instance
    end
  else
    newable.__call = function(self)
      return create(type)
    end
  end
  setmetatable(type, newable)

  return type
end

Engine.Entity = Class {
  public = {
    level = Engine.null,
    id = 0,
  },

  new = function(self, levelHandle, id)
    self.level = Engine.Level(levelHandle)
    self.id    = id
  end
}

function Engine.Entity:get(type)
  if Engine.isBehaviour(type) then
    return self.entity.behaviours[type.__name]
  else
    return self.level.getComponent(self.id, type)
  end
end

Engine.Level = Class {
  public = {
    handle = Engine.null,
  },

  new = function(self, levelHandle)
    self.handle = levelHandle
  end
}

