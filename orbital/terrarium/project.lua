
project "terrarium"

kind         "ConsoleApp"
architecture "x64"
language     "C++"
cppdialect   "C++17"
characterset "MBCS"

includedirs {
  "../lib/include",
  "../engine/src",
  "src/app",
  "src",

  ORBITAL_ROOT .. "vendor/glm/",
}

dependson {
  "lib",
  "engine"
}

links {
  "lib",
  "engine"
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
