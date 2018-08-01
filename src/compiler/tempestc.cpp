#include "tempest/compiler/compiler.hpp"
#include "llvm/Support/CommandLine.h"

using tempest::compiler::Compiler;

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv);
  Compiler compiler;
  return compiler.run();
}
