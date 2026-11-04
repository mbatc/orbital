project "lua"

kind "StaticLib"
architecture "x64"
language     "C++"
cppdialect   "C++17"
characterset "MBCS"

warnings "off"

defines { "MAKE_LIB" }

includedirs {
  ORBITAL_ROOT .. "/vendor/lua/"
}

files {
  ORBITAL_ROOT .. "/vendor/lua/onelua.c",
  "lua.lua"
}
