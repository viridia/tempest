#include "catch.hpp"
#include "tempest/sema/graph/expr_literal.hpp"
#include "tempest/sema/graph/expr_stmt.hpp"

using namespace tempest::sema::graph;
using tempest::source::Location;

TEST_CASE("Exprs", "[sema]") {
  SECTION("Block") {

    BlockStmt emptyBlock(Location(), {});
    REQUIRE(emptyBlock.stmts.size() == 0);
  }
}
