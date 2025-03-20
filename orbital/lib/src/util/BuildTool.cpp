#include "util/BuildTool.h"
#include "core/File.h"
#include "platform/OS.h"

#include <filesystem>
#include <xhash>

namespace bfc {
  static Vector<String> s_buildTargetName;
  static Vector<String> s_floatingPointModelName;
  static Vector<String> s_warningLevelName;

  namespace detail {
    template<typename T>
    static void premakeMergeList(Vector<T> & dst, Vector<T> const & other) {
      for (T const & entry : other)
        if (std::find(dst.begin(), dst.end(), entry) == dst.end())
          dst.pushBack(entry);
    };

    static String premakeStringList(String const & field, Vector<String> const & items) {
      if (items.size() == 0)
        return "";

      Vector<String> list;
      for (String const & item : items)
        list.pushBack("'" + item.replace("\\", "/") + "'");
      return field + " {\n  " + String::join(list, ",\n  ") + "\n}";
    }

    static String premakeFileList(String const & field, Vector<String> const & items) {
      if (items.size() == 0)
        return "";

      Vector<String> list;
      for (String const & item : items)
        list.pushBack("'" + item.replace("\\", "/") + "'");
      return field + " {\n  " + String::join(list, ",\n  ") + "\n}";
    }

    static String premakeFileList(String const & field, Vector<Filename> const & items, Filename const & baseDir = "") {
      if (items.size() == 0)
        return "";

      Vector<String> list = items.map([=](Filename const & file) { return file.empty() ? "" : ("'" + (baseDir / file).path() + "'"); });
      return field + " {\n  " + String::join(list, ",\n  ", true) + "\n}";
    }

    static String premakePath(String const & field, Filename const & item, Filename const & baseDir = "") {
      if (item.empty())
        return "";
      return field + " '" + (baseDir / item).getView() +  "'";
    }

    static String premakeVariables(Map<String, String> const & variables) {
      return String::join(
        variables.getItems().map([](auto const & item) {
            return String::format("%s=%s", item.first, item.second);
          }
        ),
        "\n",
        true
      );
    }

    static String premakeString(String const & field, String const & item) {
      if (item.empty())
        return "";
      return field + " '" + item.replace("\\", "/") + "'";
    }

    static String selectOption(String const & field, int64_t value, String const & trueOpt, String const & falseOpt) {
      if (value == 0)
        return field + " " + falseOpt + "";
      else if (value == 1)
        return field + " " + trueOpt + "";
      return "";
    }

    static String generateConfigurations(Map<String, BuildConfiguration> const & configurations, Filename const & baseDir = "") {
      String script;
      for (auto & [name, config] : configurations) {
        bool isGlobal = name.empty();
        script        = script + "\n-- Begin Configuration (" + (isGlobal ? "ALL" : name) + ") --\n";
        script        = script + String::join(
                     Vector<String>{selectOption("filter", isGlobal, "{}", "{ 'configurations:" + name + "' }"), config.generatePremakeScript(baseDir)}, "\n");
        script = script + "-- End Configuration (" + (isGlobal ? "ALL" : name) + ") --\n";
      }

      script = script + "filter {}\n";
      return script;
    }

    static String buildTargetName(BuildTargetType const & type) {
      switch (type) {
      case BuildTargetType_Unknown: return "Unknown";
      case BuildTargetType_Exe: return "ConsoleApp";
      case BuildTargetType_SharedLibrary: return "SharedLib";
      case BuildTargetType_StaticLibrary: return "StaticLib";
      case BuildTargetType_None:
      default: return "None";
      }
    }

    static bool writeFile(Filename const & path, String const & content) {
      if (writeTextFile(path, content)) {
        // flInfo("Generated project file '%s'", path.string().c_str());
        return true;
      } else {
        // flWarning("Failed to write project file '%s'", path.string().c_str());
        return false;
      }
    }
  } // namespace detail

  BuildConfiguration BuildConfiguration::operator|(BuildConfiguration const & rhs) {
    BuildConfiguration config = *this;
    if (rhs.debugRuntime != -1)
      config.debugRuntime = rhs.debugRuntime;
    if (rhs.symbols != -1)
      config.symbols = rhs.symbols;
    if (rhs.optimize != -1)
      config.optimize = rhs.optimize;
    if (rhs.editAndContinue != -1)
      config.editAndContinue = rhs.editAndContinue;
    if (rhs.floatModel != -1)
      config.floatModel = rhs.floatModel;

    if (!rhs.intermediateDirectory.empty())
      config.intermediateDirectory = rhs.intermediateDirectory;

    detail::premakeMergeList(config.flags, rhs.flags);
    detail::premakeMergeList(config.buildoptions, rhs.buildoptions);
    detail::premakeMergeList(config.defines, rhs.defines);
    detail::premakeMergeList(config.links, rhs.links);
    detail::premakeMergeList(config.libraryDirectories, rhs.libraryDirectories);
    detail::premakeMergeList(config.includeDirectories, rhs.includeDirectories);
    detail::premakeMergeList(config.postBuildCommands, rhs.postBuildCommands);
    detail::premakeMergeList(config.preBuildCommands, rhs.preBuildCommands);

    return config;
  }

