workspace "Orbital"
  startproject "Orbital"

  dofile "premake5-engine.lua"

  group "Orbital"
    dofile "orbital/game/project.lua"
