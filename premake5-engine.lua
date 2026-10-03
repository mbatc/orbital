
configurations {
  "Debug",
  "Release"
}

dofile "prj-common.lua"

group "Engine"
  dofile "orbital/lib/project.lua"
  dofile "orbital/test/project.lua"
  dofile "orbital/engine/project.lua"

group "Vendor"
  dofile "orbital/vendor/yaml-cpp.lua"
  dofile "orbital/vendor/bullet3.lua"