  BuildConfiguration BuildConfiguration::defaultDebug() {
    BuildConfiguration cfg;
    cfg.staticRuntime         = 0;
    cfg.debugRuntime          = 1;
    cfg.optimize              = 0;
    cfg.symbols               = 1;
    cfg.floatModel            = FloatingPointModel_Accurate;
    cfg.editAndContinue       = 0;
    cfg.defines               = {"DEBUG"};
    return cfg;
  }

  BuildConfiguration BuildConfiguration::defaultRelease() {
    BuildConfiguration cfg;
    cfg.staticRuntime         = 0;
    cfg.debugRuntime          = 0;
    cfg.optimize              = 1;
    cfg.symbols               = 1;
    cfg.floatModel            = FloatingPointModel_Fast;
    cfg.editAndContinue       = 0;
    cfg.flags                 = {"LinkTimeOptimization"};
    cfg.buildoptions          = {"/MP"};
    cfg.defines               = {"NDEBUG"};
    return cfg;
  }

  String BuildConfiguration::generatePremakeScript(Filename const & baseDir) const {
    String script = String::join(
      Vector<String>{
                      detail::selectOption("runtime", debugRuntime, "'Debug'", "'Release'"),
                      detail::selectOption("optimize", optimize, "'On'", "'Off'"),
                      detail::selectOption("symbols", symbols, "'On'", "'Off'"),
                      detail::selectOption("editAndContinue", editAndContinue, "'On'", "'Off'"),
                      detail::selectOption("staticruntime", staticRuntime, "'On'", "'Off'"),
                      detail::premakeStringList("flags", flags),
                      detail::premakeStringList("buildoptions", buildoptions),
                      detail::premakeStringList("defines", defines),
                      detail::premakeStringList("links", links),
                      detail::premakeFileList("libdirs", libraryDirectories, baseDir),
                      detail::premakeFileList("includedirs", includeDirectories, baseDir),
                      detail::premakeStringList("postbuildcommands", postBuildCommands),
                      detail::premakeStringList("prebuildcommands", preBuildCommands),
                      detail::premakePath("objdir", intermediateDirectory.path(), baseDir),
                      detail::premakePath("targetdir", targetDirectory.path(), baseDir),
                      detail::premakePath("debugdir", debugDirectory.path(), baseDir),
                      "inlining 'Auto'\n"},
      "\n", true);

    return script;
  }

  BuildTool::Module * BuildTool::findModule(String const & name) {
    for (auto & module : modules)
      if (module.name == name)
        return &module;
    return nullptr;
  }

  BuildTool::Module const * BuildTool::findModule(String const & name) const {
    for (auto & module : modules)
      if (module.name == name)
        return &module;
    return nullptr;
  }

  BuildTool::Module * BuildTool::findModule(String const & name, String const & group) {
    for (auto & module : modules)
      if (module.name == name && module.group == group)
        return &module;
    return nullptr;
  }

  BuildTool::Module const * BuildTool::findModule(String const & name, String const & group) const {
    for (auto & module : modules)
      if (module.name == name && module.group == group)
        return &module;
    return nullptr;
  }

  bool BuildTool::generateProject(String const & action) const {
    if (!writeProjectFiles())
      return false;

    String command = "premake.exe " + action + " --file=\"" + (projectDirectory / "temp/premake.lua").path() + "\"";
    system(command.c_str());
    cleanupProjectFiles();
    return true;
  }

  bool BuildTool::build(String const & msBuildPath, String const & configuration) const {
    String command = "\"\"" + msBuildPath + "\" -m \"" + (projectDirectory / projectName).path() + ".sln\"\" /property:Configuration=" + configuration;
    system(command.c_str());
    return true;
  }

