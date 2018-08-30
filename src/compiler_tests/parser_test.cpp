#include "catch.hpp"
#include "astmatcher.hpp"
#include "tempest/parse/lexer.hpp"
#include "tempest/parse/parser.hpp"
#include <iostream>

using namespace tempest::ast;
using namespace tempest::parse;
using namespace std;
using tempest::source::Location;

class TestSource : public tempest::source::StringSource {
public:
  TestSource(llvm::StringRef source)
    : tempest::source::StringSource("test.txt", source)
  {}
};

namespace {
  /** Parse a module definition. */
  Node* parseModule(tempest::support::BumpPtrAllocator& alloc, const char* srcText) {
    TestSource src(srcText);
    Parser parser(&src, alloc);
    Node* result = parser.module();
    REQUIRE(parser.done());
    return result;
  }

  /** Parse a member declaration. */
  Node* parseMemberDeclaration(tempest::support::BumpPtrAllocator& alloc, const char* srcText) {
    TestSource src(srcText);
    Parser parser(&src, alloc);
    Node* result = parser.declaration(DECL_GLOBAL);
    REQUIRE(parser.done());
    return result;
  }

  /** Parse an expression. */
  Node* parseExpr(tempest::support::BumpPtrAllocator& alloc, const char* srcText) {
    TestSource src(srcText);
    Parser parser(&src, alloc);
    Node* result = parser.expression();
    REQUIRE(parser.done());
    return result;
  }
}

