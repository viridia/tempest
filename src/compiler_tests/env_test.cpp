#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "tempest/sema/graph/env.h"
#include "tempest/sema/graph/type.h"
#include <memory>

using tempest::sema::graph::Env;
using tempest::sema::graph::SmallEnv;
using tempest::sema::graph::Type;
using tempest::sema::graph::TypeVar;
using tempest::sema::graph::TypeParameter;

TEST_CASE("Environments", "[env]") {
  Env env;
  SmallEnv<4> smEnv;
  auto tv1 = std::make_unique<TypeVar>((TypeParameter *)NULL);
  TypeVar tv2(NULL);
  Type t0(Type::Kind::BOOLEAN);
  Type t1(Type::Kind::INTEGER);

  SECTION("empty") {
    REQUIRE(env.size() == 0);
    REQUIRE(env.begin() == env.end());

    REQUIRE(smEnv.size() == 0);
    REQUIRE(smEnv.begin() == smEnv.end());
  }

  SECTION("insert") {
    smEnv[tv1.get()] = &t0;
    REQUIRE(smEnv.size() == 1);
    REQUIRE(smEnv.find(tv1.get()) != smEnv.end());
    REQUIRE(smEnv.find(tv1.get())->second == &t0);
  }

  SECTION("equals") {
    SmallEnv<4> smEnv2;
    SmallEnv<4> smEnv3;
    smEnv[tv1.get()] = &t0;
    smEnv2[tv1.get()] = &t0;
    smEnv3[tv1.get()] = &t1;
    REQUIRE(smEnv == smEnv2);
    REQUIRE(smEnv != smEnv3);
  }
}
