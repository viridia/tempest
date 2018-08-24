#include "../catch.hpp"
#include "tempest/error/diagnostics.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"
#include <cstdlib>

using namespace llvm;
using namespace llvm::sys;
using tempest::error::diag;

extern llvm::SmallString<128> execDir;

TEST_CASE("Compilation", "[compilation]") {
  llvm::SmallString<128> compilationDir(execDir);
  path::append(compilationDir, "compilation");

  llvm::SmallString<128> compilerBin(execDir);
  path::remove_filename(compilerBin);
  path::append(compilerBin, "compiler/tempestc");

  llvm::SmallString<128> commandLine(compilerBin);
  commandLine.append(" ");
  commandLine.append(compilationDir);
  commandLine.append("/basic.te");

  diag.info() << "Command line:" << commandLine;
  auto errc = std::system(commandLine.c_str());
  REQUIRE(errc == 0);
}
