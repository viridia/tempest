#ifndef TEMPEST_IMPORT_FSIMPORTER_H
#define TEMPEST_IMPORT_FSIMPORTER_H

#ifndef TEMPEST_IMPORT_IMPORTER_H
#include "tempest/import/importer.hpp"
#endif

#ifndef LLVM_ADT_SMALLSTRING_H
  #include <llvm/ADT/SmallString.h>
#endif

#ifndef LLVM_ADT_STRINGSET_H
  #include <llvm/ADT/StringSet.h>
#endif

#include <unordered_map>
#include <unordered_set>

namespace tempest::import {
  using llvm::StringRef;
  using tempest::sema::graph::Module;
  using tempest::source::ProgramSource;

  /** An import path which points to a directory in the filesystem. */
  class FileSystemImporter : public Importer {
  public:
    FileSystemImporter(StringRef path) : _path(path) {}

    Module* load(StringRef qualifiedName);

  private:
    llvm::SmallString<128> _path;
    llvm::StringMap<llvm::StringSet<>> _dirs;
  };
}

#endif
