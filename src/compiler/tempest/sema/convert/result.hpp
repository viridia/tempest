#ifndef TEMPEST_SEMA_CONVERT_RESULT_HPP
#define TEMPEST_SEMA_CONVERT_RESULT_HPP 1

#include <cstddef>
#include <iostream>

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

  const int NUM_RANKS = int(ConversionRank::IDENTICAL) + 1;

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

  /** A summary of the results of multiple conversions. */
  class ConversionRankTotals {
  public:
    size_t count[NUM_RANKS];

    ConversionRankTotals() {
      clear();
    }

    void clear() {
      for (size_t i = 0; i < NUM_RANKS; i += 1) {
        count[i] = 0;
      }
    }

    void setToWorst() {
      for (size_t i = 0; i < NUM_RANKS; i += 1) {
        count[i] = size_t(-1); // Maximum value of size_t
      }
    }

    bool isBetterThan(const ConversionRankTotals& other) {
      for (size_t i = 0; i < NUM_RANKS; i += 1) {
        if (count[i] < other.count[i]) {
          return true;
        } else if (count[i] > other.count[i]) {
          return false;
        }
      }
      return false;
    }

    bool equals(const ConversionRankTotals& other) {
      for (size_t i = 0; i < NUM_RANKS; i += 1) {
        if (count[i] != other.count[i]) {
          return false;
        }
      }
      return true;
    }

    bool isError() const {
      return count[int(ConversionRank::ERROR)] > 0 || count[int(ConversionRank::WARNING)] > 0;
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

  inline ::std::ostream& operator<<(::std::ostream& os, const ConversionRankTotals& cr) {
    os << "[" << cr.count[size_t(ConversionRank::ERROR)];
    os << ":" << cr.count[size_t(ConversionRank::WARNING)];
    os << ":" << cr.count[size_t(ConversionRank::INEXACT)];
    os << ":" << cr.count[size_t(ConversionRank::EXACT)];
    os << ":" << cr.count[size_t(ConversionRank::IDENTICAL)];
    os << "]";
    return os;
  }
}

#endif
