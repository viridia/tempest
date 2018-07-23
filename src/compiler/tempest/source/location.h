#ifndef TEMPEST_SOURCE_LOCATION_H
#define TEMPEST_SOURCE_LOCATION_H 1

#ifndef TEMPEST_CONFIG_H
  #include "config.h"
#endif

#ifndef TEMPEST_SOURCE_PROGRAMSOURCE_H
  #include "tempest/source/programsource.h"
#endif

#include <ostream>
#include <algorithm>

namespace tempest::source {
  /** Represents a location within a source file. */
  struct Location {
    ProgramSource* source;
    int32_t startLine;
    int16_t startCol;
    int32_t endLine;
    int16_t endCol;

    Location()
      : source(NULL)
      , startLine(0)
      , startCol(0)
      , endLine(0)
      , endCol(0)
    {
    }

    Location(
      ProgramSource* src,
      int32_t startLn,
      int16_t startCl,
      int32_t endLn,
      int16_t endCl)
      : source(src)
      , startLine(startLn)
      , startCol(startCl)
      , endLine(endLn)
      , endCol(endCl)
    {
    }

    Location(const Location& src)
      : source(src.source)
      , startLine(src.startLine)
      , startCol(src.startCol)
      , endLine(src.endLine)
      , endCol(src.endCol)
    {
    }

    Location unionWith(const Location& right) const {
      if (this->source != right.source) {
        if (this->source == NULL) {
          return right;
        } else {
          return *this;
        }
      } else {
        Location result;
        result.source = this->source;
        if (this->startLine < right.startLine) {
          result.startLine = this->startLine;
          result.startCol = this->startCol;
        } else if (this->startLine > right.startLine) {
          result.startLine = right.startLine;
          result.startCol = right.startCol;
        } else {
          result.startLine = this->startLine;
          result.startCol = std::min(this->startCol, right.startCol);
        }
        if (this->endLine > right.endLine) {
          result.endLine = this->endLine;
          result.endCol = this->endCol;
        } else if (this->endLine < right.endLine) {
          result.endLine = right.endLine;
          result.endCol = right.endCol;
        } else {
          result.endLine = this->endLine;
          result.endCol = std::max(this->endCol, right.endCol);
        }
        return result;
      }
    }

    /** Compute a source location that is the union of two locations. */
    inline friend Location operator|(const Location& left, const Location& right) {
      return left.unionWith(right);
    }

    Location& operator|=(const Location& right) {
      *this = this->unionWith(right);
      return *this;
    }
  };

  // How to print a location.
  inline ::std::ostream& operator<<(::std::ostream& os, const Location& loc) {
    if (loc.source) {
      os.write(loc.source->path().begin(), loc.source->path().size());
      os << ":" << loc.startLine << ":" << loc.startCol;
    } else {
      os << "(unknown location)";
    }
    return os;
  }

  class Locatable {
  public:
    virtual const Location& getLocation() const = 0;
  };
}

namespace llvm {
  // How to print a StringRef.
  inline ::std::ostream& operator<<(::std::ostream& os, const llvm::StringRef& str) {
    os.write(str.begin(), str.size());
    return os;
  }
}

#endif
