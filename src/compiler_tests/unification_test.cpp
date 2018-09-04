#include "catch.hpp"
#include "semamatcher.hpp"
#include "tempest/compiler/compilationunit.hpp"
#include "tempest/ast/module.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include "tempest/sema/convert/predicate.hpp"
#include "tempest/sema/graph/module.hpp"
#include "tempest/sema/graph/primitivetype.hpp"
#include "tempest/sema/infer/types.hpp"
#include "tempest/sema/infer/unification.hpp"
#include "tempest/sema/pass/buildgraph.hpp"
#include "tempest/sema/pass/nameresolution.hpp"
#include "llvm/Support/Casting.h"
#include <iostream>

using namespace tempest::compiler;
using namespace tempest::sema::infer;
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
}

TEST_CASE("Unification.primitive", "[sema]") {
  CompilationUnit cu;
  std::vector<UnificationResult> result;
  Conditions conditions;
  tempest::support::BumpPtrAllocator alloc;

  SECTION("Unify integer types") {
    REQUIRE(unify(result, &IntegerType::I8, &IntegerType::I8, conditions,
        BindingPredicate::EQUAL, alloc));
    REQUIRE(unify(result, &IntegerType::I32, &IntegerType::I32, conditions,
        BindingPredicate::EQUAL, alloc));
    REQUIRE_FALSE(unify(result, &IntegerType::I8, &IntegerType::I16, conditions,
        BindingPredicate::EQUAL, alloc));
    REQUIRE_FALSE(unify(result, &IntegerType::I16, &IntegerType::I8, conditions,
        BindingPredicate::EQUAL, alloc));
    REQUIRE(unify(result, &IntegerType::I16, &IntegerType::I8, conditions,
        BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(unify(result, &IntegerType::I8, &IntegerType::I16, conditions,
        BindingPredicate::ASSIGNABLE_TO, alloc));
  }
}

TEST_CASE("Unification.composite", "[sema]") {
  CompilationUnit cu;
  std::vector<UnificationResult> result;
  Conditions conditions;
  tempest::support::BumpPtrAllocator alloc;

  SECTION("Unify class with inheritance") {
    auto mod = compile(cu,
      "class A {}\n"
      "class B {}\n"
      "interface I {}\n"
      "class C extends A implements I {}\n"
    );
    auto clsA = cast<TypeDefn>(mod->members()[0])->type();
    auto clsB = cast<TypeDefn>(mod->members()[1])->type();
    auto intI = cast<TypeDefn>(mod->members()[2])->type();
    auto clsC = cast<TypeDefn>(mod->members()[3])->type();
    REQUIRE(unify(result, clsA, clsA, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE(unify(result, clsA, clsA, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(unify(result, clsA, clsA, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE(unify(result, clsA, clsA, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE(unify(result, clsA, clsA, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE_FALSE(unify(result, clsA, clsB, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE_FALSE(unify(result, clsA, clsB, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE_FALSE(unify(result, clsA, clsB, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE_FALSE(unify(result, clsA, clsB, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE_FALSE(unify(result, clsA, clsB, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE_FALSE(unify(result, clsC, clsA, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE_FALSE(unify(result, clsC, clsA, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(unify(result, clsC, clsA, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE(unify(result, clsC, clsA, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE_FALSE(unify(result, clsC, clsA, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE_FALSE(unify(result, clsA, clsC, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE(unify(result, clsA, clsC, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE_FALSE(unify(result, clsA, clsC, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE_FALSE(unify(result, clsA, clsC, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE(unify(result, clsA, clsC, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE_FALSE(unify(result, clsC, intI, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE_FALSE(unify(result, clsC, intI, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(unify(result, clsC, intI, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE(unify(result, clsC, intI, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE_FALSE(unify(result, clsC, intI, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE_FALSE(unify(result, intI, clsC, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE(unify(result, intI, clsC, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE_FALSE(unify(result, intI, clsC, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE_FALSE(unify(result, intI, clsC, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE(unify(result, intI, clsC, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE(result.size() == 0);
  }

//   SECTION("Structural typing") {
//     auto mod = compile(cu,
//       "class A { fn x() -> i32 { 0 } }\n"
//       "interface I { fn x() -> i32; }\n"
//       "interface J { fn x() -> bool; }\n"
//       "interface K extends I {}\n"
//     );
//     auto clsA = cast<TypeDefn>(mod->members()[0])->type();
//     auto intI = cast<TypeDefn>(mod->members()[1])->type();
//     auto intJ = cast<TypeDefn>(mod->members()[2])->type();
//     auto intK = cast<TypeDefn>(mod->members()[3])->type();
//     REQUIRE(isAssignable(intI, clsA) == ConversionResult(ConversionRank::EXACT));
//     REQUIRE(isAssignable(intJ, clsA)
//         == ConversionResult(ConversionRank::ERROR, ConversionError::INCOMPATIBLE));
//     REQUIRE(isAssignable(intK, clsA) == ConversionResult(ConversionRank::EXACT));
//   }

  SECTION("Unify templated class") {
    auto mod = compile(cu,
      "class A[T] {}\n"
      "class B extends A[i32] {}\n"
      "class C extends A[f32] {}\n"
      "let D: A[i32];"
    );
    auto clsA = cast<TypeDefn>(mod->members()[0])->type();
    auto clsB = cast<TypeDefn>(mod->members()[1])->type();
    auto clsC = cast<TypeDefn>(mod->members()[2])->type();
    auto letD = cast<ValueDefn>(mod->members()[3])->type();

    REQUIRE(unify(result, clsA, clsA, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE(unify(result, clsA, clsA, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(unify(result, clsA, clsA, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE(unify(result, clsA, clsA, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE(unify(result, clsA, clsA, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE_FALSE(unify(result, clsA, clsB, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE_FALSE(unify(result, clsA, clsB, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE_FALSE(unify(result, clsA, clsB, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE_FALSE(unify(result, clsA, clsB, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE_FALSE(unify(result, clsA, clsB, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE_FALSE(unify(result, clsB, clsC, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE_FALSE(unify(result, clsB, clsC, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE_FALSE(unify(result, clsB, clsC, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE_FALSE(unify(result, clsB, clsC, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE_FALSE(unify(result, clsB, clsC, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE_FALSE(unify(result, clsC, clsB, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE_FALSE(unify(result, clsC, clsB, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE_FALSE(unify(result, clsC, clsB, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE_FALSE(unify(result, clsC, clsB, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE_FALSE(unify(result, clsC, clsB, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE_FALSE(unify(result, clsB, letD, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE_FALSE(unify(result, clsB, letD, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(unify(result, clsB, letD, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE(unify(result, clsB, letD, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE_FALSE(unify(result, clsB, letD, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE_FALSE(unify(result, letD, clsB, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE(unify(result, letD, clsB, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE_FALSE(unify(result, letD, clsB, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE_FALSE(unify(result, letD, clsB, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE(unify(result, letD, clsB, conditions, BindingPredicate::SUPERTYPE, alloc));

    REQUIRE(result.size() == 0);
  }

  SECTION("Unify generic with inferred type variable") {
    auto mod = compile(cu,
      "class A[T] {}\n"
      "class B extends A[i32] {}\n"
    );
    auto clsA = cast<TypeDefn>(mod->members()[0])->type();
    auto clsB = cast<TypeDefn>(mod->members()[1])->type();

    auto typeDefA = cast<UserDefinedType>(clsA)->defn();
    Env env;
    Env empty;
    env.params = typeDefA->typeParams();
    InferredType a(env.params[0], nullptr, nullptr);
    env.args.push_back(&a);

    REQUIRE_FALSE(
        unify(result, clsA, env, clsB, empty, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE(
        unify(result, clsA, env, clsB, empty, conditions, BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].param == &a);
    REQUIRE_THAT(result[0].value, TypeEQ("i32"));
    REQUIRE(result[0].conditions.empty());
    REQUIRE(result[0].predicate == BindingPredicate::EQUAL);
    result.clear();
    REQUIRE_FALSE(
        unify(result, clsA, env, clsB, empty, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE_FALSE(
        unify(result, clsA, env, clsB, empty, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE(
        unify(result, clsA, env, clsB, empty, conditions, BindingPredicate::SUPERTYPE, alloc));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].param == &a);
    REQUIRE_THAT(result[0].value, TypeEQ("i32"));
    REQUIRE(result[0].conditions.empty());
    REQUIRE(result[0].predicate == BindingPredicate::EQUAL);
    result.clear();
  }

  SECTION("Unify generic with inferred type variable (source)") {
    auto mod = compile(cu,
      "class A[T] {}\n"
      "class B extends A[i32] {}\n"
    );
    auto clsA = cast<TypeDefn>(mod->members()[0])->type();
    auto clsB = cast<TypeDefn>(mod->members()[1])->type();

    auto typeDefA = cast<UserDefinedType>(clsA)->defn();
    Env env;
    Env empty;
    env.params = typeDefA->typeParams();
    InferredType a(env.params[0], nullptr, nullptr);
    env.args.push_back(&a);

    REQUIRE_FALSE(
        unify(result, clsB, empty, clsA, env, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE(
        unify(result, clsB, empty, clsA, env, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].param == &a);
    REQUIRE_THAT(result[0].value, TypeEQ("i32"));
    REQUIRE(result[0].conditions.empty());
    REQUIRE(result[0].predicate == BindingPredicate::EQUAL);
    result.clear();
    REQUIRE_FALSE(
        unify(result, clsB, empty, clsA, env, conditions,
            BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE_FALSE(
        unify(result, clsB, empty, clsA, env, conditions, BindingPredicate::SUPERTYPE, alloc));
    REQUIRE(
        unify(result, clsB, empty, clsA, env, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].param == &a);
    REQUIRE_THAT(result[0].value, TypeEQ("i32"));
    REQUIRE(result[0].conditions.empty());
    REQUIRE(result[0].predicate == BindingPredicate::EQUAL);
    result.clear();
  }
}

TEST_CASE("Unification.derived", "[sema]") {
  CompilationUnit cu;
  std::vector<UnificationResult> result;
  Conditions conditions;
  Env empty;
  tempest::support::BumpPtrAllocator alloc;

  SECTION("Unify tuple") {
    auto mod = compile(cu,
      "fn A[T](arg: (i32, i32, T)) -> void {}\n"
      "fn B(arg: (i32, i32, bool)) -> void {}\n"
    );
    auto fnA = cast<FunctionDefn>(mod->members()[0]);
    auto fnB = cast<FunctionDefn>(mod->members()[1]);
    auto typeA = fnA->type();
    auto typeB = fnB->type();
    auto argA = typeA->paramTypes[0];
    auto argB = typeB->paramTypes[0];

    Env env;
    env.params = fnA->typeParams();
    InferredType a(env.params[0], nullptr, nullptr);
    env.args.push_back(&a);

    REQUIRE(unify(result, argA, env, argB, empty, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].param == &a);
    REQUIRE_THAT(result[0].value, TypeEQ("bool"));
    REQUIRE(result[0].conditions.empty());
    REQUIRE(result[0].predicate == BindingPredicate::EQUAL);

    REQUIRE(
        unify(result, argA, env, argB, empty, conditions,
            BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(
        unify(result, argA, env, argB, empty, conditions, BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE(
        unify(result, argA, env, argB, empty, conditions, BindingPredicate::SUBTYPE, alloc));
    REQUIRE(
        unify(result, argA, env, argB, empty, conditions, BindingPredicate::SUPERTYPE, alloc));
  }

  SECTION("Unify union with union") {
    auto mod = compile(cu,
      "fn A[T](arg: i32 | void | T) -> void {}\n"
      "fn B[T](arg: i32 | bool | T) -> void {}\n"
      "fn C[S, T](arg: i32 | S | T) -> void {}\n"
      "fn D(arg: i32 | void | bool) -> void {}\n"
      "fn E(arg: i32 | void) -> void {}\n"
      "fn F(arg: i32 | bool | f32) -> void {}\n"
    );
    auto fnA = cast<FunctionDefn>(mod->members()[0]);
    auto fnB = cast<FunctionDefn>(mod->members()[1]);
    auto fnC = cast<FunctionDefn>(mod->members()[2]);
    auto fnD = cast<FunctionDefn>(mod->members()[3]);
    auto fnE = cast<FunctionDefn>(mod->members()[4]);
    auto fnF = cast<FunctionDefn>(mod->members()[5]);
    auto typeA = fnA->type()->paramTypes[0];
    auto typeB = fnB->type()->paramTypes[0];
    auto typeC = fnC->type()->paramTypes[0];
    auto typeD = fnD->type()->paramTypes[0];
    auto typeE = fnE->type()->paramTypes[0];
    auto typeF = fnF->type()->paramTypes[0];

    Env envA;
    envA.params = fnA->typeParams();
    InferredType a(envA.params[0], nullptr, nullptr);
    envA.args.push_back(&a);

    Env envB;
    envB.params = fnB->typeParams();
    InferredType b(envB.params[0], nullptr, nullptr);
    envB.args.push_back(&b);

    Env envC;
    envC.params = fnC->typeParams();
    InferredType cs(envC.params[0], nullptr, nullptr);
    InferredType ct(envC.params[1], nullptr, nullptr);
    envC.args.push_back(&cs);
    envC.args.push_back(&ct);

    // Generic union with generic union
    REQUIRE(unify(result, typeA, envA, typeB, envB, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE(result.size() == 2);
    REQUIRE(result[0].param == &b);
    REQUIRE_THAT(result[0].value, TypeEQ("void"));
    REQUIRE(result[0].conditions.empty());
    REQUIRE(result[0].predicate == BindingPredicate::EQUAL);
    REQUIRE(result[1].param == &a);
    REQUIRE_THAT(result[1].value, TypeEQ("bool"));
    REQUIRE(result[1].conditions.empty());
    REQUIRE(result[1].predicate == BindingPredicate::EQUAL);

    REQUIRE(unify(result, typeA, envA, typeB, envB, conditions,
        BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(unify(result, typeA, envA, typeB, envB, conditions,
        BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE(unify(result, typeA, envA, typeB, envB, conditions,
        BindingPredicate::SUBTYPE, alloc));
    REQUIRE(unify(result, typeA, envA, typeB, envB, conditions,
        BindingPredicate::SUPERTYPE, alloc));
    result.clear();

    // Union with more than one type param - fail
    REQUIRE_FALSE(unify(result, typeA, envA, typeC, envC, conditions,
        BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(result.size() == 0);

    // Generic union with non-generic - type parameter gets the leftover
    REQUIRE(unify(result, typeA, envA, typeD, empty, conditions, BindingPredicate::EQUAL, alloc));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].param == &a);
    REQUIRE_THAT(result[0].value, TypeEQ("bool"));
    REQUIRE(result[0].conditions.empty());
    REQUIRE(result[0].predicate == BindingPredicate::EQUAL);

    REQUIRE(unify(result, typeA, envA, typeD, empty, conditions,
        BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(unify(result, typeA, envA, typeD, empty, conditions,
        BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE(unify(result, typeA, envA, typeD, empty, conditions,
        BindingPredicate::SUBTYPE, alloc));
    REQUIRE(unify(result, typeA, envA, typeD, empty, conditions,
        BindingPredicate::SUPERTYPE, alloc));
    result.clear();

    // Generic union with smaller union - type parameter is undetermined
    REQUIRE_FALSE(unify(result, typeA, envA, typeE, empty, conditions,
        BindingPredicate::EQUAL, alloc));
    REQUIRE(unify(result, typeA, envA, typeE, empty, conditions,
        BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(unify(result, typeA, envA, typeE, empty, conditions,
        BindingPredicate::SUPERTYPE, alloc));
    REQUIRE_FALSE(unify(result, typeA, envA, typeE, empty, conditions,
        BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE_FALSE(unify(result, typeA, envA, typeE, empty, conditions,
        BindingPredicate::SUBTYPE, alloc));
    REQUIRE(result.empty());

    // Generic union with larger union - type parameter bound to temp union of leftovers
    REQUIRE_FALSE(unify(result, typeA, envA, typeF, empty, conditions,
        BindingPredicate::EQUAL, alloc));
    REQUIRE(result.empty());
    REQUIRE(unify(result, typeA, envA, typeF, empty, conditions,
        BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].param == &a);
    REQUIRE_THAT(result[0].value, TypeEQ("bool | f32"));
    REQUIRE(result[0].conditions.empty());
    REQUIRE_FALSE(unify(result, typeA, envA, typeF, empty, conditions,
        BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE(unify(result, typeA, envA, typeF, empty, conditions,
        BindingPredicate::SUPERTYPE, alloc));
    REQUIRE_FALSE(unify(result, typeA, envA, typeF, empty, conditions,
        BindingPredicate::SUBTYPE, alloc));
    result.clear();

    // Flipped
    REQUIRE_FALSE(unify(result, typeF, empty, typeA, envA, conditions,
        BindingPredicate::EQUAL, alloc));
    REQUIRE(result.empty());
    REQUIRE(unify(result, typeF, empty, typeA, envA, conditions,
        BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].param == &a);
    REQUIRE_THAT(result[0].value, TypeEQ("bool | f32"));
    REQUIRE(result[0].conditions.empty());
    REQUIRE_FALSE(unify(result, typeF, empty, typeA, envA, conditions,
        BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(unify(result, typeF, empty, typeA, envA, conditions,
        BindingPredicate::SUBTYPE, alloc));
    REQUIRE_FALSE(unify(result, typeF, empty, typeA, envA, conditions,
        BindingPredicate::SUPERTYPE, alloc));
  }

  SECTION("Unify union with non-union") {
    auto mod = compile(cu,
      "fn A[T](arg: i32 | void | T) -> void {}\n"
      "fn B(arg: i32) -> void {}\n"
      "fn C(arg: f32) -> void {}\n"
    );
    auto fnA = cast<FunctionDefn>(mod->members()[0]);
    auto fnB = cast<FunctionDefn>(mod->members()[1]);
    auto fnC = cast<FunctionDefn>(mod->members()[2]);
    auto typeA = fnA->type()->paramTypes[0];
    auto typeB = fnB->type()->paramTypes[0];
    auto typeC = fnC->type()->paramTypes[0];

    Env envA;
    envA.params = fnA->typeParams();
    InferredType a(envA.params[0], nullptr, nullptr);
    envA.args.push_back(&a);

    // (i32 | void | T) is not the same as i32.
    REQUIRE_FALSE(unify(result, typeA, envA, typeB, empty, conditions,
        BindingPredicate::EQUAL, alloc));
    REQUIRE(result.empty());

    // (i32 | void | T) should be assignable from i32.
    REQUIRE(unify(result, typeA, envA, typeB, empty, conditions,
        BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(result.size() == 0); // Type of T is undetermined by this unification.

    REQUIRE_FALSE(unify(result, typeA, envA, typeB, empty, conditions,
        BindingPredicate::ASSIGNABLE_TO, alloc));
    REQUIRE_FALSE(unify(result, typeA, envA, typeB, empty, conditions,
        BindingPredicate::SUBTYPE, alloc));
    REQUIRE(unify(result, typeA, envA, typeB, empty, conditions,
        BindingPredicate::SUPERTYPE, alloc));
    result.clear();

    // (i32 | void | T) can be assigned from f32 if T => f32.
    REQUIRE_FALSE(unify(result, typeA, envA, typeC, empty, conditions,
        BindingPredicate::EQUAL, alloc));
    REQUIRE(unify(result, typeA, envA, typeC, empty, conditions,
        BindingPredicate::ASSIGNABLE_FROM, alloc));
    REQUIRE(result.size() == 1);
    REQUIRE(result[0].param == &a);
    REQUIRE_THAT(result[0].value, TypeEQ("f32"));
    REQUIRE(result[0].conditions.empty());
    REQUIRE(result[0].predicate == BindingPredicate::ASSIGNABLE_FROM);
  }
}
