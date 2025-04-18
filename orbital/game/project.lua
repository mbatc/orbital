
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
}

dependson {
  "lib",
}

links {
  "lib",
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