  String BuildTool::generatePremakeScript() const {
    return String::join(Vector<String>{detail::premakeVariables(variables),
                                       detail::premakeString("workspace", projectName),
                                       detail::premakeString("startproject", projectName),
                                       detail::premakeString("location", projectDirectory.absolute().path()),
                                       detail::premakeString("symbolspath", "$(OutDir)$(TargetName).pdb"),
                                       detail::premakeStringList("flags", {"MultiProcessorCompile"}),
                                       detail::premakeStringList("buildoptions", {"/wd4251"}),
                                       detail::premakeStringList("defines", {"_CRT_SECURE_NO_WARNINGS"}),
                                       detail::premakeStringList("configurations", listConfigurations()),
                                       detail::generateConfigurations(configurations, "../"),
                                       generateModuleIncludes()},
                        "\n");
  }

  String BuildTool::generateModuleIncludes() const {
    String script;

    Map<String, Vector<Filename>> groups;
    for (Module const & module : modules) {
      groups[module.group].pushBack(Filename(module.group) / module.name / "project.lua");
    }
    for (auto &[group, file] : externalModules) {
      groups[group].pushBack(file);
    }

    for (auto & [group, groupPaths] : groups) {
      script = script + String::format("group '%s'\n", group.c_str());
      script = script + String::join(groupPaths.map([](Filename const & path) { return detail::premakeString("dofile", path.path()); }), "\n") + "\n";
    }

    return script;
  }

  Vector<String> BuildTool::listConfigurations() const {
    Vector<String> configs = configurations.getKeys();
    for (Module const & module : modules)
      detail::premakeMergeList(configs, module.configurations.getKeys());
    return configs;
  }

  bool BuildTool::writeProjectFiles() const {
    Filename tempPath    = projectDirectory / "temp";
    Filename premakePath = tempPath / "premake.lua";

    bool success = true;
    std::filesystem::create_directories(tempPath.c_str());
    success &= writeTextFile(premakePath, generatePremakeScript());
    for (Module const & module : modules) {
      Filename projectFolder = tempPath / module.group / module.name;
      os::createFolders(projectFolder);
      success &= writeTextFile(projectFolder / "project.lua", module.generatePremakeScript(projectDirectory));
    }
    return success;
  }

  void BuildTool::cleanupProjectFiles() const {
    std::filesystem::remove_all((projectDirectory / "temp").c_str());
  }

  String BuildTool::Module::generatePremakeScript(Filename const & projectDirectory) const {
    Filename baseDir = projectDirectory / moduleDirectory;
    Vector<String> lines   = { detail::premakeString("project", name) };
    if (targetType == BuildTargetType_None) {
      lines.pushBack("kind \"Makefile\"\n"
                     "buildcommands{\"\"}\n"
                     "rebuildcommands{\"\"}\n"
                     "cleancommands{\"\"}");
    } else {
      lines.pushBack(detail::premakeString("kind", detail::buildTargetName(targetType)));
    }
    lines.pushBack({detail::premakeString("targetname", targetName), detail::premakeString("architecture", "x64"),
                    detail::premakeString("language", "C++"), detail::premakeString("cppdialect", "C++17"), detail::premakeString("characterset", "MBCS"),
                    detail::premakeFileList("files", (Vector<Filename>)files, baseDir), detail::generateConfigurations(configurations, baseDir)});
    return String::join(lines, "\n");
  }

  SerializedObject BuildTool::exportConfig() const {
    SerializedObject s;
    write(s.get("project"), projectName);
    write(s.get("projectDir"), projectDirectory);
    write(s.get("configurations"), configurations);
    return s;
  }

  void BuildTool::readConfig(SerializedObject const & s) {
    read(s.get("project"), projectName);
    read(s.get("projectDir"), projectDirectory);
    read(s.get("configurations"), configurations);
  }

  SerializedObject Serializer<BuildConfiguration>::write(BuildConfiguration const & o, ...) {
    SerializedObject s;
    if (o.staticRuntime != -1)
      s.get("static-runtime").write(o.staticRuntime);
    if (o.debugRuntime != -1)
      s.get("debug-runtime").write(o.debugRuntime);
    if (o.symbols != -1)
      s.get("symbols").write(o.symbols);
    if (o.optimize != -1)
      s.get("optimize").write(o.optimize);
    if (o.editAndContinue != -1)
      s.get("edit-and-continue").write(o.editAndContinue);
    if (o.floatModel != FloatingPointModel_Unspecified)
      s.get("fp-model").write(o.floatModel);
    if (o.warningLevel != WarningLevel_Unspecified)
      s.get("warnings").write(o.warningLevel);
    if (o.flags.size())
      s.get("flags").write(o.flags);
    if (o.buildoptions.size())
      s.get("build-options").write(o.buildoptions);
    if (o.defines.size())
      s.get("defines").write(o.defines);
    if (o.links.size())
      s.get("links").write(o.links);
    if (o.libraryDirectories.size())
      s.get("library-dirs").write(o.libraryDirectories);
    if (o.includeDirectories.size())
      s.get("include-dirs").write(o.includeDirectories);
    if (o.intermediateDirectory.length())
      s.get("intermediate-dir").write(o.intermediateDirectory);
    if (o.targetDirectory.length())
      s.get("target-directory").write(o.targetDirectory);
    if (o.debugDirectory.length())
      s.get("debug-directory").write(o.debugDirectory);
    return s;
  }

