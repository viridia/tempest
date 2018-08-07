#include "tempest/parse/lexer.hpp"

#include <algorithm>
#include <cwctype>
#include <cassert>
#include <llvm/ADT/StringMap.h>

namespace tempest::parse {
  using llvm::StringRef;

  namespace {
    const char32_t EoF = char32_t(EOF);

    bool isNameStartChar(char32_t ch) {
      return std::iswalpha(ch) || ch == '_';
    }

    bool isNameChar(char32_t ch) {
      return std::iswalpha(ch) || (ch >= '0' && ch <= '9') || (ch == '_');
    }

    bool isDigitChar(char32_t ch) {
      return (ch >= '0' && ch <= '9');
    }

    bool isHexDigitChar(char32_t ch) {
      return ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
    }

    struct Keyword {
      StringRef keyword;
      TokenType token;
    };

    Keyword keywords[] = {
      { "abstract", TOKEN_ABSTRACT },
      { "and", TOKEN_AND },
      { "as", TOKEN_AS },
      { "bool", TOKEN_BOOL },
      { "break", TOKEN_BREAK },
      { "catch", TOKEN_CATCH },
      { "char", TOKEN_CHAR },
      { "class", TOKEN_CLASS },
      { "const", TOKEN_CONST },
      { "continue", TOKEN_CONTINUE },
      // { "def", TOKEN_DEF },
      { "else", TOKEN_ELSE },
      { "enum", TOKEN_ENUM },
      { "export", TOKEN_EXPORT },
      { "extend", TOKEN_EXTEND },
      { "extends", TOKEN_EXTENDS },
      { "false", TOKEN_FALSE },
      { "final", TOKEN_FINAL },
      { "finally", TOKEN_FINALLY },
      { "float", TOKEN_FLOAT },
      { "f32", TOKEN_FLOAT32 },
      { "f64", TOKEN_FLOAT64 },
      { "fn", TOKEN_FN },
      { "for", TOKEN_FOR },
      { "friend", TOKEN_FRIEND },
      { "from", TOKEN_FROM },
      { "i16", TOKEN_I16 },
      { "i32", TOKEN_I32 },
      { "i64", TOKEN_I64 },
      { "i8", TOKEN_I8 },
      { "id", TOKEN_ID },
      { "if", TOKEN_IF },
      { "implements", TOKEN_IMPLEMENTS },
      { "import", TOKEN_IMPORT },
      { "in", TOKEN_IN },
      { "int", TOKEN_INT },
      { "interface", TOKEN_INTERFACE },
      { "internal", TOKEN_INTERNAL },
      { "is", TOKEN_IS },
      { "let", TOKEN_LET },
      { "loop", TOKEN_LOOP },
      { "match", TOKEN_MATCH },
      { "not", TOKEN_NOT },
      { "null", TOKEN_NULL },
      { "object", TOKEN_OBJECT },
      { "or", TOKEN_OR },
      { "override", TOKEN_OVERRIDE },
      { "public", TOKEN_PUBLIC },
      { "private", TOKEN_PRIVATE },
      { "protected", TOKEN_PROTECTED },
      { "ref", TOKEN_REF },
      { "return", TOKEN_RETURN },
      { "self", TOKEN_SELF },
      { "static", TOKEN_STATIC },
      { "struct", TOKEN_STRUCT },
      { "super", TOKEN_SUPER },
      { "switch", TOKEN_SWITCH },
      { "throw", TOKEN_THROW },
      { "trait", TOKEN_TRAIT },
      { "true", TOKEN_TRUE },
      { "try", TOKEN_TRY },
      { "type", TOKEN_TYPE },
      { "u16", TOKEN_U16 },
      { "u32", TOKEN_U32 },
      { "u64", TOKEN_U64 },
      { "u8", TOKEN_U8 },
      { "uint", TOKEN_UINT },
      { "undef", TOKEN_UNDEF },
      // { "var", TOKEN_VAR },
      { "void", TOKEN_VOID },
      { "where", TOKEN_WHERE },
      { "while", TOKEN_WHILE },
      { "__intrinsic__", TOKEN_INTRINSIC },
      { "__tracemethod__", TOKEN_TRACEMETHOD },
      { "__unsafe__", TOKEN_UNSAFE },
    };

