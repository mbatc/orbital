project "adderlang"

kind "StaticLib"
architecture "x64"
language     "C++"
cppdialect   "C++17"
characterset "MBCS"

includedirs {
  ORBITAL_ROOT .. "/vendor/adder/lib/include/adder"
}

files {
  ORBITAL_ROOT .. "/vendor/adder/lib/include/**",
  ORBITAL_ROOT .. "/vendor/adder/lib/src/**",
}
