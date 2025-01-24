project "bullet3"

kind "StaticLib"
architecture "x64"
language     "C++"
cppdialect   "C++17"
characterset "MBCS"

-- disablewarnings { "4244", "4267" }
disablewarnings { "4244", "4267", "4018", "4316", "4756", "4005" }

includedirs {
  ORBITAL_ROOT .. "/vendor/bullet3/src/",
}

files {
  "bullet3.lua",
  ORBITAL_ROOT .. "/vendor/bullet3/src/**.h",
  ORBITAL_ROOT .. "/vendor/bullet3/src/btBulletCollisionAll.cpp",
  ORBITAL_ROOT .. "/vendor/bullet3/src/btLinearMathAll.cpp",
  ORBITAL_ROOT .. "/vendor/bullet3/src/btBulletDynamicsAll.cpp"
}

defines {
  "BT_USE_SSE_IN_API"
}

configurations {"Release", "Debug"}

floatingpoint "Fast"
editandcontinue "Off"

configuration "Release"
  optimize "On"
  vectorextensions "SSE2"
  symbols "On"

configuration "Debug"
  defines {"_DEBUG=1"}
  symbols "On"
  flags { "NoMinimalRebuild" }


  -- group "Vendor/Bullet3"
-- 
-- function addBulletProject(path)
--   dofile(path)
-- 
--   buildoptions {
--     -- Multithreaded compiling
--     "/MP",
--   }
-- 
--   disablewarnings { "4244", "4267", "4018", "4316", "4756", "4005" }
--   
--   defines {
--     "BT_USE_SSE_IN_API"
--   }
--   
--   configurations {"Release", "Debug"}
--   
--   floatingpoint "Fast"
--   editandcontinue "Off"
--   
--   configuration "Release"
--     optimize "On"
--     vectorextensions "SSE2"
--     symbols "On"
--   
--   configuration "Debug"
--     defines {"_DEBUG=1"}
--     symbols "On"
--     flags { "NoMinimalRebuild" }
-- end
-- 
-- addBulletProject (ORBITAL_ROOT .. "/vendor/bullet3/src/Bullet3Dynamics/premake4.lua")
-- addBulletProject (ORBITAL_ROOT .. "/vendor/bullet3/src/Bullet3Collision/premake4.lua")
-- addBulletProject (ORBITAL_ROOT .. "/vendor/bullet3/src/Bullet3Geometry/premake4.lua")
-- addBulletProject (ORBITAL_ROOT .. "/vendor/bullet3/src/BulletCollision/premake4.lua")
-- addBulletProject (ORBITAL_ROOT .. "/vendor/bullet3/src/BulletDynamics/premake4.lua")
-- addBulletProject (ORBITAL_ROOT .. "/vendor/bullet3/src/BulletInverseDynamics/premake4.lua")
-- addBulletProject (ORBITAL_ROOT .. "/vendor/bullet3/src/BulletSoftBody/premake4.lua")
