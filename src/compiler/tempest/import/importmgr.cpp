#include "tempest/error/diagnostics.hpp"
#include "tempest/import/fsimporter.hpp"
#include "tempest/import/importmgr.hpp"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include <cassert>

namespace tempest::import {
  using namespace llvm;
  using namespace llvm::sys;
  using tempest::error::diag;

  // static cl::opt<bool>
  // ShowImports("show-imports", cl::desc("Display imports"));

  ImportMgr::~ImportMgr() {
    for (auto imp : _importers) {
      delete imp;
    }
    // TODO: Delete modules?
  }

  void ImportMgr::addImportPath(StringRef path) {
    bool success = false;
    if (fs::is_directory(path, success) && success) {
      _importers.push_back(new FileSystemImporter(path));
  //   } else if (fs::is_regular_file(path, success) == errc::success && success) {
  //     if (path::extension(path) == ".bc") {
  //       importers_.push_back(new ArchiveImporter(path));
  //     } else {
  //       diag.error() << "Unsupported path type: " << path;
  //       exit(-1);
  //     }
    } else {
      diag.error() << "Unsupported path type: " << path;
      exit(-1);
    }
  }

  Module * ImportMgr::getCachedModule(StringRef qname) {
    // First, attempt to search the modules already loaded.
    ModuleMap::iterator it = _modules.find(qname);
    if (it != _modules.end()) {
      return it->second;
    }

    return nullptr;
  }

  Module * ImportMgr::loadModule(StringRef qname) {
    // First, attempt to search the modules already loaded.
    ModuleMap::iterator it = _modules.find(qname);
    if (it != _modules.end()) {
      // if (ShowImports && it->second != NULL) {
      //   diag.debug() << "Import: Found module '" << qname << "' in module cache";
      // }

      return it->second;
    }

    // If it's a regular file.
    Module * mod = NULL;

    // Search each element on the import path list.
    for (PathList::iterator it = _importers.begin(); it != _importers.end(); ++it) {
      mod = (*it)->load(qname);
      if (mod) {
        break;
      }
    }

    // if (!mod && ShowImports) {
    //   diag.debug() << "Import: module '" << qname << "' NOT FOUND";
    // }

    // Regardless of whether the module was found, record this result in the map
    // so we don't have to look it up again.
    _modules[qname] = mod;

    // Do this *after* we add the module to the map, because createMembers() might
    // do other module lookups.
    // if (mod != NULL) {
    //   mod->createMembers();
    // }
    return mod;
  }

  void ImportMgr::addModule(Module * mod) {
    assert(_modules.find(mod->name()) == _modules.end());
    _modules[mod->name()] = mod;
  }
}
