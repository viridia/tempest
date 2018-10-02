#include "tempest/error/diagnostics.hpp"
#include "tempest/compiler/compiler.hpp"
#include "tempest/gen/cgmodule.hpp"
#include "tempest/gen/codegen.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include "tempest/sema/pass/expandspecialization.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "tempest/sema/pass/loadimports.hpp"
#include "tempest/sema/pass/resolvetypes.hpp"
#include "tempest/sema/pass/resolvetypes.hpp"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetOptions.h"

#include <vector>

using namespace llvm;
using namespace llvm::sys;
using namespace std;
using namespace tempest::sema::pass;

cl::list<string> PackageRoots("source-root", llvm::cl::desc("Source package root directory"));
cl::list<std::string> InputFilenames(
    cl::Positional, cl::desc("<Input files or dirs>"), cl::OneOrMore);
cl::opt<string> OutputDir("d", llvm::cl::desc("Output directory"));
cl::opt<string> OutputFile("o", llvm::cl::desc("Output file"));

namespace tempest::compiler {
  using tempest::error::diag;

  Compiler::Compiler() {
    llvm::InitializeNativeTarget();
  }

  int Compiler::run() {
    addSourceFiles();
    CompilationUnit::theCU = &_cu;
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
    if (diag.errorCount() == 0) {
      ResolveTypesPass pass(_cu);
      pass.run();
    }
    if (diag.errorCount() == 0) {
      ExpandSpecializationPass pass(_cu);
      pass.run();
    }
    assert(!_cu.outputFile().empty());
    assert(!_cu.outputModName().empty());
    if (diag.errorCount() == 0) {
      llvm::LLVMContext context;
      gen::CodeGen gen(context);
      auto mod = gen.createModule(_cu.outputModName());
      gen.genSymbols(_cu.symbols());
      selectTarget(mod);
      if (diag.errorCount() == 0) {
        llvm::verifyModule(*mod->irModule(), &(llvm::errs()));
        outputModule(mod);
      }
    }

    CompilationUnit::theCU = nullptr;
    return diag.errorCount() == 0 ? 0 : 1;
  }

  void Compiler::selectTarget(tempest::gen::CGModule* mod) {
    std::string error;
    auto targetTriple = sys::getDefaultTargetTriple();
    auto target = TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
      diag.error() << error;
      return;
    }

    auto cpu = "generic";
    auto features = "";

    TargetOptions opt;
    auto rm = Optional<Reloc::Model>();
    auto targetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, rm);

    mod->irModule()->setDataLayout(targetMachine->createDataLayout());
    mod->irModule()->setTargetTriple(targetTriple);
  }

  void Compiler::outputModule(tempest::gen::CGModule* mod) {
    if (path::has_parent_path(_cu.outputFile())) {
      SmallString<128> outDir = path::parent_path(_cu.outputFile());
      if (fs::create_directories(outDir)) {
        // TODO: Decode error code.
        diag.fatal() << "Cannot create output directory '" << outDir << ".";
        return;
      }
    }

    std::error_code err;
    llvm::raw_fd_ostream binOut(_cu.outputFile(), err, fs::OpenFlags::F_None);
    if (err) {
      // TODO: Decode error code.
      diag.fatal() << "Cannot write output file '" << _cu.outputFile() << ".";
      return;
    }

    llvm::WriteBitcodeToFile(mod->irModule(), binOut);
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

    SmallString<128> outFile;
    if (!OutputFile.empty()) {
      outFile.assign(OutputFile.begin(), OutputFile.end());
    } else if (InputFilenames.size() == 1) {
      SmallString<128> outPath;
      outFile = path::filename(InputFilenames[0]);
      path::replace_extension(outFile, ".bc");
      if (!OutputDir.empty()) {
        path::append(outPath, OutputDir, outFile);
      } else {
        path::append(outPath, curDir, outFile);
      }
    } else {
      diag.error() << "output file not specified.";
      return 1;
    }

    assert(!outFile.empty());
    _cu.outputFile() = outFile;
    path::filename(outFile);
    path::replace_extension(outFile, "");
    _cu.outputModName() = outFile;

    // Process input filenames
    for (auto input : InputFilenames) {
      SmallString<128> srcPath;
      SmallString<128> realSrcPath;

      // Make input path absolute
      if (path::is_absolute(input)) {
        srcPath = input;
      } else {
        path::append(srcPath, curDir, input);
      }

      path::remove_dots(srcPath);

      // Check for source file with extension
      if (!fs::exists(srcPath)) {
        diag.error() << "input file not found: " << input;
        return 1;
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
