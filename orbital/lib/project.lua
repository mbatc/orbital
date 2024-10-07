
project "lib"

kind "StaticLib"
architecture "x64"
language     "C++"
cppdialect   "C++17"
characterset "MBCS"

dependson { "yaml-cpp" }
defines   { "BFC_EXPORT_SYMBOLS" }

includedirs {
  "include/",
  BFC_ROOT .. "vendor/yaml-cpp/include"
}

libdirs {
  BFC_ROOT .. "vendor/glew/lib/Release/x64"
}

links {
  "glew32s",
  "Opengl32.lib",
  "Rpcrt4.lib",
  "yaml-cpp.lib"
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

  "shaders/**",

  -- imgui source
  BFC_ROOT .. "vendor/imgui/*.h",
  BFC_ROOT .. "vendor/imgui/*.cpp",
  BFC_ROOT .. "vendor/imguizmo/*.h",
  BFC_ROOT .. "vendor/imguizmo/*.cpp",
}

-- Find the FBX-SDK
if BFC_FBX_ROOT == nil then
  print("BFC_FBX_ROOT not specified. Searching for the FBX SDK")
  local FBX_SEARCH_VERSIONS = { "2020.0.1" }
  
  local FBX_SEARCH_PATHS = {
    os.getenv("PROGRAMFILES"),
    os.getenv("PROGRAMW6432"),
    os.getenv("PROGRAMFILES(x86)"),
  }

  for _,ver in pairs(FBX_SEARCH_VERSIONS) do
    if BFC_FBX_ROOT ~= nil then
      break
    end

    for _, fbxPath in pairs(FBX_SEARCH_PATHS) do
      local testPath = path.join(fbxPath, "Autodesk/FBX/FBX SDK/" .. ver)
      print("Testing " .. testPath)
      if os.isdir(testPath) then
        BFC_FBX_ROOT = testPath
        break
      end
    end
  end
  
  if BFC_FBX_ROOT == nil then
    print("The FBX-SDK was not found. Disabling FBX features. Download and install the FBX SDK to enable these features")
  else
    print("The FBX SDK was found at " .. BFC_FBX_ROOT)
  end
else
  print("Using FBX SDK path " .. BFC_FBX_ROOT)
end

if BFC_FBX_ROOT then
  BFC_FBX_COMPILER = "vs2017" -- TODO: Figure this out based on platform and paths that exist
  BFC_FBX_PLATFORM = "x64"

  includedirs {
    BFC_FBX_ROOT .. "/include"
  }

  BFC_FBX_BIN_PATH = BFC_FBX_ROOT .. "/lib/" .. BFC_FBX_COMPILER .. "/" .. BFC_FBX_PLATFORM .. "/%{cfg.buildcfg}"

  libdirs {
    BFC_FBX_BIN_PATH
  }

  links {
    "libfbxsdk.lib"
  }

  postbuildcommands {
    "{COPYFILE} \"" .. BFC_FBX_BIN_PATH .. "/libfbxsdk.dll\" \"%{cfg.buildtarget.directory}/libfbxsdk.dll\""
  }

  defines {
    "BFC_ENABLE_FBX_SDK"
  }
end
