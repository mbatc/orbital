#pragma once

#include "core/Filename.h"
#include "core/Serialize.h"
#include "core/String.h"

namespace bfc {
  class SerializedObject;

  enum BuildTargetType {
    BuildTargetType_Unknown = -1,
    BuildTargetType_Exe,
    BuildTargetType_SharedLibrary,
    BuildTargetType_StaticLibrary,
    BuildTargetType_None,
    BuildTargetType_Count,
  };

  enum FloatingPointModel {
    FloatingPointModel_Unspecified = -1,
    FloatingPointModel_Fast,
    FloatingPointModel_Accurate,
    FloatingPointModel_Count,
  };

  enum WarningLevel {
    WarningLevel_Unspecified = -1,
    WarningLevel_Off,
    WarningLevel_Default,
    WarningLevel_Extra,
    WarningLevel_Count,
  };

  struct BFC_API BuildConfiguration {
    int64_t            staticRuntime   = -1;
    int64_t            debugRuntime    = -1;
    int64_t            symbols         = -1;
    int64_t            optimize        = -1;
    int64_t            editAndContinue = -1;
    FloatingPointModel floatModel      = FloatingPointModel_Unspecified;
    WarningLevel       warningLevel    = WarningLevel_Unspecified;

    Vector<String> flags;
    Vector<String> buildoptions;
    Vector<String> defines;
    Vector<String> links;

    Vector<Filename> libraryDirectories;
    Vector<Filename> includeDirectories;

    Filename intermediateDirectory; ///< Where to put intermediate build files (e.g. obj)
    Filename targetDirectory;       ///< Where to put the build artifacts
    Filename debugDirectory;        ///< Working directory when debugging

    Vector<String> postBuildCommands;
    Vector<String> preBuildCommands;

    // Merge build configurations, if there are conflicting values the right hand side is taken.
    BuildConfiguration operator|(BuildConfiguration const & rhs);

    String generatePremakeScript(Filename const & baseDir) const;

    static BuildConfiguration defaultDebug();
    static BuildConfiguration defaultRelease();
  };

  class BFC_API BuildTool {
  public:
    struct Module {
    public:
      String group; ///< Group the module belongs to
      String name;  ///< Name of the module in the project.

      String          targetName;                           ///< Name of build artifact
      BuildTargetType targetType = BuildTargetType_Unknown; ///< Type of build artifact

      Vector<String> files; ///< Files in the project

      Filename moduleDirectory; ///< Relative path to module (from project root)

      Map<String, BuildConfiguration> configurations; ///< Configurations in this module.

      String generatePremakeScript(Filename const & projectDirectory) const;
    };

    struct ExternalModule {
      String   group;  ///< The group to place the module under
      Filename script; ///< Premake lua script to include.
    };

    String   projectName;      ///< Name of the entire project
    Filename projectDirectory; ///< Top level directory for the project

    Vector<Module> modules; ///< Modules in the project

    Map<String, String>    variables;       ///< Variables to add to the generate premake script. These may be referenced by an external project.
    Vector<ExternalModule> externalModules; ///< Additional external modules to include

    Map<String, BuildConfiguration> configurations; ///< Global configurations (defaults for every module).

    Module * findModule(String const & name);

    Module const * findModule(String const & name) const;

    Module * findModule(String const & name, String const & group);

    Module const * findModule(String const & name, String const & group) const;

    bool generateProject(String const & action = "vs2019") const;

    bool build(String const & msBuildPath, String const & configuration) const;

    SerializedObject exportConfig() const;

    void readConfig(SerializedObject const & dom);

  private:
    String generatePremakeScript() const;
    String generateModuleIncludes() const;

    Vector<String> listConfigurations() const;

    bool writeProjectFiles() const;
    void cleanupProjectFiles() const;
  };

