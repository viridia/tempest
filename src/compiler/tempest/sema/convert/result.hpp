#ifndef TEMPEST_SEMA_CONVERT_RESULT_HPP
#define TEMPEST_SEMA_CONVERT_RESULT_HPP 1

namespace tempest::sema::convert {
  /** Indicates whether a type conversion is possible. */
  enum class ConversionRank {
    UNSET = 0,                // Conversion rank has not yet been determined
    ERROR,                    // Conversion is not possible.
    WARNING,                  // Possible, but with a warning.
    INEXACT,                  // Legal, but value is translated, for example 1 -> bool.
    EXACT,                    // Legal, and value is the same.
    IDENTICAL,                // Two sides have exactly the same type.
  };

  /** Details about a conversion error. */
  enum class ConversionError {
    NONE = 0,

    // Impossible conversions
    INCOMPATIBLE = 1,         // let x:String = 1       No conversion possible
    QUALIFIER_LOSS = 2,       // let x:T = immutable(T) Conversion would lose qualifiers

    //Lossy conversions (cause warning message to be emitted.)
    TRUNCATION = 3,           // let x:ubyte = 256      Value will be truncated
    SIGNED_UNSIGNED = 4,      // let x:uint = -1        Signed / unsigned mismatch
    PRECISION_LOSS = 5,       // let x:int = 1.2        Loss of decimal precision
    // INTEGER_TO_BOOL = 6,      // let x:bool = 256       Compare with 0
  };

  /** Result of a conversion. */
  struct ConversionResult {
    ConversionRank rank = ConversionRank::UNSET;
    ConversionError error = ConversionError::NONE;

    ConversionResult() : rank(ConversionRank::UNSET), error(ConversionError::NONE) {}
    ConversionResult(ConversionRank rank, ConversionError error = ConversionError::NONE)
      : rank(rank)
      , error(error)
    {}
    ConversionResult(const ConversionResult&) = default;

    /** Pick the worse of two conversion ranks. */
    const ConversionResult& worse(const ConversionResult& other) const {
      return
          rank == ConversionRank::UNSET ||
          (other.rank != ConversionRank::UNSET && other.rank < rank) ||
          (other.rank == rank && other.error < error)
        ? other : *this;
    }

    /** Pick the better of two conversion ranks. */
    const ConversionResult& better(const ConversionResult& other) const {
      return (other.rank > rank) || (other.rank == rank && other.error > error) ? other : *this;
    }

    /** Compare two conversion results for equality - used in unit tests. */
    friend bool operator==(const ConversionResult& l, const ConversionResult& r) {
      return l.rank == r.rank && l.error == r.error;
    }
  };

  inline ::std::ostream& operator<<(::std::ostream& os, const ConversionResult& cr) {
    os << "ConversionResult(";
    switch (cr.rank) {
      case ConversionRank::UNSET:
        os << "UNSET";
        break;
      case ConversionRank::ERROR:
        os << "ERROR";
        break;
      case ConversionRank::WARNING:
        os << "WARNING";
        break;
      case ConversionRank::INEXACT:
        os << "INEXACT";
        break;
      case ConversionRank::EXACT:
        os << "EXACT";
        break;
      case ConversionRank::IDENTICAL:
        os << "IDENTICAL";
        break;
    }
    os << ", ";
    switch (cr.error) {
      case ConversionError::NONE:
        os << "NONE";
        break;
      case ConversionError::INCOMPATIBLE:
        os << "INCOMPATIBLE";
        break;
      case ConversionError::QUALIFIER_LOSS:
        os << "QUALIFIER_LOSS";
        break;
      case ConversionError::TRUNCATION:
        os << "TRUNCATION";
        break;
      case ConversionError::SIGNED_UNSIGNED:
        os << "SIGNED_UNSIGNED";
        break;
      case ConversionError::PRECISION_LOSS:
        os << "PRECISION_LOSS";
        break;
    }
    os << ")";
    return os;
  }

}

#endif
