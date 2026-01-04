workspace "Orbital"
  startproject "Orbital"

  configurations {
    "Debug",
    "Release"
  }

  dofile "prj-common.lua"

  group "benfromcanada"
    dofile "orbital/lib/project.lua"
    dofile "orbital/test/project.lua"
    dofile "orbital/game/project.lua"

  group "Vendor"
    dofile "orbital/vendor/yaml-cpp.lua"

  dofile "orbital/experiments/project.lua"
