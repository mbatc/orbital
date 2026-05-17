workspace "Orbital"
  startproject "Orbital"

  configurations {
    "Debug",
    "Release"
  }

  dofile "prj-common.lua"

  
  group "Orbital"
    dofile "orbital/lib/project.lua"
    dofile "orbital/test/project.lua"
    dofile "orbital/game/project.lua"
    dofile "orbital/engine/project.lua"

  group "Vendor"
    dofile "orbital/vendor/yaml-cpp.lua"
