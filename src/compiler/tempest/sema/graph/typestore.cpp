#include "tempest/sema/graph/defn.h"
#include "tempest/sema/graph/typestore.h"
#include "tempest/sema/graph/type.h"

namespace tempest::sema::graph {
  // Type* TypeStore::memberType(Member* m) {
  //   switch (m->kind()) {
  //     case Member::Kind::MODULE:
  //     case Member::Kind::PACKAGE:
  //       return &Type::ERROR;
  //     case Member::Kind::TYPE:
  //       assert(static_cast<TypeDefn*>(m)->type() != nullptr);
  //       return static_cast<TypeDefn*>(m)->type();
  //     case Member::Kind::FUNCTION:
  //       assert(false && "Unsupported kind: function.");
  //     case Member::Kind::PROPERTY:
  //       assert(false && "Unsupported kind: property.");
  //     case Member::Kind::LET:
  //     case Member::Kind::VAR:
  //     case Member::Kind::ENUM_VAL:
  //     case Member::Kind::PARAM:
  //     case Member::Kind::TUPLE_MEMBER:
  //       assert(static_cast<ValueDefn*>(m)->type() != nullptr);
  //       return static_cast<ValueDefn*>(m)->type();
  //     case Member::Kind::TYPE_PARAM:
  //       return static_cast<TypeParameter*>(m)->typeVar();
  //     case Member::Kind::SPECIALIZED:
  //       assert(false && "Unsupported kind: specialized.");
  //     default:
  //       assert(false && "Unsupported kind.");
  //   }
  //   assert(false);
  // }

  // Env TypeStore::createEnv(const Env& env) {
  //   // TODO: Definitely going to need unit tests for this.
  //   auto it = _envs.find(env);
  //   if (it != _envs.end()) {
  //     return *it;
  // //     return Env(Env::Bindings(it->data(), it->size()));
  //   }

  //   // Make a copy of the data on the arena
  //   auto data = reinterpret_cast<TypeBinding*>(_alloc.allocate(env.size() * sizeof(TypeBinding)));
  //   std::uninitialized_copy(env.begin(), env.end(), data);
  //   auto newEnv = semgraph::ImmutableEnv(data, env.size());
  //   _envs.insert(newEnv);
  //   return newEnv;
  // /*

  //   // Return a new environment object.
  //   return Env(Env::Bindings(data, env.size()));*/
  // }

  UnionType* TypeStore::createUnionType(const TypeArray& members) {
    // Sort members by pointer. This makes the type key hash independent of order.
    llvm::SmallVector<Type*, 4> sortedMembers(members.begin(), members.end());
    std::sort(sortedMembers.begin(), sortedMembers.end(), [](Type* a, Type* b) {
      return (uintptr_t)a < (uintptr_t)b;
    });

    // Return matching union instance if already exists.
    TypeKey key(sortedMembers);
    auto it = _unionTypes.find(key);
    if (it != _unionTypes.end()) {
      return it->second;
    }

    // Allocate type arrays of both the original and sorted orders.
    auto membersCopy = new (_alloc) TypeArray(members.begin(), members.end());
    auto ut = new (_alloc) UnionType(*membersCopy);
    auto keyCopy = new (_alloc) TypeArray(key.begin(), key.end());
    _unionTypes[TypeKey(*keyCopy)] = ut;
    return ut;
  }

  TupleType* TypeStore::createTupleType(const TypeArray& members) {
    TypeKey key = TypeKey(members);
    auto it = _tupleTypes.find(key);
    if (it != _tupleTypes.end()) {
      return it->second;
    }

    auto keyCopy = new (_alloc) TypeArray(key.begin(), key.end());
    auto tt = new (_alloc) TupleType(*keyCopy);
    _tupleTypes[TypeKey(tt->members)] = tt;
    return tt;
  }

  ConstType* TypeStore::createConstType(Type* base, bool provisional) {
    auto key = std::pair<Type*, bool>(base, provisional);
    auto it = _constTypes.find(key);
    if (it != _constTypes.end()) {
      return it->second;
    }

    auto ct = new (_alloc) ConstType(base, provisional);
    _constTypes[key] = ct;
    return ct;
  }

  FunctionType* TypeStore::createFunctionType(Type* returnType, const TypeArray& paramTypes) {
    std::vector<Type*> signature;
    signature.reserve(paramTypes.size() + 1);
    signature.push_back(returnType);
    signature.insert(signature.end(), paramTypes.begin(), paramTypes.end());
    TypeKey key = TypeKey(signature);
    auto it = _functionTypes.find(key);
    if (it != _functionTypes.end()) {
      return it->second;
    }
    auto paramTypesCopy = new (_alloc) TypeArray(paramTypes);
    auto signatureCopy = new (_alloc) TypeArray(signature);
    auto ft = new (_alloc) FunctionType(returnType, *paramTypesCopy, false);
    _functionTypes[TypeKey(*signatureCopy)] = ft;
    return ft;
  }

  FunctionType* TypeStore::createFunctionType(
      Type* returnType,
      const llvm::ArrayRef<ParameterDefn*>& params) {
    std::vector<Type*> paramTypes;
    paramTypes.reserve(params.size());
    for (auto param : params) {
      paramTypes.push_back(param->type());
    }
    return createFunctionType(returnType, paramTypes);
  }
}
