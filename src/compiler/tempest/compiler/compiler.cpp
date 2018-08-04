#include "tempest/error/diagnostics.hpp"
#include "tempest/compiler/compiler.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include "tempest/sema/pass/loadimports.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

#include <vector>

using namespace llvm;
using namespace llvm::sys;
using namespace std;
using namespace tempest::sema::pass;

cl::list<string> PackageRoots("source-root", llvm::cl::desc("Source package root directory"));

cl::list<std::string> InputFilenames(
    cl::Positional, cl::desc("<Input files or dirs>"), cl::OneOrMore);

namespace tempest::compiler {
  using tempest::error::diag;

  Compiler::Compiler() {}

  int Compiler::run() {
    addSourceFiles();
    if (_cu.sourceModules().empty()) {
      diag.error() << "No input files found.";
      return 1;
    }
    if (diag.errorCount() == 0) {
      LoadImportsPass pass(_cu);
      pass.run();
    }
    if (diag.errorCount() == 0) {
      BuildGraphPass pass(_cu);
      pass.run();
    }
    if (diag.errorCount() == 0) {
      NameResolutionPass pass(_cu);
      pass.run();
    }
    return diag.errorCount() == 0 ? 0 : 1;
  }

  int Compiler::addSourceFiles() {
    SmallString<128> curDir;
    vector<SmallString<128>> rootPaths;
    auto err = fs::current_path(curDir);
    if (err) {
      diag.error() << "current_path failed with error: " << err;
      return 1;
    }

    // Compute absolute paths for all package roots
    for (auto root : PackageRoots) {
      SmallString<128> srcRoot;
      SmallString<128> realSrcRoot;
      if (path::is_absolute(root)) {
        srcRoot = root;
      } else {
        path::append(srcRoot, curDir, root);
      }
      auto err = fs::real_path(srcRoot, realSrcRoot);
      if (err) {
        diag.error() << "real_path failed with error: " << err;
        return 1;
      }
      rootPaths.push_back(realSrcRoot);
    }
    if (rootPaths.empty()) {
      rootPaths.push_back(curDir);
    }

    // Process input filenames
    for (auto input : InputFilenames) {
      SmallString<128> srcPath;
      SmallString<128> realSrcPath;

      // Check for source file with extension
      if (!fs::exists(input)) {
        diag.error() << "input file not found: " << input;
        return 1;
      }

      // Make input path absolute
      if (path::is_absolute(input)) {
        srcPath = input;
      } else {
        path::append(srcPath, curDir, input);
      }
      auto err = fs::real_path(srcPath, realSrcPath);
      if (err) {
        diag.error() << "real_path failed with error: " << err;
        return 1;
      }

      // Strip extension off of absolute source path.
      path::replace_extension(realSrcPath, "");

      // Find a package root that is a prefix of this path.
      SmallString<128> *srcRoot = nullptr;
      for (auto root : rootPaths) {
        if (realSrcPath.startswith(root)) {
          srcRoot = &root;
          break;
        }
      }
      if (!srcRoot) {
        diag.error() << "Cannot determine source package root for path: " << input;
        return 1;
      }

      // Strip off the package root prefix
      auto it = realSrcPath.begin();
      std::advance(it, srcRoot->size());
      while (path::is_separator(*it) && it < realSrcPath.end()) {
        it += 1;
      }

      // Transform slashes into dots
      SmallString<128> modName;
      for (StringRef::const_iterator ch = it; ch != realSrcPath.end(); ++ch) {
        if (path::is_separator(*ch)) {
          modName.push_back('.');
        } else {
          modName.push_back(*ch);
        }
      }

      bool success = false;
      if (fs::is_directory(input, success) && success) {
        assert(false && "implement source directory");
      } else {
        auto ext = path::extension(input);
        if (ext == ".te") {
          _cu.addSourceFile(input, modName);
        } else {
          diag.error() << "Unknown extension: '" << ext << "': " << input;
          return 1;
        }
      }
    }
    return 0;
  }
}
