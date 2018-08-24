#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

llvm::SmallString<128> execDir;

int main(int argc, char* argv[]) {
  // Compute the direction in which the test runner is running
  llvm::sys::fs::current_path(execDir);
  if (llvm::sys::path::is_absolute(argv[0])) {
    execDir = argv[0];
  } else {
    llvm::sys::path::append(execDir, argv[0]);
  }
  llvm::sys::path::remove_filename(execDir);

  return Catch::Session().run( argc, argv );
}