    typedef llvm::StringMap<TokenType> KeywordMap;

    template<size_t N>
    const KeywordMap& BuildIndex(Keyword (&keywords)[N]) {
      static KeywordMap map;
      for (size_t i = 0; i < N; ++i) {
        Keyword k = keywords[i];
        map[k.keyword] = k.token;
      }
      return map;
    }

    TokenType LookupKeyword(const StringRef& tokenValue) {
      static const KeywordMap& map = BuildIndex(keywords);
      const KeywordMap::const_iterator it = map.find(tokenValue);
      if (it != map.end()) {
        return it->second;
      }

      return TOKEN_ID;
    }
  }

  Lexer::Lexer(ProgramSource* src)
    : _src(src)
    , _stream(src->open())
    , _errorCode(ERROR_NONE)
  {
    _ch = 0;
    readCh();
    _tokenLocation.source = src;
    _tokenLocation.startLine = _line = 1;
    _tokenLocation.startCol = _col = 1;
  }

  inline void Lexer::readCh() {
    if (_ch != EoF) {
      _col += 1;
    }

    _ch = _stream.get();
  }

  TokenType Lexer::next() {

    // Whitespace loop
    for (;;) {
      if (_ch == EoF) {
        return TOKEN_END;
      } else if (_ch == ' ' || _ch == '\t' || _ch == '\b') {
        // Horizontal whitespace
        readCh();
      } else if (_ch == '\n') {
        // Linefeed
        readCh();
        _col = 1;
        _line += 1;
      } else if (_ch == '\r') {
        // Carriage return. Look for CRLF pair and count as 1 line.
        readCh();
        if (_ch == '\n') {
          readCh();
        }
        _col = 1;
        _line += 1;
      } else if (_ch == '/') {
        // Check for comment start
        readCh();
        DocComment * docComment = nullptr;
  //       SourceLocation commentLocation(tokenLocation_.file, currentOffset_, currentOffset_);
  //       llvm::SmallString<128> commentText;
        if (_ch == '/') {
          readCh();
          if (_ch == '/') {
            // Doc comment
            readCh();
  //           docComment = &docCommentFwd_;
  //           commentLocation.begin = currentOffset_;
          }

          while (_ch != EoF && _ch != '\n' && _ch != '\r') {
            if (docComment != nullptr) {
              // Expand tabs
              if (_ch == '\t') {
                while ((_col++ % 4) != 1) {
                  _commentText.push_back(' ');
                }
              } else {
                _commentText.push_back(_ch);
              }
            }
            readCh();
          }
          if (docComment != nullptr) {
            _commentText.push_back('\n');
  //           commentLocation.end = currentOffset_;
  //           docComment->entries().push_back(
  //               new DocComment::Entry(commentLocation, commentText));
          }
        } else if (_ch == '*') {
          readCh();
          if (_ch == '*') {
            // Doc comment
            readCh();
  //           docComment = &docCommentFwd_;
  //           clearDocComment();
            if (_ch == '<') {
  //             if (!docCommentFwd_.empty()) {
  //               diag.warn(tokenLocation_) << "Conflicting comment directions.";
  //             }
  //             docComment = &docCommentBwd_;
              readCh();
            }
  //           commentLocation.begin = currentOffset_;
          }

          for (;;) {
            if (_ch == EoF) {
              _errorCode = UNTERMINATED_COMMENT;
              return TOKEN_ERROR;
            }
            if (_ch == '*') {
              readCh();
              if (_ch == '/') {
                readCh();
                break;
              } else {
  //               if (docComment != nullptr) {
  //                 _commentText.push_back('*');
  //               }
                // Push the star we skipped, and reprocess the following char
              }
            } else {
              if (_ch == '\r' || _ch == '\n') {
                // Carriage return.
                // Look for CRLF pair and count as 1 line.
                if (_ch == '\r') {
                  readCh();
                }
                if (_ch == '\n') {
                  readCh();
                }
  //               if (docComment != nullptr) {
  //                 _commentText.push_back('\n');
  //               }
                _col = 1;
                _line += 1;
              } else {
                if (docComment != nullptr) {
                  // Expand tabs
                  if (_ch == '\t') {
                    while ((_col++ % 4) != 1) {
                      _commentText.push_back(' ');
                    }
                  } else {
                    _commentText.push_back(_ch);
                  }
                }
                readCh();
              }
            }
          }
  //         if (docComment != nullptr) {
  //           commentLocation.end = currentOffset_;
  //           docComment->entries().push_back(
  //               new DocComment::Entry(commentLocation, commentText));
  //         }
        } else {
          // What comes after a '/' char.
          _tokenLocation.startLine = _line;
          _tokenLocation.startCol = _col;
          if (_ch == '=') {
            readCh();
            return TOKEN_ASSIGN_DIV;
          }
          return TOKEN_DIV;
        }
      } else {
        break;
      }
    }

    _tokenLocation.startLine = _line;
    _tokenLocation.startCol = _col;
    _tokenValue.clear();

    // Identifier
    if (isNameStartChar(_ch)) {
      TokenType result = ident();
      _tokenLocation.endLine = _line;
      _tokenLocation.endCol = _col;
      return result;
    }

    // Number
    if (isDigitChar(_ch) || _ch == '.') {
      TokenType result = number();
      _tokenLocation.endLine = _line;
      _tokenLocation.endCol = _col;
      return result;
    }

    // Punctionation
    TokenType result = punc();
    _tokenLocation.endLine = _line;
    _tokenLocation.endCol = _col;
    return result;
  }

