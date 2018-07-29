#include "catch.hpp"
#include "astmatcher.hpp"
#include "tempest/ast/literal.hpp"
#include "tempest/ast/oper.hpp"
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

/** Parse a module definition. */
Node* parseModule(llvm::BumpPtrAllocator& alloc, const char* srcText) {
  TestSource src(srcText);
  Parser parser(&src, alloc);
  return parser.module();
}

/** Parse an expression. */
Node* parseExpr(llvm::BumpPtrAllocator& alloc, const char* srcText) {
  TestSource src(srcText);
  Parser parser(&src, alloc);
  return parser.expression();
}

TEST_CASE("Parser", "[parse]") {
  const Location L;

  SECTION("Module") {
    llvm::BumpPtrAllocator alloc;
    REQUIRE_THAT(
      parseModule(alloc,
        "import { A } from b.c;\n"
        "import { A, B } from .c;\n"
        "export { C } from d.e.f;\n"
      ),
      ASTEQ(
        "(#MODULE\n"
        "  (#IMPORT 0 c.b\n"
        "    A)\n"
        "  (#IMPORT 1 c\n"
        "    A\n"
        "    B)\n"
        "  (#EXPORT 0 f.e.d\n"
        "    C))\n"
      ));
  }

  SECTION("InfixOperators") {
    llvm::BumpPtrAllocator alloc;
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
}
