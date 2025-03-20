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
  struct BFC_API Serializer<BuildConfiguration> {
    static SerializedObject write(BuildConfiguration const & o, ...);
    static bool             read(SerializedObject const & s, BuildConfiguration & o, ...);
  };

  template<>
  struct BFC_API Serializer<BuildTool::Module> {
    static SerializedObject write(BuildTool::Module const & o, ...);
    static bool             read(SerializedObject const & s, BuildTool::Module & o, ...);
  };

  template<>
  struct BFC_API Serializer<BuildTool::ExternalModule> {
    static SerializedObject write(BuildTool::ExternalModule const & o, ...);
    static bool             read(SerializedObject const & s, BuildTool::ExternalModule & o, ...);
  };

  template<>
  struct BFC_API Serializer<BuildTool> {
    static SerializedObject write(BuildTool const & o, ...);
    static bool             read(SerializedObject const & s, BuildTool & o, ...);
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
