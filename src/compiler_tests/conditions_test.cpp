#include "catch.hpp"
#include "tempest/sema/infer/conditions.hpp"

using tempest::sema::infer::Conditions;

TEST_CASE("Conditions", "[infer]") {
  Conditions c;

  SECTION("empty") {
    REQUIRE(c.empty());
    REQUIRE(c.begin() == c.end());

    c.conjoinWith(Conditions());
    REQUIRE(c.empty());
    REQUIRE(c.begin() == c.end());
    REQUIRE(c.numConjuncts() == 0);
  }

  SECTION("constructor") {
    Conditions c2(1, 2);
    REQUIRE_FALSE(c2.empty());
    REQUIRE_FALSE(c2.begin() == c2.end());
    REQUIRE(c2.numConjuncts() == 1);
  }

  SECTION("singular") {
    c.add(1, 2);
    REQUIRE_FALSE(c.empty());
    REQUIRE_FALSE(c.begin() == c.end());
    REQUIRE(c.numConjuncts() == 1);
    REQUIRE(c.begin()->site() == 1);
    REQUIRE(c.begin()->size() == 1);
    REQUIRE(*c.begin()->begin() == 2);

    // Comparisons
    REQUIRE(c == Conditions(1, 2));
    REQUIRE(c != Conditions());
    REQUIRE(c != Conditions(1, 3));
    REQUIRE(c != Conditions(2, 2));
  }

  SECTION("add same choice") {
    c.add(1, 2);
    c.add(1, 2);
    REQUIRE(c.numConjuncts() == 1);
    REQUIRE(c.begin()->site() == 1);
    REQUIRE(c.begin()->size() == 1);
    REQUIRE(*c.begin()->begin() == 2);
  }

  SECTION("add same site") {
    // Add out of order
    c.add(1, 2);
    c.add(1, 3);
    c.add(1, 1);
    REQUIRE_FALSE(c.empty());
    REQUIRE_FALSE(c.begin() == c.end());
    REQUIRE(c.numConjuncts() == 1);
    REQUIRE(c.begin()->site() == 1);
    REQUIRE(c.begin()->size() == 3);
    REQUIRE(c.begin()->begin()[0] == 1);
    REQUIRE(c.begin()->begin()[1] == 2);
    REQUIRE(c.begin()->begin()[2] == 3);
  }

  SECTION("add different sites") {
    // Add out of order
    c.add(1, 2);
    c.add(3, 2);
    c.add(2, 2);
    REQUIRE_FALSE(c.empty());
    REQUIRE_FALSE(c.begin() == c.end());
    REQUIRE(c.numConjuncts() == 3);
    REQUIRE(c.begin()[0].site() == 1);
    REQUIRE(c.begin()[1].site() == 2);
    REQUIRE(c.begin()[2].site() == 3);
  }

  SECTION("should be same regardless of order") {
    c.add(1, 2);
    c.add(1, 3);
    c.add(2, 4);
    Conditions c2;
    c2.add(2, 4);
    c2.add(1, 3);
    c2.add(1, 2);

    REQUIRE(c == c2);
  }

  SECTION("superset & subset of conjuncts") {
    c.add(1, 4);
    c.add(1, 2);
    c.add(1, 3);
    c.add(1, 5);
    Conditions c2;
    c2.add(1, 3);
    c2.add(1, 5);

    Conditions::Conjunct& cj0 = *c.begin();
    Conditions::Conjunct& cj1 = *c2.begin();
    REQUIRE(cj0.isSuperset(cj1));
    REQUIRE_FALSE(cj1.isSuperset(cj0));
    REQUIRE_FALSE(cj0.isSubset(cj1));
    REQUIRE(cj1.isSubset(cj0));
  }

  SECTION("subsets") {
    c.add(1, 4);
    c.add(1, 2);
    c.add(1, 3);
    c.add(1, 5);
    Conditions c2;
    c2.add(1, 3);
    c2.add(1, 5);

    REQUIRE(c2.isSubset(c));
    REQUIRE_FALSE(c.isSubset(c2));

    // Adding an additional site to the subset shouldn't affect the result
    c2.add(2, 5);
    REQUIRE(c2.isSubset(c));
    REQUIRE_FALSE(c.isSubset(c2));

    // But adding an additional site to the other side does.
    c.add(3, 5);
    REQUIRE_FALSE(c2.isSubset(c));
    REQUIRE_FALSE(c.isSubset(c2));
  }

  SECTION("subsets (2)") {
    c.add(1, 4);
    c.add(1, 2);
    c.add(1, 3);
    c.add(1, 5);
    c.add(2, 6);
    c.add(2, 7);
    Conditions c2;
    c2.add(1, 3);
    c2.add(1, 5);
    c2.add(2, 6);
    c2.add(3, 9);

    REQUIRE(c2.isSubset(c));
    REQUIRE_FALSE(c.isSubset(c2));
  }
}
