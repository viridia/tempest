#ifndef TEMPEST_IMPORT_PACKAGEMGR_H
#define TEMPEST_IMPORT_PACKAGEMGR_H

#ifndef TEMPEST_IMPORT_IMPORTER_H
#include "tempest/import/importer.hpp"
#endif

#ifndef LLVM_ADT_STRINGMAP_H
  #include <llvm/ADT/StringMap.h>
#endif

#ifndef LLVM_ADT_SMALLVECTOR_H
  #include <llvm/ADT/SmallVector.h>
#endif

namespace tempest::import {
  using llvm::StringRef;
  using tempest::sema::graph::Module;
  using tempest::source::Location;

  /** Keeps track of which modules have been imported and where they are. */
  class ImportMgr {
  public:
    ~ImportMgr();

    /** Add a path to the list of module search paths. The path can either be
        a directory, or a bitcode library file. */
    void addImportPath(StringRef path);

    /** Explicitly add a module to the module map, but don't load it. */
    void addModule(Module * mod);

    /** Given a fully-qualified name to a symbol, load the module containing
        that symbol and return it. */
    Module* loadModule(StringRef qualifiedName);

    /** Load a module relative to another module. 'qname' is the relative path without the
        leading dots. 'baseName' is the name we're importing relative to. 'parentLevels' is
        the number of leading dots in the relative path. */
    Module* loadModuleRelative(
        const Location& loc, StringRef baseName, size_t parentLevels, StringRef qname);

    /** Get the module from the module cache using this exact name. */
    Module* getCachedModule(StringRef moduleName);

  private:
    typedef llvm::StringMap<Module *> ModuleMap;
    typedef llvm::SmallVector<Importer *, 8> PathList;

    // Set of all modules, arranged by package
    ModuleMap _modules;

    // Set of directories to search for modules.
    PathList _importers;
  };
}

#endif
