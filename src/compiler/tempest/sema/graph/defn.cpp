#include "tempest/sema/graph/defn.h"
#include "tempest/sema/graph/type.h"
#include <assert.h>

namespace tempest::sema::graph {
  void Defn::formatModifiers(std::ostream& out) const {
    if (_visibility == PRIVATE) {
      out << "private ";
    } else if (_visibility == PROTECTED) {
      out << "protected ";
    }

    if (_static) {
      out << "static ";
    }
    if (_final) {
      out << "final ";
    }
    if (_override) {
      out << "override ";
    }
    if (_abstract) {
      out << "abstract ";
    }
    if (_undef) {
      out << "undef ";
    }
  }

  GenericDefn::~GenericDefn() {
    for (auto tparam : _typeParams) {
      delete tparam;
    }
    // for (auto iscope : _interceptScopes) {
    //   delete iscope.second;
    // }
  }

  TypeDefn::~TypeDefn() {
    for (Member* member : _members) {
      delete member;
    }
  }

  void TypeDefn::format(std::ostream& out) const {
    formatModifiers(out);
    switch (type()->kind) {
      case Type::Kind::CLASS: out << "class "; break;
      case Type::Kind::TRAIT: out << "trait "; break;
      case Type::Kind::STRUCT: out << "struct "; break;
      case Type::Kind::INTERFACE: out << "interface "; break;
      case Type::Kind::ENUM: out << "enum "; break;
      default:
        assert(false);
    }
    out << name();
  }

  void TypeParameter::format(std::ostream& out) const {
    formatModifiers(out);
    out << "tparam " << name();
  }

  void ValueDefn::format(std::ostream& out) const {
    formatModifiers(out);
    switch (kind) {
      case Kind::CONST: out << "let "; break;
      case Kind::VAR: out << "var "; break;
      default:
        assert(false);
    }
    out << name();
  }

  void EnumValueDefn::format(std::ostream& out) const {
    formatModifiers(out);
    out << "enum value " << name();
  }

  void ParameterDefn::format(std::ostream& out) const {
    formatModifiers(out);
    out << "param " << name();
  }

  FunctionDefn::~FunctionDefn() {
    for (ParameterDefn* param : _params) {
      delete param;
    }
  }

  void FunctionDefn::format(std::ostream& out) const {
    formatModifiers(out);
    out << "fn " << name();
  }
}
