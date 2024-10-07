newoption {
  trigger     = "bfc-root",
  value       = "path",
  description = "Output directory for the compiled executable"
}

function getScriptDir()
   local str = debug.getinfo(2, "S").source:sub(2)
   return str:match("(.*/)")
end

location (_ACTION)

if BFC_ROOT == nil then
  BFC_ROOT=_OPTIONS["bfc-root"] or getScriptDir()
end

targetdir  "%{wks.location}/../build/bin/%{cfg.buildcfg}/%{cfg.system}/%{cfg.architecture}"
objdir     "%{wks.location}/../build/intermediate/%{cfg.buildcfg}/%{cfg.system}/%{cfg.architecture}/%{prj.name}"
debugdir   "%{wks.location}/../build/bin/"
targetname "%{prj.name}"
symbolspath "$(OutDir)$(TargetName).pdb"

libdirs {
  "%{wks.location}/../build/bin/%{cfg.buildcfg}/%{cfg.system}/%{cfg.architecture}",
}

-- include this file in the project
files {
  "%{wks.location}/../prj-common.lua",
  "%{wks.location}/../.clang-format"
}

-- Build options
buildoptions {
  "/wd4251",
  "/bigobj"  -- enable large number of obj sections
}

-- Linker options
linkoptions {
  "/ignore:4006",
  "/ignore:4221",
  "/ignore:4075"
}

filter   { "configurations:Debug" }
  runtime         "Debug"
  optimize        "Off"
  symbols         "On"
  inlining        "Default"
  floatingpoint   "Fast"
  editandcontinue "Off"
  buildoptions    { "/MP" }
  defines         { "DEBUG"}
  

filter   { "configurations:Release" }
  runtime         "Release"
  optimize        "On"
  symbols         "On"
  inlining        "Auto"
  floatingpoint   "Fast"
  editandcontinue "Off"
  flags           { "LinkTimeOptimization" }
  buildoptions    { "/MP" }
  defines         { "NDEBUG" }

filter {}
  defines { "BFC_STATIC" }
