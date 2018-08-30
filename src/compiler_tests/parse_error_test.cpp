#include "catch.hpp"
#include "astmatcher.hpp"
#include "tempest/error/diagnostics.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include "./mockreporter.hpp"
#include <iostream>

using namespace tempest::ast;
using namespace tempest::error;
using namespace tempest::parse;
using namespace std;
using tempest::source::Location;

class TestSource : public tempest::source::StringSource {
public:
  TestSource(llvm::StringRef source)
    : tempest::source::StringSource("test.txt", source)
  {}
};

/** Parse a module definition. */
std::string parseModuleError(tempest::support::BumpPtrAllocator& alloc, const char* srcText) {
  UseMockReporter umr;
  TestSource src(srcText);
  Parser parser(&src, alloc);
  parser.module();
  REQUIRE(MockReporter::INSTANCE.errorCount() == 1);
  return MockReporter::INSTANCE.content().str();
}

// /** Parse a member declaration. */
// Node* parseMemberDeclaration(tempest::support::BumpPtrAllocator& alloc, const char* srcText) {
//   UseMockReporter umr;
//   TestSource src(srcText);
//   Parser parser(&src, alloc);
//   Node* result = parser.memberDeclaration();
//   REQUIRE(parser.done());
//   return result;
// }

/** Parse an expression. */
std::string parseExprError(tempest::support::BumpPtrAllocator& alloc, const char* srcText) {
  UseMockReporter umr;
  TestSource src(srcText);
  Parser parser(&src, alloc);
  parser.expression();
  REQUIRE(MockReporter::INSTANCE.errorCount() == 1);
  return MockReporter::INSTANCE.content().str();
}

