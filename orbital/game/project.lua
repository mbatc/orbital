
project "game"

kind         "ConsoleApp"
architecture "x64"
language     "C++"
cppdialect   "C++17"
characterset "MBCS"

includedirs {
  "../lib/include",
  "src/app",
  "src/engine",
  "src",

  ORBITAL_ROOT .. "vendor/glm/",
  ORBITAL_ROOT .. "vendor/bullet3/src/"
}

dependson {
  "lib",
  "bullet3"
}

links {
  "lib",
  "bullet3"
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

  "../../assets/**"
}
