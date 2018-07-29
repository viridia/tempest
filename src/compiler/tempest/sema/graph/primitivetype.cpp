#include "tempest/sema/graph/symboltable.hpp"
#include "tempest/sema/graph/defn.hpp"
// #include "tempest/sema/graph/expr.hpp"
#include "tempest/sema/graph/primitivetype.hpp"

namespace tempest::sema::graph {

  /** Void. */
  VoidType VoidType::VOID;

  /** Boolean. */
  BooleanType BooleanType::BOOL;

  /** Character. */
  IntegerType IntegerType::CHAR("char", 32, true, false);

  /** Signed integers. */
  IntegerType IntegerType::I8("i8", 8, false, false);
  IntegerType IntegerType::I16("i16", 16, false, false);
  IntegerType IntegerType::I32("i32", 32, false, false);
  IntegerType IntegerType::I64("i64", 64, false, false);

  /** Unsigned integers. */
  IntegerType IntegerType::U8("u8", 8, true, false);
  IntegerType IntegerType::U16("u16", 16, true, false);
  IntegerType IntegerType::U32("u32", 32, true, false);
  IntegerType IntegerType::U64("u64", 64, true, false);

  /** Positive signed integers. */
  IntegerType IntegerType::P8("literal i8", 8, false, true);
  IntegerType IntegerType::P16("literal i16", 16, false, true);
  IntegerType IntegerType::P32("literal i32", 32, false, true);
  IntegerType IntegerType::P64("literal i64", 64, false, true);

  /** Float types. */
  FloatType FloatType::F32("f32", 32);
  FloatType FloatType::F64("f64", 64);

  // void IntegerType::createConstant(const llvm::StringRef& name, int32_t value) {
  //   auto constVal = new (arena) IntegerLiteral(Location(), value, _unsigned);
  //   constVal->setType(this);
  // //  graphtools.encodeInt(constVal, value)

  //   auto v = new ValueDefn(Defn::Kind::LET, Location(), name, defn());
  //   v->setStatic(true);
  //   v->setType(this);
  //   v->setInit(constVal);
  //   v->setVisibility(Visibility::PUBLIC);

  //   defn()->members().push_back(v);
  //   defn()->memberScope()->addMember(v);
  // }

  // void IntegerType::createConstants(support::Arena& arena) {
  //   if (_unsigned) {
  //     createConstant(arena, "minVal", 0);
  //     createConstant(arena, "maxVal", ~(-1LL << _bits));
  //   } else if (_positive) {
  //     createConstant(arena, "minVal", 0);
  //     createConstant(arena, "maxVal", (1LL << (_bits - 1)) - 1);
  //   } else {
  //     createConstant(arena, "minVal", -1 << (_bits - 1));
  //     createConstant(arena, "maxVal", (1LL << (_bits - 1)) - 1);
  //   }
  // }


  // scope::SymbolScope* _scope = nullptr;
  //
  // scope::SymbolScope* PrimitiveType::scope() {
  //   if (_scope != nullptr) {
  //     return _scope;
  //   }
  //
  //   _scope = new scope::StandardScope(scope::SymbolScope::DEFAULT, "primitive types");
  //   _scope->addMember(IntegerType::I8.defn());
  // //   StandardScope(ScopeType st, const StringRef& description)
  // //     : _scopeType(st)
  // //     , _description(description.begin(), description.end())
  // //     , _owner(NULL)
  // //   {}
  //   return _scope;
  // }

  // void createConstants(support::Arena& arena) {
  //   IntegerType::I8.createConstants(arena);
  //   IntegerType::I16.createConstants(arena);
  //   IntegerType::I32.createConstants(arena);
  //   IntegerType::I64.createConstants(arena);
  //   IntegerType::U8.createConstants(arena);
  //   IntegerType::U16.createConstants(arena);
  //   IntegerType::U32.createConstants(arena);
  //   IntegerType::U64.createConstants(arena);
  //   IntegerType::P8.createConstants(arena);
  //   IntegerType::P16.createConstants(arena);
  //   IntegerType::P32.createConstants(arena);
  //   IntegerType::P64.createConstants(arena);
  // }

}