  TokenType Lexer::ident() {
    _tokenValue.push_back(_ch);
    readCh();
    while (isNameChar(_ch)) {
      _tokenValue.push_back(_ch);
      readCh();
    }

    // Check for keyword
    return LookupKeyword(_tokenValue);
  }

  TokenType Lexer::number() {
    bool isFloat = false;
    _tokenSuffix.clear();

    // Hex number check
    if (_ch == '0') {
      _tokenValue.push_back('0');
      readCh();
      if (_ch == 'X' || _ch == 'x') {
        _tokenValue.push_back('x');
        readCh();
        for (;;) {
          if (isHexDigitChar(_ch)) {
            _tokenValue.push_back(_ch);
            readCh();
          } else if (_ch == '_') {
            readCh();
          } else {
            break;
          }
        }

        // Unsigned suffix
        if (_ch == 'u' || _ch == 'U') {
          readCh();
          _tokenSuffix.push_back('u');
        }
        return TOKEN_HEX_INT_LIT;
      }
    }

    // Integer part
    for (;;) {
      if (isDigitChar(_ch)) {
        _tokenValue.push_back(_ch);
        readCh();
      } else if (_ch == '_') {
        readCh();
      } else {
        break;
      }
    }

    // Fractional part
    if (_ch == '.') {
      readCh();

      // Special case of '..' range token and '...' ellipsis token.
      if (_ch == '.') {
        if (!_tokenValue.empty()) {
          _stream.putback('.');
          // TODO: read suffix
          return TOKEN_DEC_INT_LIT;
        }
        readCh();
        if (_ch == '.') {
          readCh();
          return TOKEN_ELLIPSIS;
        }
        return TOKEN_RANGE;
      }

      // Check for case where this isn't a decimal point,
      // but just a dot token.
      if (!isDigitChar(_ch) && _tokenValue.empty()) {
        return TOKEN_DOT;
      }

      // It's a float
      isFloat = true;

      _tokenValue.push_back('.');
      for (;;) {
        if (isDigitChar(_ch)) {
          _tokenValue.push_back(_ch);
          readCh();
        } else if (_ch == '_') {
          readCh();
        } else {
          break;
        }
      }
    }

    // Exponent part
    if ((_ch == 'e' || _ch == 'E')) {
      isFloat = true;
      _tokenValue.push_back(_ch);
      readCh();
      if ((_ch == '+' || _ch == '-')) {
        _tokenValue.push_back(_ch);
        readCh();
      }
      for (;;) {
        if (isDigitChar(_ch)) {
          _tokenValue.push_back(_ch);
          readCh();
        } else if (_ch == '_') {
          readCh();
        } else {
          break;
        }
      }
    }

    if ((_ch == 'f' || _ch == 'F')) {
      isFloat = true;
      _tokenValue.push_back(_ch);
      readCh();
    }

    if (isFloat) {
      return TOKEN_FLOAT_LIT;
    }

    // Unsigned suffix
    if (_ch == 'u' || _ch == 'U') {
      readCh();
      _tokenSuffix.push_back('u');
    }
    return TOKEN_DEC_INT_LIT;
  }