  bool Serializer<BuildConfiguration>::read(SerializedObject const & s, BuildConfiguration & o, ...) {
    s.get("static-runtime").readOrConstruct(o.staticRuntime, -1);
    s.get("debug-runtime").readOrConstruct(o.debugRuntime, -1);
    s.get("symbols").readOrConstruct(o.symbols, -1);
    s.get("optimize").readOrConstruct(o.optimize, -1);
    s.get("edit-and-continue").readOrConstruct(o.editAndContinue, -1);
    s.get("fp-model").readOrConstruct(o.floatModel, FloatingPointModel_Unspecified);
    s.get("warnings").readOrConstruct(o.warningLevel, WarningLevel_Default);
    s.get("flags").readOrConstruct(o.flags);
    s.get("build-options").readOrConstruct(o.buildoptions);
    s.get("defines").readOrConstruct(o.defines);
    s.get("links").readOrConstruct(o.links);
    s.get("library-dirs").readOrConstruct(o.libraryDirectories);
    s.get("include-dirs").readOrConstruct(o.includeDirectories);
    s.get("intermediate-dir").readOrConstruct(o.intermediateDirectory);
    s.get("target-directory").readOrConstruct(o.targetDirectory);
    s.get("debug-directory").readOrConstruct(o.debugDirectory);
    return true;
  }

  SerializedObject Serializer<BuildTool::Module>::write(BuildTool::Module const & o, ...) {
    SerializedObject s;
    if (o.name.length())
      s.get("name").write(o.name);
    if (o.group.length())
      s.get("group").write(o.group);
    if (o.moduleDirectory.length())
      s.get("directory").write(o.moduleDirectory);
    if (o.targetName.length())
      s.get("target-name").write(o.targetName);
    if (o.targetType != BuildTargetType_Unknown)
      s.get("target-type").write(o.targetType);
    if (o.files.size())
      s.get("files").write(o.files);
    if (o.configurations.size())
      s.get("configurations").write(o.configurations);
    return s;
  }

  bool Serializer<BuildTool::Module>::read(SerializedObject const & s, BuildTool::Module & o, ...) {
    s.get("name").readOrConstruct(o.name);
    s.get("directory").readOrConstruct(o.moduleDirectory);
    s.get("group").readOrConstruct(o.group);
    s.get("target-name").readOrConstruct(o.targetName);
    s.get("target-type").readOrConstruct(o.targetType, BuildTargetType_Unknown);
    s.get("files").readOrConstruct(o.files);
    s.get("configurations").readOrConstruct(o.configurations);
    return true;
  }

  SerializedObject Serializer<BuildTool::ExternalModule>::write(BuildTool::ExternalModule const & o, ...) {
    SerializedObject s;
    s.add("group").write(o.group);
    s.add("script").write(o.script);
    return s;
  }

  bool Serializer<BuildTool::ExternalModule>::read(SerializedObject const & s, BuildTool::ExternalModule & o, ...) {
    s.get("group").readOrConstruct(o.group);
    s.get("script").readOrConstruct(o.script);
    return true;
  }

  SerializedObject Serializer<BuildTool>::write(BuildTool const & o, ...) {
    SerializedObject s;
    s.get("variables").write(o.variables);
    SerializedObject &project = s.get("project");
    project.get("name").write(o.projectName);
    project.get("directory").write(o.projectDirectory);
    s.get("configurations").write(o.configurations);
    s.get("external-modules").write(o.externalModules);
    s.get("modules").write(o.modules);
    return s;
  }

  bool Serializer<BuildTool>::read(SerializedObject const & s, BuildTool & o, ...) {
    s.get("variables").readOrConstruct(o.variables);
    s.get("configurations").readOrConstruct(o.configurations);
    s.get("external-modules").readOrConstruct(o.externalModules);
    s.get("modules").readOrConstruct(o.modules);

    SerializedObject const & project = s.get("project");
    project.get("name").readOrConstruct(o.projectName);
    project.get("directory").readOrConstruct(o.projectDirectory);
    return true;
  }
} // namespace bfc
