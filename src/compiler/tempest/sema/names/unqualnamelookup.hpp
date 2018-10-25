#ifndef TEMPEST_SEMA_NAMES_UNQUALNAMELOOKUP_HPP
#define TEMPEST_SEMA_NAMES_UNQUALNAMELOOKUP_HPP 1

#ifndef TEMPEST_COMMON_HPP
  #include "tempest/common.hpp"
#endif

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

#ifndef LLVM_ADT_SMALLPTRSET_H
  #include <llvm/ADT/SmallPtrSet.h>
#endif

namespace tempest::sema::graph {
  class SpecializationStore;
}

namespace tempest::sema::names {
  using tempest::sema::graph::Expr;
  using tempest::sema::graph::GenericDefn;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::FunctionDefn;
  using tempest::sema::graph::Member;
  using tempest::sema::graph::Module;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::NameCallback;
  using tempest::sema::graph::SymbolTable;
  using tempest::sema::graph::SpecializationStore;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::UserDefinedType;
  using tempest::sema::graph::MemberLookupResultRef;
  using tempest::source::Location;

  /** Abstract scope for looking up unqualified names. */
  struct LookupScope {
    LookupScope* prev;

    LookupScope(LookupScope* prev) : prev(prev) {}
    LookupScope() = delete;
    LookupScope(const LookupScope&) = delete;
    virtual ~LookupScope() {}
    virtual void lookup(const llvm::StringRef& name, MemberLookupResultRef& result) = 0;
    virtual void forEach(const NameCallback& nameFn) = 0;
    virtual bool addMember(Member* member) { return false; };

    // The 'subject' is the context which determines whether private variables are visible.
    // The basic rule is that any scope which defines private variables grants visibility
    // to any of its inner scopes, but only for those variables defined directly in that scope,
    // not variables defined in descendant scopes.
    virtual Member* subject() const = 0;
  };

  /** A lookup scope representing an enclosing module definition. */
  struct ModuleScope : public LookupScope {
    Module* module;

    ModuleScope(LookupScope* prev, Module* module) : LookupScope(prev), module(module) {}
    void lookup(const llvm::StringRef& name, MemberLookupResultRef& result);
    void forEach(const NameCallback& nameFn);
    Member* subject() const {
      return nullptr;
    }
  };

  /** A lookup scope representing an enclosing type definition. */
  struct TypeParamScope : public LookupScope {
    GenericDefn* generic;

    TypeParamScope(LookupScope* prev, GenericDefn* generic) : LookupScope(prev), generic(generic) {}
    void lookup(const llvm::StringRef& name, MemberLookupResultRef& result);
    void forEach(const NameCallback& nameFn);
    Member* subject() const {
      return generic;
    }
  };

  /** A lookup scope representing an enclosing type definition. */
  struct TypeDefnScope : public LookupScope {
    TypeDefn* typeDefn;
    SpecializationStore& spec;

    TypeDefnScope(LookupScope* prev, TypeDefn* typeDefn, SpecializationStore& spec)
      : LookupScope(prev)
      , typeDefn(typeDefn)
      , spec(spec)
    {}
    void lookup(const llvm::StringRef& name, MemberLookupResultRef& result);
    void forEach(const NameCallback& nameFn);
    Member* subject() const {
      return typeDefn;
    }
  };

  /** A lookup scope that includes the function parameters. */
  struct FunctionScope : public LookupScope {
    FunctionDefn* funcDefn;

    FunctionScope(LookupScope* prev, FunctionDefn* funcDefn)
      : LookupScope(prev)
      , funcDefn(funcDefn)
    {}
    void lookup(const llvm::StringRef& name, MemberLookupResultRef& result);
    void forEach(const NameCallback& nameFn);
    Member* subject() const {
      return funcDefn;
    }
  };

  /** A lookup scope representing an enclosing local scope. */
  struct LocalScope : public LookupScope {
    LocalScope(LookupScope* prev) : LookupScope(prev) {}
    void lookup(const llvm::StringRef& name, MemberLookupResultRef& result);
    void forEach(const NameCallback& nameFn);
    bool addMember(Member* member);
    Member* subject() const {
      return prev ? prev->subject() : nullptr;
    }

  private:
    SymbolTable _symbols;
  };
}

#endif