  TokenType Lexer::punc() {
    switch (_ch) {
      case ':':
        readCh();
        if (_ch == '>') {
          readCh();
          return TOKEN_TYPE_GE;
        }
        return TOKEN_COLON;

      case '+':
        readCh();
        if (_ch == '+') {
          readCh();
          return TOKEN_INC;
        }
        if (_ch == '=') {
          readCh();
          return TOKEN_ASSIGN_PLUS;
        }
        return TOKEN_PLUS;

      case '-':
        readCh();
        if (_ch == '-') {
          readCh();
          return TOKEN_DEC;
        }
        if (_ch == '=') {
          readCh();
          return TOKEN_ASSIGN_MINUS;
        }
        if (_ch == '>') {
          readCh();
          return TOKEN_RETURNS;
        }
        return TOKEN_MINUS;

      case '*':
        readCh();
        if (_ch == '=') {
          readCh();
          return TOKEN_ASSIGN_MUL;
        }
        return TOKEN_MUL;

      case '%':
        readCh();
        if (_ch == '=') {
          readCh();
          return TOKEN_ASSIGN_MOD;
        }
        return TOKEN_MOD;

      case '^':
        readCh();
        if (_ch == '=') {
          readCh();
          return TOKEN_ASSIGN_BITXOR;
        }
        return TOKEN_CARET;

      case '|':
        readCh();
        if (_ch == '=') {
          readCh();
          return TOKEN_ASSIGN_BITOR;
        }
        return TOKEN_VBAR;

      case '&':
        readCh();
        if (_ch == '=') {
          readCh();
          return TOKEN_ASSIGN_BITAND;
        }
        return TOKEN_AMP;

      case '>':
        readCh();
        if (_ch == '=') {
          readCh();
          return TOKEN_GE;
        }
        if (_ch == '>') {
          readCh();
          if (_ch == '=') {
            readCh();
            return TOKEN_ASSIGN_RSHIFT;
          }
          return TOKEN_RSHIFT;
        }
        return TOKEN_GT;

      case '<':
        readCh();
        if (_ch == '=') {
          readCh();
          return TOKEN_LE;
        }
        if (_ch == ':') {
          readCh();
          return TOKEN_TYPE_LE;
        }
        if (_ch == '<') {
          readCh();
          if (_ch == '=') {
            readCh();
            return TOKEN_ASSIGN_LSHIFT;
          }
          return TOKEN_LSHIFT;
        }
        if (_ch == ':') {
          readCh();
          return TOKEN_TYPE_LE;
        }
        return TOKEN_LT;

      case '=':
        readCh();
        if (_ch == '=') {
          readCh();
          if (_ch == '=') {
            readCh();
            return TOKEN_REF_EQ;
          }
          return TOKEN_EQ;
        }
        if (_ch == '>') {
          readCh();
          return TOKEN_FAT_ARROW;
        }
        return TOKEN_ASSIGN;

      case '!':
        readCh();
        if (_ch == '=') {
          readCh();
          return TOKEN_NE;
        }
        _tokenValue.push_back(_ch);
        _errorCode = ILLEGAL_CHAR;
        return TOKEN_ERROR;

      case '{':
        readCh();
        return TOKEN_LBRACE;

      case '}':
        readCh();
        return TOKEN_RBRACE;

      case '[':
        readCh();
        return TOKEN_LBRACKET;

      case ']':
        readCh();
        return TOKEN_RBRACKET;

      case '(':
        readCh();
        return TOKEN_LPAREN;

      case ')':
        readCh();
        return TOKEN_RPAREN;

      case ';':
        readCh();
        return TOKEN_SEMI;

      case ',':
        readCh();
        return TOKEN_COMMA;

      case '?':
        readCh();
        return TOKEN_QMARK;

      case '@':
        readCh();
        return TOKEN_ATSIGN;

      case '"':
      case '\'': {
          // String literal
          char32_t quote = _ch;
          int charCount = 0;
          readCh();
          for (;;) {
            if (_ch == EoF) {
              _errorCode = UNTERMINATED_STRING;
              return TOKEN_ERROR;
            } else if (_ch == quote) {
              readCh();
              break;
            } else if (_ch == '\\') {
              readCh();
              if (_ch == EoF) {
                _errorCode = MALFORMED_ESCAPE_SEQUENCE;
                return TOKEN_ERROR;
              }
              if (!readEscapeChars()) {
                return TOKEN_ERROR;
              }
            } else if (_ch >= ' ') {
              _tokenValue.push_back(_ch);
              readCh();
            } else {
              _errorCode = MALFORMED_ESCAPE_SEQUENCE;
              return TOKEN_ERROR;
            }

            ++charCount;
          }

          if (quote == '\'') {
            if (charCount != 1) {
              _errorCode = (charCount == 0 ? EMPTY_CHAR_LITERAL : MULTI_CHAR_LITERAL);
              return TOKEN_ERROR;
            }
            return TOKEN_CHAR_LIT;
          } else {
            return TOKEN_STRING_LIT;
          }

          return quote == '"' ? TOKEN_STRING_LIT : TOKEN_CHAR_LIT;
        }

      default:
        break;
    }

    _tokenValue.push_back(_ch);
    _errorCode = ILLEGAL_CHAR;
    return TOKEN_ERROR;
  }

