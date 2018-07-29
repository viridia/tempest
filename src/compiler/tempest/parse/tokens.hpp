#ifndef TEMPEST_PARSE_TOKENS_HPP
#define TEMPEST_PARSE_TOKENS_HPP 1

#include <ostream>

namespace tempest::parse {

  #ifdef DEFINE_TOKEN
  #undef DEFINE_TOKEN
  #endif

  #define DEFINE_TOKEN(x) TOKEN_##x,

  enum TokenType {
    #include "tempest/parse/tokens.inc"
    TOKEN_LAST
  };

  // Return the name of the specified token.
  const char* GetTokenName(TokenType tt);

  // How to print a token type.
  inline ::std::ostream& operator<<(::std::ostream& os, TokenType tt) {
    return os << GetTokenName(tt);
  }
}

#endif
