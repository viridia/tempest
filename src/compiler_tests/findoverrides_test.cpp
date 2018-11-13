#include "catch.hpp"
#include "semamatcher.hpp"
#include "mockreporter.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include "tempest/sema/pass/findoverrides.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "tempest/sema/pass/resolvetypes.hpp"

using namespace tempest::compiler;
using namespace tempest::sema::graph;
using namespace tempest::sema::pass;
// using namespace std;
// using namespace llvm;
using tempest::parse::Parser;
// using tempest::source::Location;

class TestSource : public tempest::source::StringSource {
public:
  TestSource(llvm::StringRef source)
    : tempest::source::StringSource("test.txt", source)
  {}
};

namespace {
  /** Parse a module definition and apply buildgraph & nameresolution pass. */
  std::unique_ptr<Module> compile(CompilationUnit &cu, const char* srcText) {
    diag.reset();
    auto mod = std::make_unique<Module>(std::make_unique<TestSource>(srcText), "test.mod");
    Parser parser(mod->source(), mod->astAlloc());
    CompilationUnit::theCU = &cu;
    auto result = parser.module();
    mod->setAst(result);
    BuildGraphPass bgPass(cu);
    bgPass.process(mod.get());
    NameResolutionPass nrPass(cu);
    nrPass.process(mod.get());
    ResolveTypesPass rtPass(cu);
    rtPass.process(mod.get());
    FindOverridesPass foPass(cu);
    foPass.process(mod.get());
    CompilationUnit::theCU = nullptr;
    REQUIRE(diag.errorCount() == 0);
    return mod;
  }

  /** Parse a module definition with errors and return the error message. */
  std::string compileError(CompilationUnit &cu, const char* srcText) {
    UseMockReporter umr;
    auto mod = std::make_unique<Module>(std::make_unique<TestSource>(srcText), "test.mod");
    Parser parser(mod->source(), mod->astAlloc());
    CompilationUnit::theCU = &cu;
    auto result = parser.module();
    mod->setAst(result);
    BuildGraphPass bgPass(cu);
    bgPass.process(mod.get());
    NameResolutionPass nrPass(cu);
    nrPass.process(mod.get());
    ResolveTypesPass rtPass(cu);
    rtPass.process(mod.get());
    FindOverridesPass foPass(cu);
    foPass.process(mod.get());
    CompilationUnit::theCU = nullptr;
    REQUIRE(MockReporter::INSTANCE.errorCount() > 0);
    return MockReporter::INSTANCE.content().str();
  }
}

