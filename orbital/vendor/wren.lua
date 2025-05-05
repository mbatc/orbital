project "wren"

kind "StaticLib"
architecture "x64"
language     "C++"
cppdialect   "C++17"
characterset "MBCS"

defines {
  "_CRT_SECURE_NO_WARNINGS"
}

includedirs {
  ORBITAL_ROOT .. "/vendor/wren/src/include/",
  ORBITAL_ROOT .. "/vendor/wren/src/optional/",
  ORBITAL_ROOT .. "/vendor/wren/src/vm/",
}

files {
  ORBITAL_ROOT .. "/vendor/wren/src/include/**",
  ORBITAL_ROOT .. "/vendor/wren/src/optional/**",
  ORBITAL_ROOT .. "/vendor/wren/src/vm/**",
}
