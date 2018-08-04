#include "tempest/error/diagnostics.hpp"
#include "tempest/import/fsimporter.hpp"
#include "tempest/source/programsource.hpp"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

namespace tempest::import {
  using namespace llvm;
  using namespace llvm::sys;
  using tempest::sema::graph::Module;
  using tempest::error::diag;

  static auto TEMPEST_SOURCE_FILE_EXTENSION = ".te";

    // Module(
    //     std::unique_ptr<source::ProgramSource>& source,
    //     const llvm::StringRef& name)

  Module* FileSystemImporter::load(StringRef qualName, bool& isPackage) {
    // Transform dots into path separators to get the relative path.
    SmallString<128> relpath;
    for (StringRef::const_iterator ch = qualName.begin(); ch != qualName.end(); ++ch) {
      if (*ch == '.') {
        relpath.push_back('/');
      } else {
        relpath.push_back(*ch);
      }
    }

    // Transform dots into path separators.
    SmallString<128> filepath(_path);
    path::append(filepath, Twine(relpath));
    SmallString<128> filepathWithExtension(filepath);
    path::replace_extension(filepathWithExtension, TEMPEST_SOURCE_FILE_EXTENSION);

    // Check for source file
    if (fs::exists(filepathWithExtension)) {
      isPackage = false;
      // We want to make sure that the file that we are looking for has the exact same
      // case, even on a case-insensitive filesystem. The only easy way to do this is
      // to get all the filanems in the directory and then string compare to see if the
      // match is exact.

      // We need the dirname and the filename
      StringRef filename = path::filename(filepathWithExtension);
      SmallString<128> dirname = path::parent_path(filepathWithExtension);

      // See if the directory contents are already cached.
      auto it = _dirs.find(dirname);
      if (it == _dirs.end()) {
        std::error_code ec;
        fs::directory_iterator iter(dirname, ec, false);
        llvm::StringSet<> entries;
        while (ec) {
          if (iter->status()->type() == fs::file_type::regular_file) {
            diag.info() << iter->path();
            entries.insert(iter->path());
          }
          iter.increment(ec);
        }

        bool inserted;
        std::tie(it, inserted) = _dirs.insert(std::make_pair(dirname, entries));
      }

      auto fit = it->second.find(filename);
      if (fit == it->second.end()) {
        // No match.
        return nullptr;
      }

      auto source = std::make_unique<source::FileSource>(filepathWithExtension, relpath);
      auto module = new Module(std::move(source), qualName);
      module->setGroup(sema::graph::ModuleGroup::IMPORT_SOURCE);
      // if (ShowImports) {
      //   diag.debug() << "Import: Found source module '" << qualName << "' at " << filepath;
      // }

      return module;

      // Parser parser(module->source(), module);
      // if (parser.parse()) {
      //   return module;
      // } else {
      //   delete module;
      //   // module = NULL;
      //   // return nullptr;
      // }
    } else if (fs::is_directory(filepath)) {
      // If the original path, with no extension, is a directory, then treat it as a package.
      isPackage = true;
    }

    return nullptr;
  }
}
