#include "catch.hpp"
#include "semamatcher.hpp"
#include "mockreporter.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "llvm/Support/Casting.h"
#include <iostream>

using namespace tempest::compiler;
using namespace tempest::sema::graph;
using namespace tempest::sema::pass;
using namespace std;
using namespace llvm;
using tempest::parse::Parser;
using tempest::source::Location;

class TestSource : public tempest::source::StringSource {
public:
  TestSource(llvm::StringRef source)
    : tempest::source::StringSource("test.txt", source)
  {}
};

namespace {
  /** Parse a module definition and apply buildgraph & nameresolution pass. */
  std::unique_ptr<Module> compile(CompilationUnit &cu, const char* srcText) {
    auto mod = std::make_unique<Module>(std::make_unique<TestSource>(srcText), "test.mod");
    Parser parser(mod->source(), mod->astAlloc());
    auto result = parser.module();
    mod->setAst(result);
    BuildGraphPass bgPass(cu);
    bgPass.process(mod.get());
    NameResolutionPass nrPass(cu);
    nrPass.process(mod.get());
    return mod;
  }

  /** Parse a module definition with errors and return the error message. */
  std::string compileError(CompilationUnit &cu, const char* srcText) {
    UseMockReporter umr;
    auto mod = std::make_unique<Module>(std::make_unique<TestSource>(srcText), "test.mod");
    Parser parser(mod->source(), mod->astAlloc());
    auto result = parser.module();
    mod->setAst(result);
    BuildGraphPass bgPass(cu);
    bgPass.process(mod.get());
    NameResolutionPass nrPass(cu);
    nrPass.process(mod.get());
    REQUIRE(MockReporter::INSTANCE.errorCount() > 0);
    return MockReporter::INSTANCE.content().str();
  }
}