  template<>
  struct BFC_API Serialization<BuildConfiguration> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, BuildConfiguration const & o) {
      SerializedObject  s;
      if (o.staticRuntime != -1)
        s.get("static-runtime") = ar(o.staticRuntime);
      if (o.debugRuntime != -1)
        s.get("debug-runtime") = ar(o.debugRuntime);
      if (o.symbols != -1)
        s.get("symbols") = ar(o.symbols);
      if (o.optimize != -1)
        s.get("optimize") = ar(o.optimize);
      if (o.editAndContinue != -1)
        s.get("edit-and-continue") = ar(o.editAndContinue);
      if (o.floatModel != FloatingPointModel_Unspecified)
        s.get("fp-model") = ar(o.floatModel);
      if (o.warningLevel != WarningLevel_Unspecified)
        s.get("warnings") = ar(o.warningLevel);
      if (o.flags.size())
        s.get("flags") = ar(o.flags);
      if (o.buildoptions.size())
        s.get("build-options") = ar(o.buildoptions);
      if (o.defines.size())
        s.get("defines") = ar(o.defines);
      if (o.links.size())
        s.get("links") = ar(o.links);
      if (o.libraryDirectories.size())
        s.get("library-dirs") = ar(o.libraryDirectories);
      if (o.includeDirectories.size())
        s.get("include-dirs") = ar(o.includeDirectories);
      if (o.intermediateDirectory.length())
        s.get("intermediate-dir") = ar(o.intermediateDirectory);
      if (o.targetDirectory.length())
        s.get("target-directory") = ar(o.targetDirectory);
      if (o.debugDirectory.length())
        s.get("debug-directory") = ar(o.debugDirectory);
      return s;
    }
    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, BuildConfiguration & o) {
      ar(s.get("static-runtime"), ReadOrConstruct(o.staticRuntime, -1));
      ar(s.get("debug-runtime"), ReadOrConstruct(o.debugRuntime, -1));
      ar(s.get("symbols"), ReadOrConstruct(o.symbols, -1));
      ar(s.get("optimize"), ReadOrConstruct(o.optimize, -1));
      ar(s.get("edit-and-continue"), ReadOrConstruct(o.editAndContinue, -1));
      ar(s.get("fp-model"), ReadOrConstruct(o.floatModel, FloatingPointModel_Unspecified));
      ar(s.get("warnings"), ReadOrConstruct(o.warningLevel, WarningLevel_Default));
      ar(s.get("flags"), ReadOrConstruct(o.flags));
      ar(s.get("build-options"), ReadOrConstruct(o.buildoptions));
      ar(s.get("defines"), ReadOrConstruct(o.defines));
      ar(s.get("links"), ReadOrConstruct(o.links));
      ar(s.get("library-dirs"), ReadOrConstruct(o.libraryDirectories));
      ar(s.get("include-dirs"), ReadOrConstruct(o.includeDirectories));
      ar(s.get("intermediate-dir"), ReadOrConstruct(o.intermediateDirectory));
      ar(s.get("target-directory"), ReadOrConstruct(o.targetDirectory));
      ar(s.get("debug-directory"), ReadOrConstruct(o.debugDirectory));
      return true;
    }
  };

  template<>
  struct BFC_API Serialization<BuildTool::Module> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, BuildTool::Module const & o) {
      SerializedObject  s;
      if (o.name.length())
        s.get("name") = ar(o.name);
      if (o.group.length())
        s.get("group") = ar(o.group);
      if (o.moduleDirectory.length())
        s.get("directory") = ar(o.moduleDirectory);
      if (o.targetName.length())
        s.get("target-name") = ar(o.targetName);
      if (o.targetType != BuildTargetType_Unknown)
        s.get("target-type") = ar(o.targetType);
      if (o.files.size())
        s.get("files") = ar(o.files);
      if (o.configurations.size())
        s.get("configurations") = ar(o.configurations);
      return s;
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, BuildTool::Module & o) {
      ar(s.get("name"), ReadOrConstruct(o.name));
      ar(s.get("directory"), ReadOrConstruct(o.moduleDirectory));
      ar(s.get("group"), ReadOrConstruct(o.group));
      ar(s.get("target-name"), ReadOrConstruct(o.targetName));
      ar(s.get("target-type"), ReadOrConstruct(o.targetType, BuildTargetType_Unknown));
      ar(s.get("files"), ReadOrConstruct(o.files));
      ar(s.get("configurations"), ReadOrConstruct(o.configurations));
      return true;
    }
  };

  template<>
  struct BFC_API Serialization<BuildTool::ExternalModule> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, BuildTool::ExternalModule const & o) {
      SerializedObject  s;
      s.add("group")  = ar(o.group);
      s.add("script") = ar(o.script);
      return s;
    }
    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, BuildTool::ExternalModule & o) {
      ar(s.get("group"), ReadOrConstruct(o.group));
      ar(s.get("script"), ReadOrConstruct(o.script));
      return true;
    }
  };

  template<>
  struct BFC_API Serialization<BuildTool> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, BuildTool const & o) {
      SerializedObject  s;
      s.get("variables")         = ar(o.variables);
      SerializedObject & project = s.get("project");
      project.get("name")        = ar(o.projectName);
      project.get("directory")   = ar(o.projectDirectory);
      s.get("configurations")    = ar(o.configurations);
      s.get("external-modules")  = ar(o.externalModules);
      s.get("modules")           = ar(o.modules);
      return s;
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, BuildTool & o) {
      ar(s.get("variables"), ReadOrConstruct(o.variables));
      ar(s.get("configurations"), ReadOrConstruct(o.configurations));
      ar(s.get("external-modules"), ReadOrConstruct(o.externalModules));
      ar(s.get("modules"), ReadOrConstruct(o.modules));

      SerializedObject const & project = s.get("project");
      ar(project.get("name"), ReadOrConstruct(o.projectName));
      ar(project.get("directory"), ReadOrConstruct(o.projectDirectory));
      return true;
    }
  };

  template<>
  struct EnumValueMap<WarningLevel> {
    inline static Map<WarningLevel, String> const mapping = {
      {WarningLevel_Off, "off"},
      {WarningLevel_Default, "default"},
      {WarningLevel_Extra, "extra"},
    };
  };

  template<>
  struct EnumValueMap<FloatingPointModel> {
    inline static Map<FloatingPointModel, String> const mapping = {
      {FloatingPointModel_Unspecified, "unspecified"},
      {FloatingPointModel_Fast, "fast"},
      {FloatingPointModel_Accurate, "accurate"},
    };
  };

  template<>
  struct EnumValueMap<BuildTargetType> {
    inline static Map<BuildTargetType, String> const mapping = {
      {BuildTargetType_Unknown, "unknown"},          {BuildTargetType_Exe, "exe"},   {BuildTargetType_SharedLibrary, "shared-lib"},
      {BuildTargetType_StaticLibrary, "static-lib"}, {BuildTargetType_None, "none"},
    };
  };
} // namespace bfc
