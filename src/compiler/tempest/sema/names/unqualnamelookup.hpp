#ifndef TEMPEST_SEMA_NAMES_UNQUALNAMELOOKUP_HPP
#define TEMPEST_SEMA_NAMES_UNQUALNAMELOOKUP_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_MEMBER_HPP
  #include "tempest/sema/graph/member.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_MODULE_HPP
  #include "tempest/sema/graph/module.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_TYPE_HPP
  #include "tempest/sema/graph/type.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_SYMBOLTABLE_HPP
  #include "tempest/sema/graph/symboltable.hpp"
#endif

#ifndef LLVM_ADT_ARRAYREF_H
  #include <llvm/ADT/ArrayRef.h>
#endif

#ifndef LLVM_ADT_STRINGREF_H
  #include <llvm/ADT/StringRef.h>
#endif

#ifndef LLVM_ADT_SMALLPTRSET_H
  #include <llvm/ADT/SmallPtrSet.h>
#endif

namespace tempest::sema::names {
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::Member;
  using tempest::sema::graph::Module;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::NameCallback;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::UserDefinedType;
  using tempest::sema::graph::NameLookupResultRef;
  using tempest::source::Location;

  /** Abstract scope for looking up unqualified names. */
  struct LookupScope {
    LookupScope* prev;

    LookupScope(LookupScope* prev) : prev(prev) {}
    LookupScope() = delete;
    LookupScope(const LookupScope&) = delete;
    virtual void lookup(const llvm::StringRef& name, NameLookupResultRef& result) = 0;
    virtual void forEach(const NameCallback& nameFn) = 0;
  };

  /** A lookup scope representing an enclosing module definition. */
  struct ModuleScope : public LookupScope {
    Module* module;

    ModuleScope(LookupScope* prev, Module* module) : LookupScope(prev), module(module) {}
    void lookup(const llvm::StringRef& name, NameLookupResultRef& result);
    void forEach(const NameCallback& nameFn);
  };

  /** A lookup scope representing an enclosing type definition. */
  struct TypeDefnScope : public LookupScope {
    TypeDefn* typeDefn;

    TypeDefnScope(LookupScope* prev, TypeDefn* typeDefn) : LookupScope(prev), typeDefn(typeDefn) {}
    void lookup(const llvm::StringRef& name, NameLookupResultRef& result);
    void forEach(const NameCallback& nameFn);
  };

  /** A lookup scope representing an enclosing local scope. */
  struct LocalScope : public LookupScope {
    LocalScope(LookupScope* prev, TypeDefn* defn) : LookupScope(prev) {}
    void lookup(const llvm::StringRef& name, NameLookupResultRef& result);
    void forEach(const NameCallback& nameFn);
  };
}

#endif