TEST_CASE("ParserError", "[parse]") {
  const Location L;

  SECTION("Module") {
    tempest::support::BumpPtrAllocator alloc;
    REQUIRE_THAT(
      parseModuleError(alloc,
        "import { A } from 1;\n"
        "import { B } from a;\n"
      ),
      Catch::Contains("Expected module name"));

    REQUIRE_THAT(
      parseModuleError(alloc,
        "class X\n"
        "struct X {}\n"
        "trait X {}\n"
        "interface X {}\n"
      ),
      Catch::Contains("Expected {"));
  }

  // SECTION("Class") {
  //   tempest::support::BumpPtrAllocator alloc;

  //   // Basic class declaration
  //   REQUIRE_THAT(
  //     parseMemberDeclaration(alloc,
  //       "class X {}\n"
  //     ),
  //     ASTEQ(
  //       "(#CLASS_DEFN X)\n"
  //     ));

  //   // Member variables
  //   REQUIRE_THAT(
  //     parseMemberDeclaration(alloc,
  //       "class X {\n"
  //       "  x: Point;\n"
  //       "  private y: i32;\n"
  //       "  final const z: [bool];\n"
  //       "}\n"
  //     ),
  //     ASTEQ(
  //       "(#CLASS_DEFN X\n"
  //       "  (#MEMBER_VAR x: Point)\n"
  //       "  (#MEMBER_VAR\n"
  //       "    #private y: i32)\n"
  //       "  (#MEMBER_CONST\n"
  //       "    #final z: (#ARRAY_TYPE bool)))\n"
  //     ));

  //   // Private block
  //   REQUIRE_THAT(
  //     parseMemberDeclaration(alloc,
  //       "class X {\n"
  //       "  private {\n"
  //       "   x: i32;\n"
  //       "  }\n"
  //       "}\n"
  //     ),
  //     ASTEQ(
  //       "(#CLASS_DEFN X\n"
  //       "  (#MEMBER_VAR\n"
  //       "    #private x: i32))\n"
  //     ));

  //   // Member function
  //   REQUIRE_THAT(
  //     parseMemberDeclaration(alloc,
  //       "class X {\n"
  //       "  fn a() {}\n"
  //       "}\n"
  //     ),
  //     ASTEQ(
  //       "(#CLASS_DEFN X\n"
  //       "  (#FUNCTION a (#BLOCK)))\n"
  //     ));
  // }

  // SECTION("Statement") {
  //   tempest::support::BumpPtrAllocator alloc;

  //   // Block with multiple statements
  //   REQUIRE_THAT(
  //     parseMemberDeclaration(alloc,
  //       "fn a() {\n"
  //       "  if a { 1 } else { 2 }\n"
  //       "  if b { 3 } else { 4 }\n"
  //       "}\n"
  //     ),
  //     ASTEQ(
  //       "(#FUNCTION a (#BLOCK\n"
  //       "  (#IF\n"
  //       "    a\n"
  //       "    (outcomes\n"
  //       "      (#BLOCK\n"
  //       "        result: (int 1))\n"
  //       "      (#BLOCK\n"
  //       "        result: (int 2))))\n"
  //       "  result: (#IF\n"
  //       "    b\n"
  //       "    (outcomes\n"
  //       "      (#BLOCK\n"
  //       "        result: (int 3))\n"
  //       "      (#BLOCK\n"
  //       "        result: (int 4))))))\n"
  //     ));

  //   // Block with terminating semicolon - void result
  //   REQUIRE_THAT(
  //     parseMemberDeclaration(alloc,
  //       "fn a() {\n"
  //       "  if a { 1 } else { 2 }\n"
  //       "  if b { 3 } else { 4 };\n"
  //       "}\n"
  //     ),
  //     ASTEQ(
  //       "(#FUNCTION a (#BLOCK\n"
  //       "  (#IF\n"
  //       "    a\n"
  //       "    (outcomes\n"
  //       "      (#BLOCK\n"
  //       "        result: (int 1))\n"
  //       "      (#BLOCK\n"
  //       "        result: (int 2))))\n"
  //       "  (#IF\n"
  //       "    b\n"
  //       "    (outcomes\n"
  //       "      (#BLOCK\n"
  //       "        result: (int 3))\n"
  //       "      (#BLOCK\n"
  //       "        result: (int 4))))))\n"
  //     ));

  //   // Block with simple statements - non-void return
  //   REQUIRE_THAT(
  //     parseMemberDeclaration(alloc,
  //       "fn a() {\n"
  //       "  x = 1;\n"
  //       "  y = 2\n"
  //       "}\n"
  //     ),
  //     ASTEQ(
  //       "(#FUNCTION a (#BLOCK\n"
  //       "  (#ASSIGN\n"
  //       "    x\n"
  //       "    (int 1))\n"
  //       "  result: (#ASSIGN\n"
  //       "    y\n"
  //       "    (int 2))))\n"
  //     ));

  //   // Block with simple statements - void return
  //   REQUIRE_THAT(
  //     parseMemberDeclaration(alloc,
  //       "fn a() {\n"
  //       "  x = 1;\n"
  //       "  y = 2;\n"
  //       "}\n"
  //     ),
  //     ASTEQ(
  //       "(#FUNCTION a (#BLOCK\n"
  //       "  (#ASSIGN\n"
  //       "    x\n"
  //       "    (int 1))\n"
  //       "  (#ASSIGN\n"
  //       "    y\n"
  //       "    (int 2))))\n"
  //     ));
  // }

  SECTION("InfixOperators") {
    tempest::support::BumpPtrAllocator alloc;
    REQUIRE_THAT(
      parseExprError(alloc, "1 + ;"),
      Catch::Contains("Expression expected"));

  //   REQUIRE_THAT(
  //     parseExpr(alloc, "1 - 2"),
  //     ASTEQ(
  //       "(#SUB\n"
  //       "  (int 1)\n"
  //       "  (int 2))\n"
  //     ));

  //   REQUIRE_THAT(
  //     parseExpr(alloc, "1 * 2"),
  //     ASTEQ(
  //       "(#MUL\n"
  //       "  (int 1)\n"
  //       "  (int 2))\n"
  //     ));

  //   REQUIRE_THAT(
  //     parseExpr(alloc, "1 / 2"),
  //     ASTEQ(
  //       "(#DIV\n"
  //       "  (int 1)\n"
  //       "  (int 2))\n"
  //     ));

  //   REQUIRE_THAT(
  //     parseExpr(alloc, "1 % 2"),
  //     ASTEQ(
  //       "(#MOD\n"
  //       "  (int 1)\n"
  //       "  (int 2))\n"
  //     ));

  //   REQUIRE_THAT(
  //     parseExpr(alloc, "1 - 2 * 3"),
  //     ASTEQ(
  //       "(#SUB\n"
  //       "  (int 1)\n"
  //       "  (#MUL\n"
  //       "    (int 2)\n"
  //       "    (int 3)))\n"
  //     ));
  }
}
