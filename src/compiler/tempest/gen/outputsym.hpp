#ifndef TEMPEST_GEN_OUTPUTSYM_HPP
#define TEMPEST_GEN_OUTPUTSYM_HPP 1

#ifndef TEMPEST_SEMA_GRAPH_DEFN_HPP
  #include "tempest/sema/graph/defn.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_ENV_HPP
  #include "tempest/sema/graph/env.hpp"
#endif

#ifndef TEMPEST_SEMA_GRAPH_SPECSTORE_HPP
  #include "tempest/sema/graph/specstore.hpp"
#endif

namespace llvm {
  class Function;
  class GlobalVariable;
}

namespace tempest::gen {
  using tempest::sema::graph::Env;
  using tempest::sema::graph::Expr;
  using tempest::sema::graph::FunctionDefn;
  using tempest::sema::graph::Member;
  using tempest::sema::graph::Type;
  using tempest::sema::graph::TypeDefn;
  using tempest::sema::graph::ValueDefn;
  using tempest::sema::graph::SpecializationKey;
  using tempest::sema::graph::SpecializationKeyHash;

  class ClassInterfaceTranslationSym;

  /** Base class for output symbols. */
  class OutputSym {
  public:
    enum class Kind {
      FUNCTION,
      CLS_DESC,
      IFACE_DESC,
      GLOBAL,
      CLS_IFACE_TRANS,
    };

    const Kind kind;

    // Linkage name for this symbol.
    StringRef linkageName;

    // Array of type arguments for generic definition.
    ArrayRef<const Type*> typeArgs;

    // If true, this was a template expansion.
    bool linkOnce = false;

    OutputSym(Kind kind, ArrayRef<const Type*> typeArgs) : kind(kind), typeArgs(typeArgs) {}
  };

  /** A symbol representing a function. */
  class FunctionSym : public OutputSym {
  public:
    /** The original function. */
    FunctionDefn* function;

    /** The template-expansion of the function body. */
    Expr* body = nullptr;

    /** LLVM function value. */
    llvm::Function* fnVal = nullptr;

    FunctionSym(FunctionDefn* function, ArrayRef<const Type*> typeArgs)
      : OutputSym(Kind::FUNCTION, typeArgs)
      , function(function)
    {}

    /** Dynamic casting support. */
    static bool classof(const FunctionSym* m) { return true; }
    static bool classof(const OutputSym* m) { return m->kind == Kind::FUNCTION; }
  };

  /** A reference to a class definition symbol. */
  class ClassDescriptorSym : public OutputSym {
  public:
    /** The class definition. */
    TypeDefn* typeDefn;

    /** Type descriptor constant. */
    llvm::GlobalVariable* desc = nullptr;

    /** Method table for this class. */
    ArrayRef<FunctionSym*> methodTable;

    /** Reference to base class. */
    ClassDescriptorSym* baseClsSym = nullptr;

    /** Table of implemented interfaces. */
    ArrayRef<ClassInterfaceTranslationSym*> interfaceTable;

    ClassDescriptorSym(TypeDefn* typeDefn, ArrayRef<const Type*> typeArgs)
      : OutputSym(Kind::CLS_DESC, typeArgs)
      , typeDefn(typeDefn)
    {}

    /** Dynamic casting support. */
    static bool classof(const ClassDescriptorSym* m) { return true; }
    static bool classof(const OutputSym* m) { return m->kind == Kind::CLS_DESC; }
  };

  /** A reference to a class definition symbol. */
  class InterfaceDescriptorSym : public OutputSym {
  public:
    /** The interface definition. */
    TypeDefn* typeDefn;

    /** Type descriptor constant. */
    llvm::GlobalVariable* desc = nullptr;

    InterfaceDescriptorSym(TypeDefn* typeDefn, ArrayRef<const Type*> typeArgs)
      : OutputSym(Kind::IFACE_DESC, typeArgs)
      , typeDefn(typeDefn)
    {}

    /** Dynamic casting support. */
    static bool classof(const InterfaceDescriptorSym* m) { return true; }
    static bool classof(const OutputSym* m) { return m->kind == Kind::IFACE_DESC; }
  };

  /** A binding between a class and an interface. */
  class ClassInterfaceTranslationSym : public OutputSym {
  public:
    ClassDescriptorSym* cls;
    InterfaceDescriptorSym* iface;

    /** Method table for this class. */
    ArrayRef<FunctionSym*> methodTable;

    /** Type descriptor constant. */
    llvm::GlobalVariable* desc = nullptr;

    ClassInterfaceTranslationSym(ClassDescriptorSym* cls, InterfaceDescriptorSym* iface)
      : OutputSym(Kind::CLS_IFACE_TRANS, llvm::ArrayRef<Type*>())
      , cls(cls)
      , iface(iface)
    {}

    /** Dynamic casting support. */
    static bool classof(const ClassInterfaceTranslationSym* m) { return true; }
    static bool classof(const OutputSym* m) { return m->kind == Kind::CLS_IFACE_TRANS; }
  };

  /** A reference to a global or static variable. */
  class GlobalVarSym : public OutputSym {
  public:
    /** The variable definition. */
    ValueDefn* varDefn;
    llvm::GlobalVariable* global = nullptr;

    GlobalVarSym(ValueDefn* varDefn, ArrayRef<const Type*> typeArgs)
      : OutputSym(Kind::GLOBAL, typeArgs)
      , varDefn(varDefn)
    {}

    /** Dynamic casting support. */
    static bool classof(const GlobalVarSym* m) { return true; }
    static bool classof(const OutputSym* m) { return m->kind == Kind::GLOBAL; }
  };
}

#endif
