#include "tempest/error/diagnostics.hpp"
#include "tempest/sema/names/createnameref.hpp"
#include "tempest/sema/names/unqualnamelookup.hpp"
#include "tempest/sema/graph/defn.hpp"
#include "tempest/sema/graph/expr.hpp"
#include "tempest/support/allocator.hpp"

namespace tempest::sema::names {
  using namespace tempest::sema::graph;
  using tempest::source::Location;
  using tempest::support::BumpPtrAllocator;
  using tempest::error::diag;

  Expr* createNameRef(
      support::BumpPtrAllocator& alloc,
      const Location& loc,
      const MemberLookupResultRef& result,
      Defn* subject,
      Expr* stem,
      bool preferPrivate,
      bool useADL) {
    MemberLookupResult vars;
    MemberLookupResult functions;
    MemberLookupResult types;
    MemberLookupResult privateMembers;
    MemberLookupResult protectedMembers;

    for (auto lr : result) {
      auto m = unwrapSpecialization(lr.member);
      if (!isVisibleMember(subject, m)) {
        if (m->visibility() == PRIVATE) {
          privateMembers.push_back({ m, lr.stem });
        } else if (m->visibility() == PROTECTED) {
          protectedMembers.push_back({ m, lr.stem });
        }
      }

      if (m->kind == Member::Kind::TYPE || m->kind == Member::Kind::TYPE_PARAM) {
        types.push_back(lr);
      } else if (m->kind == Member::Kind::FUNCTION) {
        functions.push_back(lr);
      } else if (m->kind == Member::Kind::VAR_DEF
          || m->kind == Member::Kind::ENUM_VAL
          || m->kind == Member::Kind::FUNCTION_PARAM
          || m->kind == Member::Kind::TUPLE_MEMBER) {
        vars.push_back(lr);
      } else {
        assert(false && "Other member types not allowed here.");
      }
    }

    if (privateMembers.size() > 0) {
      auto p = cast<Defn>(privateMembers[0].member);
      diag.error(loc) << "Cannot access private member '" << p->name() << "'.";
      diag.info(p->location()) << "Defined here.";
    } else if (protectedMembers.size() > 0) {
      auto p = cast<Defn>(protectedMembers[0].member);
      diag.error(loc) << "Cannot access protected member '" << p->name() << "'.";
      diag.info(p->location()) << "Defined here.";
    } else {
      auto m = unwrapSpecialization(result[0].member);
      if (vars.size() > 0) {
        if (functions.size() > 0 || types.size() > 0) {
          diag.error(loc) << "Conflicting definitions for '" << m->name() << "'.";
        } else if (vars.size() > 1) {
          diag.error(loc) << "Reference to '" << m->name() << "' is ambiguous.";
        } else {
          return new (alloc) DefnRef(Expr::Kind::VAR_REF, loc, m,
            stem ? stem : result[0].stem);
        }
      } else if (types.size() > 0) {
        if (functions.size() > 0) {
          diag.error(loc) << "Conflicting definitions for '" << m->name() << "'.";
        } else {
          auto tref = new (alloc) MemberListExpr(
              Expr::Kind::TYPE_REF_OVERLOAD, loc, result[0].member->name(), alloc.copyOf(types));
          return tref;
        }
      } else {
        auto mref = new (alloc) MemberListExpr(
            Expr::Kind::FUNCTION_REF_OVERLOAD,
            loc, result[0].member->name(), alloc.copyOf(functions));
        mref->useADL = useADL;
        mref->stem = stem;
        return mref;
      }
    }
    return nullptr;
  }
}
