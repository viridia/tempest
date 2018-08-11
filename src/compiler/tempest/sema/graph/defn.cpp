#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/type.hpp"
#include <assert.h>

namespace tempest::sema::graph {
  // void Defn::formatModifiers(std::ostream& out) const {
  //   if (_visibility == PRIVATE) {
  //     out << "private ";
  //   } else if (_visibility == PROTECTED) {
  //     out << "protected ";
  //   }

  //   if (_static) {
  //     out << "static ";
  //   }
  //   if (_final) {
  //     out << "final ";
  //   }
  //   if (_override) {
  //     out << "override ";
  //   }
  //   if (_abstract) {
  //     out << "abstract ";
  //   }
  //   if (_undef) {
  //     out << "undef ";
  //   }
  // }

  GenericDefn::~GenericDefn() {
    // for (auto tparam : _typeParams) {
    //   delete tparam;
    // }
    // for (auto iscope : _interceptScopes) {
    //   delete iscope.second;
    // }
  }

  TypeDefn::~TypeDefn() {
    for (Member* member : _members) {
      delete member;
    }
  }

  FunctionDefn::~FunctionDefn() {
    // for (ParameterDefn* param : _params) {
    //   delete param;
    // }
  }
}