TEST_CASE("NameResolution.Type", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Resolve primitive type") {
    auto mod = compile(cu, "let X: i32;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::INTEGER);
    REQUIRE_THAT(vdef->type(), TypeEQ("i32"));
  }

  SECTION("Resolve type with error") {
    REQUIRE_THAT(
      compileError(cu, "let X: p32;"),
      Catch::Contains("Name not found: p32"));
  }

  SECTION("Resolve type with a non-type name") {
    REQUIRE_THAT(
      compileError(cu, "let X: i32; let Y: X;"),
      Catch::Contains("Expecting a type name"));
  }

  SECTION("Resolve union type") {
    auto mod = compile(cu, "let X: i32 | f32 | bool;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::UNION);
    REQUIRE(cast<UnionType>(vdef->type())->members.size() == 3);
    REQUIRE_THAT(vdef->type(), TypeEQ("bool | i32 | f32"));
  }

  SECTION("Resolve union of singletons") {
    auto mod = compile(cu, "let X: 1 | 2 | 3;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::UNION);
    REQUIRE(cast<UnionType>(vdef->type())->members.size() == 3);
    REQUIRE_THAT(vdef->type(), TypeEQ("1 | 2 | 3"));
  }

  SECTION("Resolve union type with error") {
    REQUIRE_THAT(
      compileError(cu, "let X: i32 | f32 | p32;"),
      Catch::Contains("Name not found: p32"));
  }

  SECTION("Resolve tuple type") {
    auto mod = compile(cu, "let X: (i32, f32, bool);");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::TUPLE);
    REQUIRE(cast<TupleType>(vdef->type())->members.size() == 3);
    REQUIRE_THAT(vdef->type(), TypeEQ("(i32, f32, bool)"));
  }

  SECTION("Resolve alias type") {
    auto mod = compile(cu,
      "type A = i32 | void;\n"
      "let X: A;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::ALIAS);
    REQUIRE_THAT(vdef->type(), TypeEQ("type A"));
    auto udt = static_cast<const UserDefinedType*>(vdef->type());
    REQUIRE_THAT(udt->defn()->aliasTarget(), TypeEQ("void | i32"));
  }
}

TEST_CASE("NameResolution.Class", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Resolve class type") {
    auto mod = compile(cu,
      "class A {}\n"
      "let X: A;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::CLASS);
    REQUIRE_THAT(vdef->type(), TypeEQ("class A"));
  }

  SECTION("Resolve class member explicitly") {
    auto mod = compile(cu,
      "class A {\n"
      "  class B {}\n"
      "}\n"
      "let X: A.B;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::CLASS);
    REQUIRE_THAT(vdef->type(), TypeEQ("class B"));
  }

  SECTION("Cannot access private member outside of class") {
    REQUIRE_THAT(
      compileError(cu,
        "class A {\n"
        "  private class B {}\n"
        "}\n"
        "let X: A.B;"
      ),
      Catch::Contains("Cannot access private member 'B'"));
  }

  SECTION("Cannot access protected member outside of class") {
    REQUIRE_THAT(
      compileError(cu,
        "class A {\n"
        "  protected class B {}\n"
        "}\n"
        "let X: A.B;"
      ),
      Catch::Contains("Cannot access protected member 'B'"));
  }

  SECTION("Resolve class member from within class") {
    auto mod = compile(cu,
      "class A {\n"
      "  class B {}\n"
      "  const X: B;\n"
      "}"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    REQUIRE(td->kind == Defn::Kind::TYPE);
    auto vdef = cast<ValueDefn>(td->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE(vdef->isConstant());
    REQUIRE(vdef->type()->kind == Type::Kind::CLASS);
    REQUIRE_THAT(vdef->type(), TypeEQ("class B"));
  }

  SECTION("Resolve private class member from within class") {
    auto mod = compile(cu,
      "class A {\n"
      "  private class B {}\n"
      "  const X: B;\n"
      "}"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    REQUIRE(td->kind == Defn::Kind::TYPE);
    auto vdef = cast<ValueDefn>(td->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE(vdef->type()->kind == Type::Kind::CLASS);
    REQUIRE_THAT(vdef->type(), TypeEQ("class B"));
  }

  SECTION("Resolve inherited class member") {
    auto mod = compile(cu,
      "class A {\n"
      "  class B {}\n"
      "}\n"
      "class C extends A {}\n"
      "let X: C.B;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::CLASS);
    REQUIRE_THAT(vdef->type(), TypeEQ("class B"));
  }

  SECTION("Resolve inherited specialized class member") {
    auto mod = compile(cu,
      "class A[T] {\n"
      "  class B {}\n"
      "}\n"
      "class C extends A[i32] {}\n"
      "let X: C.B;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::SPECIALIZED);
    REQUIRE_THAT(vdef->type(), TypeEQ("class B[i32]"));
  }

  SECTION("Specialize non-generic") {
    REQUIRE_THAT(
      compileError(cu, "class A {} let X: A[i32]"),
      Catch::Contains("Definition 'A' is not generic"));
  }

  SECTION("Specialize generic with incorrect parameter count") {
    REQUIRE_THAT(
      compileError(cu, "class A[T] {} let X: A[i32, i32]"),
      Catch::Contains("Generic definition 'A' requires 1 type arguments, 2 were provided"));
  }

  SECTION("Class with generic type constraint") {
    auto mod = compile(cu,
      "class A[T: f32] {}\n"
    );
    auto td = cast<TypeDefn>(mod->members().front());
    auto param = td->typeParams().front();
    REQUIRE(param->subtypeConstraints().size() == 1);
  }

  SECTION("Class with generic type sum constraint") {
    auto mod = compile(cu,
      "class A[T: f32 & i32] {}\n"
    );
    auto td = cast<TypeDefn>(mod->members().front());
    auto param = td->typeParams().front();
    REQUIRE(param->subtypeConstraints().size() == 2);
  }

  SECTION("Implicit specialization") {
    auto mod = compile(cu,
      "class A[T] {\n"
      "  x: A;\n"
      "  y: A | i32;\n"
      "  x: (A, i32);\n"
      "}\n"
    );
    auto td = cast<TypeDefn>(mod->members().front());
    auto vdef = cast<ValueDefn>(td->members()[1]);
    REQUIRE(vdef->type()->kind == Type::Kind::SPECIALIZED);
    REQUIRE_THAT(vdef->type(), TypeEQ("class A[T]"));

    vdef = cast<ValueDefn>(td->members()[2]);
    REQUIRE_THAT(vdef->type(), TypeEQ("i32 | class A[T]"));

    vdef = cast<ValueDefn>(td->members()[3]);
    REQUIRE_THAT(vdef->type(), TypeEQ("(class A[T], i32)"));
  }
}

TEST_CASE("NameResolution.Interface", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Resolve interface type") {
    auto mod = compile(cu,
      "interface A {}\n"
      "let X: A;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::INTERFACE);
    REQUIRE_THAT(vdef->type(), TypeEQ("interface A"));
  }

  SECTION("Resolve interface member explicitly") {
    auto mod = compile(cu,
      "interface A {\n"
      "  class B {}\n"
      "}\n"
      "let X: A.B;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::CLASS);
    REQUIRE_THAT(vdef->type(), TypeEQ("class B"));
  }

  SECTION("Interface members cannot be private") {
    REQUIRE_THAT(
      compileError(cu,
        "interface A {\n"
        "  private class B {}\n"
        "}\n"
      ),
      Catch::Contains("Interface members cannot be 'private' or 'protected'"));
  }

  SECTION("Interface members cannot be protected") {
    REQUIRE_THAT(
      compileError(cu,
        "interface A {\n"
        "  protected class B {}\n"
        "}\n"
      ),
      Catch::Contains("Interface members cannot be 'private' or 'protected'"));
  }

  SECTION("Resolve inherited interface member") {
    auto mod = compile(cu,
      "interface A {\n"
      "  interface B {}\n"
      "}\n"
      "interface C extends A {}\n"
      "let X: C.B;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::INTERFACE);
    REQUIRE_THAT(vdef->type(), TypeEQ("interface B"));
  }

  SECTION("Redundant interface") {
    REQUIRE_THAT(
      compileError(cu,
          "interface A[T] {}\n"
          "class C implements A[i32], A[i32] {}\n"
      ),
      Catch::Contains("Type implements the same interface more than once"));

    compile(cu,
        "interface A[T] {}\n"
        "class C implements A[i32], A[i8] {}\n"
    );
  }
}

TEST_CASE("NameResolution.Trait", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Class with trait constraint") {
    auto mod = compile(cu,
      "trait X {}\n"
      "class A[T: X] {}\n"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    auto param = td->typeParams().front();
    REQUIRE(param->subtypeConstraints().size() == 1);
  }

  SECTION("Variable cannot be a trait") {
    REQUIRE_THAT(
      compileError(cu,
        "trait A {}\n"
        "let X: A;"
      ),
      Catch::Contains("Variable type cannot be a trait."));
  }

  SECTION("Trait members cannot be private") {
    REQUIRE_THAT(
      compileError(cu,
        "trait A {\n"
        "  private x: i32;\n"
        "}\n"
      ),
      Catch::Contains("Trait members cannot be 'private' or 'protected'"));
  }

  SECTION("Trait members cannot be protected") {
    REQUIRE_THAT(
      compileError(cu,
        "trait A {\n"
        "  protected x: i32;\n"
        "}\n"
      ),
      Catch::Contains("Trait members cannot be 'private' or 'protected'"));
  }

}

TEST_CASE("NameResolution.Struct", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Resolve struct type") {
    auto mod = compile(cu,
      "struct A {}\n"
      "let X: A;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::STRUCT);
    REQUIRE_THAT(vdef->type(), TypeEQ("struct A"));
  }

  SECTION("Resolve struct member explicitly") {
    auto mod = compile(cu,
      "struct A {\n"
      "  struct B {}\n"
      "}\n"
      "let X: A.B;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::STRUCT);
    REQUIRE_THAT(vdef->type(), TypeEQ("struct B"));
  }
}

TEST_CASE("NameResolution.TypeAlias", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Resolve type alias") {
    auto mod = compile(cu,
      "type Optional[T] = T | void;\n"
      "let X: Optional[i32];"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::SPECIALIZED);
    auto sp = cast<SpecializedType>(vdef->type());
    REQUIRE(cast<TypeDefn>(sp->spec->generic())->type()->kind == Type::Kind::ALIAS);
    REQUIRE_THAT(vdef->type(), TypeEQ("type Optional[i32]"));
  }

  SECTION("Resolve class member via alias") {
    auto mod = compile(cu,
      "class A {\n"
      "  class B {}\n"
      "}\n"
      "type C = A;\n"
      "let X: C.B;"
    );
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE(vdef->type() != nullptr);
    REQUIRE(vdef->type()->kind == Type::Kind::CLASS);
    REQUIRE_THAT(vdef->type(), TypeEQ("class B"));
  }

  SECTION("Cannot access private member outside of class via alias") {
    REQUIRE_THAT(
      compileError(cu,
        "class A {\n"
        "  private class B {}\n"
        "}\n"
        "type C = A;\n"
        "let X: C.B;"
      ),
      Catch::Contains("Cannot access private member 'B'"));
  }

  SECTION("Cannot access protected member outside of class via alias") {
    REQUIRE_THAT(
      compileError(cu,
        "class A {\n"
        "  protected class B {}\n"
        "}\n"
        "type C = A;\n"
        "let X: C.B;"
      ),
      Catch::Contains("Cannot access protected member 'B'"));
  }

  SECTION("Specialize non-generic") {
    REQUIRE_THAT(
      compileError(cu, "class A {} type B = A; let X: B[i32]"),
      Catch::Contains("Definition 'B' is not generic"));
  }
}

TEST_CASE("NameResolution.Enum", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Resolve enum type") {
    auto mod = compile(cu,
      "enum A {\n"
      "  B,\n"
      "  C,\n"
      "}\n"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    REQUIRE(td->type()->kind == Type::Kind::ENUM);
    REQUIRE(td->extends()[0]->kind == Defn::Kind::TYPE);
    REQUIRE(static_cast<const TypeDefn*>(td->extends()[0])->type()->kind == Type::Kind::INTEGER);
    REQUIRE_THAT(cast<ValueDefn>(td->members()[0])->init(), ExprEQ("0"));
    REQUIRE_THAT(cast<ValueDefn>(td->members()[1])->init(), ExprEQ("1"));
  }

  SECTION("Resolve enum type") {
    auto mod = compile(cu,
      "enum A extends u8 {\n"
      "  B,\n"
      "  C,\n"
      "  D = 3,\n"
      "  E,\n"
      "  F = 10 + 1,\n"
      "  G = 1 << 4,\n"
      "}\n"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    REQUIRE(td->type()->kind == Type::Kind::ENUM);
    REQUIRE(td->extends()[0]->kind == Defn::Kind::TYPE);
    auto enumBase = static_cast<const TypeDefn*>(td->extends()[0]);
    REQUIRE(enumBase->type()->kind == Type::Kind::INTEGER);
    REQUIRE(cast<IntegerType>(enumBase->type())->bits() == 8);
    REQUIRE(td->members().size() == 6);
    REQUIRE(td->members()[0]->name() == "B");
    REQUIRE_THAT(cast<ValueDefn>(td->members()[0])->init(), ExprEQ("0"));
    REQUIRE(td->members()[1]->name() == "C");
    REQUIRE_THAT(cast<ValueDefn>(td->members()[1])->init(), ExprEQ("1"));
    REQUIRE(td->members()[2]->name() == "D");
    REQUIRE_THAT(cast<ValueDefn>(td->members()[2])->init(), ExprEQ("3"));
    REQUIRE(td->members()[3]->name() == "E");
    REQUIRE_THAT(cast<ValueDefn>(td->members()[3])->init(), ExprEQ("4"));
    REQUIRE(td->members()[4]->name() == "F");
    REQUIRE_THAT(cast<ValueDefn>(td->members()[4])->init(), ExprEQ("11"));
    REQUIRE(td->members()[5]->name() == "G");
    REQUIRE_THAT(cast<ValueDefn>(td->members()[5])->init(), ExprEQ("16"));
  }

  SECTION("References to enum value within enum") {
    auto mod = compile(cu,
      "enum A extends u8 {\n"
      "  B = 3,\n"
      "  C = B * 2,\n"
      "  D = C / 2,\n"
      "}\n"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    REQUIRE(td->type()->kind == Type::Kind::ENUM);
    REQUIRE(td->extends()[0]->kind == Defn::Kind::TYPE);
    auto enumBase = static_cast<const TypeDefn*>(td->extends()[0]);
    REQUIRE(enumBase->type()->kind == Type::Kind::INTEGER);
    REQUIRE(td->members().size() == 3);
    REQUIRE(td->members()[1]->name() == "C");
    REQUIRE_THAT(cast<ValueDefn>(td->members()[1])->init(), ExprEQ("6"));
    REQUIRE(td->members()[2]->name() == "D");
    REQUIRE_THAT(cast<ValueDefn>(td->members()[2])->init(), ExprEQ("3"));
  }

  SECTION("Non-integer enum") {
    REQUIRE_THAT(
      compileError(cu,
        "enum A extends u8 {\n"
        "  B = \"A\",\n"
        "}"
      ),
      Catch::Contains("Enumeration value expression should be a constant integer"));

    REQUIRE_THAT(
      compileError(cu,
        "enum A extends u8 {\n"
        "  B = 1.0,\n"
        "}"
      ),
      Catch::Contains("Enumeration value expression should be a constant integer"));
  }

  SECTION("Enum too large") {
    REQUIRE_THAT(
      compileError(cu,
        "enum A extends u8 {\n"
        "  B = 1000,\n"
        "}"
      ),
      Catch::Contains("Constant integer size too large to fit in enum value"));
  }

  SECTION("Enum negative") {
    REQUIRE_THAT(
      compileError(cu,
        "enum A extends u8 {\n"
        "  B = -1,\n"
        "}"
      ),
      Catch::Contains("Can't assign a negative value to an unsigned enumeration"));
  }
}

TEST_CASE("NameResolution.Extend", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Basic Extension") {
    auto mod = compile(cu,
      "class A {}\n"
      "extend A {}"
    );
    auto cls = cast<TypeDefn>(mod->members().front());
    auto ext = cast<TypeDefn>(mod->members().back());
    REQUIRE(cls->kind == Defn::Kind::TYPE);
    REQUIRE(ext->kind == Defn::Kind::TYPE);
    REQUIRE(cls->type()->kind == Type::Kind::CLASS);
    REQUIRE(ext->type()->kind == Type::Kind::EXTENSION);
    auto result = mod->extensions()->lookup(cls);
    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == ext);
  }

  SECTION("Extend an alias") {
    auto mod = compile(cu,
      "type A = i32 | void;\n"
      "extend A {}"
    );
    auto cls = cast<TypeDefn>(mod->members().front());
    auto ext = cast<TypeDefn>(mod->members().back());
    REQUIRE(cls->kind == Defn::Kind::TYPE);
    REQUIRE(ext->kind == Defn::Kind::TYPE);
    REQUIRE(cls->type()->kind == Type::Kind::ALIAS);
    REQUIRE(ext->type()->kind == Type::Kind::EXTENSION);
    auto result = mod->extensions()->lookup(cls);
    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == ext);
  }
}

TEST_CASE("NameResolution.Attribute", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Resolve class attribute") {
    auto mod = compile(cu,
      "fn observer() {}\n"
      "@observer\n"
      "class B {}\n"
    );
    auto tdef = cast<TypeDefn>(mod->members().back());
    REQUIRE(tdef->attributes().size() == 1);
  }
}

TEST_CASE("NameResolution.Literal", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Character literal") {
    auto mod = compile(cu, "let X = 'a';");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE_THAT(vdef->init(), ExprEQ("97"));
  }

  SECTION("Integer literal") {
    auto mod = compile(cu, "let X = 1;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE_THAT(vdef->init(), ExprEQ("1"));
  }

  SECTION("Integer literal") {
    auto mod = compile(cu, "let X = 0x10;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE_THAT(vdef->init(), ExprEQ("16"));
  }

  SECTION("Float literal") {
    auto mod = compile(cu, "let X = 1.1;");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE_THAT(vdef->init(), ExprEQ("1.1"));
  }

  SECTION("String literal") {
    auto mod = compile(cu, "let X = \"abc\";");
    auto vdef = cast<ValueDefn>(mod->members().back());
    REQUIRE(vdef->kind == Defn::Kind::VAR_DEF);
    REQUIRE_FALSE(vdef->isConstant());
    REQUIRE_THAT(vdef->init(), ExprEQ("\"abc\""));
  }
}

TEST_CASE("NameResolution.FunctionBody", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Basic function") {
    auto mod = compile(cu,
      "fn test() {\n"
      "  1\n"
      "}\n"
    );
    auto fn = cast<FunctionDefn>(mod->members().back());
    REQUIRE(fn->kind == Defn::Kind::FUNCTION);
    REQUIRE_THAT(fn->body(), ExprEQ("{ 1 }"));
  }

  SECTION("Basic function w/ infix") {
    auto mod = compile(cu,
      "fn test() {\n"
      "  1 + 2\n"
      "}\n"
    );
    auto fn = cast<FunctionDefn>(mod->members().back());
    REQUIRE(fn->kind == Defn::Kind::FUNCTION);
    REQUIRE_THAT(fn->body(), ExprEQ("{ (1 + 2) }"));
  }

  SECTION("Basic function w/ call") {
    auto mod = compile(cu,
      "fn c(a: i32, b: i32) {}\n"
      "fn test() {\n"
      "  c(1, 2)\n"
      "}\n"
    );
    auto fn = cast<FunctionDefn>(mod->members().back());
    REQUIRE(fn->kind == Defn::Kind::FUNCTION);
    REQUIRE_THAT(fn->body(), ExprEQ("{ c(1, 2) }"));
  }

  SECTION("Varargs function") {
    auto mod = compile(cu,
      "fn c(a: i32, b: i32...) -> void {}\n"
    );
    auto fn = cast<FunctionDefn>(mod->members().front());
    REQUIRE(fn->kind == Defn::Kind::FUNCTION);
    REQUIRE(fn->isVariadic());
    REQUIRE(fn->type()->isVariadic);
  }

  // With ADL the error comes later.
  // SECTION("Basic function name not found") {
  //   REQUIRE_THAT(
  //     compileError(cu,
  //       "fn test() {\n"
  //       "  c(1, 2)\n"
  //       "}\n"
  //     ),
  //     Catch::Contains("Name not found: c"));
  // }
}

TEST_CASE("NameResolution.Method", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Class methods") {
    auto mod = compile(cu,
      "class A {\n"
      "  a() {}\n"
      "  get b(): i32 {}\n"
      "  set c(value: i32) {}\n"
      "}\n"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    auto fnA = cast<FunctionDefn>(td->members()[1]);
    REQUIRE(fnA->kind == Defn::Kind::FUNCTION);
    REQUIRE(!fnA->isAbstract());
    REQUIRE(!fnA->isConstructor());
    REQUIRE(!fnA->isGetter());
    REQUIRE(!fnA->isSetter());
    auto fnB = cast<FunctionDefn>(td->members()[2]);
    REQUIRE(fnB->kind == Defn::Kind::FUNCTION);
    REQUIRE(!fnB->isAbstract());
    REQUIRE(!fnB->isConstructor());
    REQUIRE(fnB->isGetter());
    REQUIRE(!fnB->isSetter());
    auto fnC = cast<FunctionDefn>(td->members()[3]);
    REQUIRE(fnC->kind == Defn::Kind::FUNCTION);
    REQUIRE(!fnC->isAbstract());
    REQUIRE(!fnC->isConstructor());
    REQUIRE(!fnC->isGetter());
    REQUIRE(fnC->isSetter());
  }

  SECTION("Unsafe") {
    auto mod = compile(cu,
      "class A {\n"
      "  unsafe a() {}\n"
      "  b() {}\n"
      "}\n"
    );
    auto td = cast<TypeDefn>(mod->members().back());
    auto fnA = cast<FunctionDefn>(td->members()[1]);
    REQUIRE(fnA->kind == Defn::Kind::FUNCTION);
    REQUIRE(fnA->isUnsafe());
    auto fnB = cast<FunctionDefn>(td->members()[2]);
    REQUIRE(fnB->kind == Defn::Kind::FUNCTION);
    REQUIRE(!fnB->isUnsafe());
  }

  SECTION("Invalid getter") {
    REQUIRE_THAT(
      compileError(cu,
        "class A {\n"
        "  get b() {}\n"
        "}\n"
      ),
      Catch::Contains("Getter method must declare a return type"));
  }

  SECTION("Invalid setter") {
    REQUIRE_THAT(
      compileError(cu,
        "class A {\n"
        "  set c() {}\n"
        "}\n"
      ),
      Catch::Contains("Setter method must declare at least one method parameter"));

    REQUIRE_THAT(
      compileError(cu,
        "class A {\n"
        "  set c(value: i32): i32 {}\n"
        "}\n"
      ),
      Catch::Contains("Setter method may not declare a return type"));
  }
}

TEST_CASE("NameResolution.LocalVar", "[sema]") {
  const Location L;
  CompilationUnit cu;

  SECTION("Local var") {
    auto mod = compile(cu,
      "fn x() {\n"
      "  let result = 1;\n"
      "  result\n"
      "}\n"
    );
    auto fd = cast<FunctionDefn>(mod->members().back());
    auto body = cast<BlockStmt>(fd->body());
    auto letSt = cast<LocalVarStmt>(body->stmts[0]);
    REQUIRE(letSt->defn->name() == "result");
  }
}
