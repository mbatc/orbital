project "yaml-cpp"

kind "StaticLib"
architecture "x64"
language     "C++"
cppdialect   "C++17"
characterset "MBCS"

defines {
  "YAML_CPP_STATIC_DEFINE"
}

includedirs {
  ORBITAL_ROOT .. "/vendor/yaml-cpp/include/",
}

files {
  ORBITAL_ROOT .. "/vendor/yaml-cpp/include/**",
  ORBITAL_ROOT .. "/vendor/yaml-cpp/src/**",
}