TEST_CASE("Parser", "[parse]") {
  const Location L;

  SECTION("Module") {
    tempest::support::BumpPtrAllocator alloc;
    REQUIRE_THAT(
      parseModule(alloc,
        "import { A } from b.c;\n"
        "import { A, B } from .c;\n"
        "export { C } from d.e.f;\n"
      ),
      ASTEQ(
        "(#MODULE\n"
        "  (#IMPORT 0 \"b.c\"\n"
        "    A)\n"
        "  (#IMPORT 1 \"c\"\n"
        "    A\n"
        "    B)\n"
        "  (#EXPORT 0 \"d.e.f\"\n"
        "    C))\n"
      ));

    REQUIRE_THAT(
      parseModule(alloc,
        "class X {}\n"
        "struct X {}\n"
        "trait X {}\n"
        "interface X {}\n"
      ),
      ASTEQ(
        "(#MODULE\n"
        "  (#CLASS_DEFN X)\n"
        "  (#STRUCT_DEFN X)\n"
        "  (#TRAIT_DEFN X)\n"
        "  (#INTERFACE_DEFN X))\n"
      ));
  }

  SECTION("Class") {
    tempest::support::BumpPtrAllocator alloc;

    // Basic class declaration
    REQUIRE_THAT(
      parseMemberDeclaration(alloc,
        "class X {}\n"
      ),
      ASTEQ(
        "(#CLASS_DEFN X)\n"
      ));

    // Member variables
    REQUIRE_THAT(
      parseMemberDeclaration(alloc,
        "class X {\n"
        "  x: Point;\n"
        "  private y: i32;\n"
        "  final const z: [bool];\n"
        "}\n"
      ),
      ASTEQ(
        "(#CLASS_DEFN X\n"
        "  (#MEMBER_VAR x: Point)\n"
        "  (#MEMBER_VAR\n"
        "    #private y: i32)\n"
        "  (#MEMBER_CONST\n"
        "    #final z: (#ARRAY_TYPE bool)))\n"
      ));

    // Private block
    REQUIRE_THAT(
      parseMemberDeclaration(alloc,
        "class X {\n"
        "  private {\n"
        "   x: i32;\n"
        "   y = 3;\n"
        "  }\n"
        "}\n"
      ),
      ASTEQ(
        "(#CLASS_DEFN X\n"
        "  (#MEMBER_VAR\n"
        "    #private x: i32)\n"
        "  (#MEMBER_VAR\n"
        "    #private y = (int 3)))\n"
      ));

    // Member function
    REQUIRE_THAT(
      parseMemberDeclaration(alloc,
        "class X {\n"
        "  fn a() {}\n"
        "}\n"
      ),
      ASTEQ(
        "(#CLASS_DEFN X\n"
        "  (#FUNCTION a (#BLOCK)))\n"
      ));
  }

  SECTION("Statement") {
    tempest::support::BumpPtrAllocator alloc;

    // Block with multiple statements
    REQUIRE_THAT(
      parseMemberDeclaration(alloc,
        "fn a() {\n"
        "  if a { 1 } else { 2 }\n"
        "  if b { 3 } else { 4 }\n"
        "}\n"
      ),
      ASTEQ(
        "(#FUNCTION a (#BLOCK\n"
        "  (#IF\n"
        "    a\n"
        "    (outcomes\n"
        "      (#BLOCK\n"
        "        result: (int 1))\n"
        "      (#BLOCK\n"
        "        result: (int 2))))\n"
        "  result: (#IF\n"
        "    b\n"
        "    (outcomes\n"
        "      (#BLOCK\n"
        "        result: (int 3))\n"
        "      (#BLOCK\n"
        "        result: (int 4))))))\n"
      ));

    // Block with terminating semicolon - void result
    REQUIRE_THAT(
      parseMemberDeclaration(alloc,
        "fn a() {\n"
        "  if a { 1 } else { 2 }\n"
        "  if b { 3 } else { 4 };\n"
        "}\n"
      ),
      ASTEQ(
        "(#FUNCTION a (#BLOCK\n"
        "  (#IF\n"
        "    a\n"
        "    (outcomes\n"
        "      (#BLOCK\n"
        "        result: (int 1))\n"
        "      (#BLOCK\n"
        "        result: (int 2))))\n"
        "  (#IF\n"
        "    b\n"
        "    (outcomes\n"
        "      (#BLOCK\n"
        "        result: (int 3))\n"
        "      (#BLOCK\n"
        "        result: (int 4))))))\n"
      ));

    // Block with simple statements - non-void return
    REQUIRE_THAT(
      parseMemberDeclaration(alloc,
        "fn a() {\n"
        "  x = 1;\n"
        "  y = 2\n"
        "}\n"
      ),
      ASTEQ(
        "(#FUNCTION a (#BLOCK\n"
        "  (#ASSIGN\n"
        "    x\n"
        "    (int 1))\n"
        "  result: (#ASSIGN\n"
        "    y\n"
        "    (int 2))))\n"
      ));

    // Block with simple statements - void return
    REQUIRE_THAT(
      parseMemberDeclaration(alloc,
        "fn a() {\n"
        "  x = 1;\n"
        "  y = 2;\n"
        "}\n"
      ),
      ASTEQ(
        "(#FUNCTION a (#BLOCK\n"
        "  (#ASSIGN\n"
        "    x\n"
        "    (int 1))\n"
        "  (#ASSIGN\n"
        "    y\n"
        "    (int 2))))\n"
      ));
  }

  SECTION("InfixOperators") {
    tempest::support::BumpPtrAllocator alloc;
    REQUIRE_THAT(
      parseExpr(alloc, "1 + 2"),
      ASTEQ(
        "(#ADD\n"
        "  (int 1)\n"
        "  (int 2))\n"
      ));

    REQUIRE_THAT(
      parseExpr(alloc, "1 - 2"),
      ASTEQ(
        "(#SUB\n"
        "  (int 1)\n"
        "  (int 2))\n"
      ));

    REQUIRE_THAT(
      parseExpr(alloc, "1 * 2"),
      ASTEQ(
        "(#MUL\n"
        "  (int 1)\n"
        "  (int 2))\n"
      ));

    REQUIRE_THAT(
      parseExpr(alloc, "1 / 2"),
      ASTEQ(
        "(#DIV\n"
        "  (int 1)\n"
        "  (int 2))\n"
      ));

    REQUIRE_THAT(
      parseExpr(alloc, "1 % 2"),
      ASTEQ(
        "(#MOD\n"
        "  (int 1)\n"
        "  (int 2))\n"
      ));

    REQUIRE_THAT(
      parseExpr(alloc, "1 - 2 * 3"),
      ASTEQ(
        "(#SUB\n"
        "  (int 1)\n"
        "  (#MUL\n"
        "    (int 2)\n"
        "    (int 3)))\n"
      ));
  }

  SECTION("Terminals") {
    tempest::support::BumpPtrAllocator alloc;
    CHECK_THAT(parseExpr(alloc, "true"), ASTEQ("true\n"));
    CHECK_THAT(parseExpr(alloc, "false"), ASTEQ("false\n"));
    CHECK_THAT(parseExpr(alloc, "self"), ASTEQ("#SELF\n"));
    CHECK_THAT(parseExpr(alloc, "super"), ASTEQ("#SUPER\n"));
    CHECK_THAT(parseExpr(alloc, "X"), ASTEQ("X\n"));
    CHECK_THAT(parseExpr(alloc, "'X'"), ASTEQ("'X'\n"));

  #if 0
    def p_primary(self, p):
      '''primary : tuple_expr
                | array_lit
  .               | specialize
                | type_fn
  .               | member_ref
                | fluent_member_ref
  .               | dotid
  .               | null
  .               | string_lit
  .               | dec_int
  .               | hex_int
  .               | float
  .               | primitive_type'''
      p[0] = p[1]
      assert isinstance(p[0], ast.Node), type(p[0])
  #endif
  }
}