TEST_CASE("FindOverrides", "[sema][override]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Trait requirements") {
    compile(cu,
      "trait X {\n"
      "  f(a0: f32, a1: bool) -> i32;\n"
      "}\n"
      "class A implements X {\n"
      "  f(a0: f32, a1: bool) -> i32 { return 7; }\n"
      "}\n"
    );
  }

  SECTION("Conflicting definitions") {
    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  f: i32;\n"
          "  f(a0: f32, a1: bool) -> i32 { return 7; }\n"
          "}\n"
      ),
      Catch::Contains("Conflicting definitions for 'f'"));

    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  class f {}\n"
          "  f(a0: f32, a1: bool) -> i32 { return 7; }\n"
          "}\n"
      ),
      Catch::Contains("Conflicting definitions for 'f'"));

    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  class f {}\n"
          "  f: f64;\n"
          "}\n"
      ),
      Catch::Contains("Conflicting definitions for 'f'"));
  }

  SECTION("Multiple variable definitions") {
    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  f: i32;\n"
          "  f: i16;\n"
          "}\n"
      ),
      Catch::Contains("Multiple definitions for not allowed for 'f'"));
  }

  SECTION("Conflicting inherited definitions") {
    REQUIRE_THAT(
      compileError(cu,
          "trait X {\n"
          "  f(a0: f32, a1: bool) -> i32;\n"
          "}\n"
          "class A implements X {\n"
          "  f: i32;\n"
          "}\n"
      ),
      Catch::Contains("Definition of 'f' conflicts with inherited definition."));

    REQUIRE_THAT(
      compileError(cu,
          "trait X {\n"
          "  class f {}\n"
          "}\n"
          "class A implements X {\n"
          "  f: i32;\n"
          "}\n"
      ),
      Catch::Contains("Definition of 'f' conflicts with inherited definition."));

    REQUIRE_THAT(
      compileError(cu,
          "trait X {\n"
          "  f: i32;\n"
          "}\n"
          "class A implements X {\n"
          "  f(a0: f32, a1: bool) -> i32;\n"
          "}\n"
      ),
      Catch::Contains("Definition of 'f' conflicts with inherited definition."));
  }

  SECTION("Conflicting method signatures") {
    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  f(a0: f32, a1: bool) -> i32;\n"
          "  f(a0: f32, a1: bool) -> i32;\n"
          "}\n"
      ),
      Catch::Contains("Method definition 'f' is indistinguishable from earlier method"));

    REQUIRE_THAT(
      compileError(cu,
          "fn f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "fn f(a0: f32, a1: bool) -> i32 { 0 }\n"
      ),
      Catch::Contains("Method definition 'f' is indistinguishable from earlier method"));
  }

  SECTION("Non-conflicting method signatures") {
    compile(cu,
        "interface A {\n"
        "  f(a0: f32, a1: bool) -> i32;\n"
        "  f(a0: f64, a1: bool) -> i32;\n"
        "}\n"
    );
    compile(cu,
        "interface A {\n"
        "  f(a0: f32, a1: bool) -> i32;\n"
        "  f(a0: f32, a1: bool) -> i16;\n"
        "}\n"
    );
    compile(cu,
        "fn f(a0: f32, a1: bool) -> i32 { 0 }\n"
        "fn f(a0: f64, a1: bool) -> i32 { 0 }\n"
    );
  }

  SECTION("Invalid override") {
    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  override f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
      ),
      Catch::Contains("Method 'f' declared as 'override' but there is no inherited method"));

    REQUIRE_THAT(
      compileError(cu,
          "interface A {\n"
          "  f(a0: f32, a1: bool) -> i32;\n"
          "}\n"
          "interface B extends A {\n"
          "  override f(a0: f32, a1: bool) -> i32;\n"
          "}\n"
      ),
      Catch::Contains("Only class methods can be declared as 'override'"));

    REQUIRE_THAT(
      compileError(cu,
          "trait A {\n"
          "  f(a0: f32, a1: bool) -> i32;\n"
          "}\n"
          "trait B extends A {\n"
          "  override f(a0: f32, a1: bool) -> i32;\n"
          "}\n"
      ),
      Catch::Contains("Only class methods can be declared as 'override'"));

    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
          "abstract class B extends A {\n"
          "  abstract override f(a0: f32, a1: bool) -> i32;\n"
          "}\n"
      ),
      Catch::Contains("Method overrides cannot be abstract"));

    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
          "class B extends A {\n"
          "  private override f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
      ),
      Catch::Contains("Method overrides cannot be private"));

    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  final f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
          "class B extends A {\n"
          "  override f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
      ),
      Catch::Contains("cannot override inherited method declared as 'final'"));

    REQUIRE_THAT(
      compileError(cu,
          "final class A {\n"
          "  f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
          "class B extends A {\n"
          "  override f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
      ),
      Catch::Contains("cannot override inherited method declared in final class"));

    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
          "class B extends A {\n"
          "  f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
      ),
      Catch::Contains("must be declared with 'override'"));

    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  private f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
          "class B extends A {\n"
          "  override f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
      ),
      Catch::Contains("overrides inherited method which is not visible"));

    REQUIRE_THAT(
      compileError(cu,
          "class A {\n"
          "  f(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
          "class B extends A {\n"
          "  override f!(a0: f32, a1: bool) -> i32 { 0 }\n"
          "}\n"
      ),
      Catch::Contains("does not match the type signature"));
  }

  SECTION("Parameterized overrides") {
    auto mod = compile(cu,
        "abstract class A[T] {\n"
        "  abstract f(a0: T, a1: bool) -> i32;\n"
        "}\n"
        "class B extends A[f32] {\n"
        "  override f(a0: f32, a1: bool) -> i32 { 0 }\n"
        "}\n"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    REQUIRE(td->methods().size() == 1);

    mod = compile(cu,
        "interface A[T] {\n"
        "  f(a0: T, a1: bool) -> i32;\n"
        "}\n"
        "class B implements A[f32] {\n"
        "  f(a0: f32, a1: bool) -> i32 { 0 }\n"
        "}\n"
    );
    td = cast<TypeDefn>(mod->members().back());
    REQUIRE(td->methods().size() == 1);
  }

  SECTION("Method table") {
    auto mod = compile(cu,
        "class A {\n"
        "  f(a0: f32, a1: bool) -> i32 { 0 }\n"
        "}\n"
        "class B extends A {\n"
        "  final a(a0: f32, a1: bool) -> i32 { 0 }\n"
        "  static b(a0: f32, a1: bool) -> i32 { 0 }\n"
        "  private c(a0: f32, a1: bool) -> i32 { 0 }\n"
        "  override f(a0: f32, a1: bool) -> i32 { 0 }\n"
        "}\n"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    REQUIRE(td->methods().size() == 1);
  }

  SECTION("Interface table") {
    auto mod = compile(cu,
        "interface A {\n"
        "  f(a0: f32, a1: bool) -> i32;\n"
        "}\n"
        "interface B {\n"
        "  f(a0: f32, a1: bool) -> i32;\n"
        "}\n"
        "class C implements A, B {\n"
        "  final a(a0: f32, a1: bool) -> i32 { 0 }\n"
        "  static b(a0: f32, a1: bool) -> i32 { 0 }\n"
        "  private c(a0: f32, a1: bool) -> i32 { 0 }\n"
        "  f(a0: f32, a1: bool) -> i32 { 0 }\n"
        "}\n"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    REQUIRE(td->interfaceMethods().size() == 2);
    REQUIRE(td->interfaceMethods()[0].size() == 1);
    REQUIRE(td->interfaceMethods()[1].size() == 1);
  }

  SECTION("Missing interface definition") {
    REQUIRE_THAT(
      compileError(cu,
          "interface A {\n"
          "  f(a0: f32, a1: bool) -> i32;\n"
          "}\n"
          "interface B {\n"
          "  f(a0: f32, a1: bool) -> i32;\n"
          "}\n"
          "class C implements A, B {\n"
          "}\n"
      ),
      Catch::Contains("but missing definition for"));
  }

  SECTION("Missing abstract override") {
    REQUIRE_THAT(
      compileError(cu,
          "abstract class A {\n"
          "  abstract g(a0: f32, a1: bool) -> i32;\n"
          "}\n"
          "class C extends A {\n"
          "}\n"
      ),
      Catch::Contains("lacks concrete implementation"));
  }
}
