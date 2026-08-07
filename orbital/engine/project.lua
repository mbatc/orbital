
project "engine"

kind         "StaticLib"
architecture "x64"
language     "C++"
cppdialect   "C++17"
characterset "MBCS"

includedirs {
  "../lib/include",
  "src/engine",
  "src",

  ORBITAL_ROOT .. "vendor/adder/lib/include",
  ORBITAL_ROOT .. "vendor/glm/",
}

dependson {
  "lib",
  "adderlang"
}

links {
  "lib",
  "adderlang"
}

files {
  "README.md",
  "project.lua",

  "**.natvis",

  "include/**.h",
  "include/**.inl",
  "include/**.cpp",

  "src/**.h",
  "src/**.inl",
  "src/**.cpp",
  
  "assets/**"
}
