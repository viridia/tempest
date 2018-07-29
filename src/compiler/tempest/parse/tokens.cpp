#include "tempest/parse/tokens.hpp"
#include <stdint.h>

#ifdef DEFINE_TOKEN
#undef DEFINE_TOKEN
#endif

#define DEFINE_TOKEN(x) #x,

namespace tempest::parse {
  const char* TOKEN_NAMES[] = {
    #include "tempest/parse/tokens.inc"
  };

  const char* GetTokenName(TokenType tt) {
    uint32_t index = (uint32_t)tt;
    if (index < TOKEN_LAST) {
      return TOKEN_NAMES[index];
    }

    return "<Invalid Token>";
  }
}