  bool Lexer::readEscapeChars() {
    // Assume that the initial backslash has already been read.
    switch (_ch) {
      case '0':
        _tokenValue.push_back('\0');
        readCh();
        break;
      case '\\':
        _tokenValue.push_back('\\');
        readCh();
        break;
      case '\'':
        _tokenValue.push_back('\'');
        readCh();
        break;
      case '\"':
        _tokenValue.push_back('\"');
        readCh();
        break;
      case 'r':
        _tokenValue.push_back('\r');
        readCh();
        break;
      case 'n':
        _tokenValue.push_back('\n');
        readCh();
        break;
      case 't':
        _tokenValue.push_back('\t');
        readCh();
        break;
      case 'b':
        _tokenValue.push_back('\b');
        readCh();
        break;
      case 'v':
        _tokenValue.push_back('\v');
        readCh();
        break;
      case 'x': {
        // Parse a hexidecimal character in a string.
        char charbuf[3];
        size_t  len = 0;
        readCh();
        while (isHexDigitChar(_ch) && len < 2) {
          charbuf[len++] = _ch;
          readCh();
        }

        if (len == 0) {
          _errorCode = MALFORMED_ESCAPE_SEQUENCE;
          return false;
        }

        charbuf[len] = 0;
        long charVal = ::strtoul(charbuf, nullptr, 16);
        _tokenValue.push_back(charVal);
        break;
      }

      case 'u':
      case 'U': {
        // Parse a Unicode character literal in a string.
        size_t maxLen = (_ch == 'u' ? 4 : 8);
        char charbuf[9];
        size_t len = 0;
        readCh();
        while (isHexDigitChar(_ch) && len < maxLen) {
          charbuf[len++] = _ch;
          readCh();
        }
        if (len == 0) {
          // TODO: Report it
          _errorCode = MALFORMED_ESCAPE_SEQUENCE;
          return false;
        }

        charbuf[len] = 0;
        assert(false && "Implement wide character escaped literals.");
  //       wchar_t charVal = ::strtoul(charbuf, nullptr, 16);
  //
  //       std::byte_string converted = std::wstring_convert(charVal);
  //       if (!encodeUnicodeChar(charVal)) {
  //         _errorCode = INVALID_UNICODE_CHAR;
  //         return false;
  //       }

        break;
      }

      default:
        _tokenValue.push_back(_ch);
        readCh();
        break;
    }

    return true;
  }

}
