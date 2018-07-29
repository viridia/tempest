#ifndef TEMPEST_PARSE_LEXER_HPP
#define TEMPEST_PARSE_LEXER_HPP 1

#ifndef TEMPEST_AST_NODE_HPP
  #include "tempest/ast/node.hpp"
#endif

#ifndef TEMPEST_SOURCE_PROGRAMSOURCE_HPP
  #include "tempest/source/programsource.hpp"
#endif

#ifndef TEMPEST_SOURCE_DOCCOMMENT_HPP
  #include "tempest/source/doccomment.hpp"
#endif

#ifndef TEMPEST_SOURCE_LOCATION_HPP
  #include "tempest/source/location.hpp"
#endif

#ifndef TEMPEST_PARSE_TOKENS_HPP
  #include "tempest/parse/tokens.hpp"
#endif

#ifndef LLVM_ADT_STRINGREF_H
  #include <llvm/ADT/StringRef.h>
#endif

#include <istream>

namespace tempest::parse {
  using tempest::source::DocComment;
  using tempest::source::ProgramSource;
  using tempest::source::Location;

  /** Lexical analyzer. */
  class Lexer {
  public:
    enum LexerError {
      ERROR_NONE = 0,
      ILLEGAL_CHAR,
      UNTERMINATED_COMMENT,
      UNTERMINATED_STRING,
      MALFORMED_ESCAPE_SEQUENCE,
      INVALID_UNICODE_CHAR,
      EMPTY_CHAR_LITERAL,
      MULTI_CHAR_LITERAL,
    };

    /** Constructor */
    Lexer(ProgramSource* src);

    /** Destructor closes the stream. */
    ~Lexer() {
      _src->close();
    }

    /** Get the next token */
    TokenType next();

    /** Current value of the token. */
    const std::string& tokenValue() const { return _tokenValue; }
    std::string& tokenValue() { return _tokenValue; }

    /** Suffix for numeric tokens. */
    const std::string& tokenSuffix() const { return _tokenSuffix; }
    std::string& tokenSuffix() { return _tokenSuffix; }

    /** Location of the token in the source file. */
    const Location& tokenLocation() const { return _tokenLocation; }

  //   /** Get the current accumulated doc comment for this token. */
  //   DocComment& docComment(CommentDirection dir) {
  //     return dir == FORWARD ? docCommentFwd_ : docCommentBwd_;
  //   }

    /** Clear the accumulated doc comment. */
    void clearDocComment();

    /** Get the current doc comment, and transfer its contents to 'dst', which must be empty.
        'direction' specifies whether we want a forward or backward comment; If the current
        comment's direction is not the requested direction, then no action is taken.
    */
  //   void takeDocComment(DocComment& dst, CommentDirection direction = FORWARD);

    /** Current error code. */
    LexerError errorCode() const { return _errorCode; }

    /** Add 'charVal' to the current token value, encoded as UTF-8. */
    bool encodeUnicodeChar(long charVal);

  private:
    // Source file containing the buffer
    ProgramSource*    _src;           /** Pointer to source file buffer */
    std::istream&     _stream;        /** Input stream. */
    char32_t          _ch;            /** Previously read char. */
    uint32_t          _line;          /** Line number of current read position. */
    uint32_t          _col;           /** Column number of current read position. */
    Location          _tokenLocation; /** Current token location. */
    std::string       _tokenValue;    /** String value of token. */
    std::string       _tokenSuffix;   /** Numeric suffix. */
    std::string       _commentText;   /** Text of the doc comment. */
    DocComment*       _docComment;    /** Accumulated doc comment. */
    LexerError        _errorCode;     /** Error code. */

    // Read the next character.
    void readCh();
    bool readEscapeChars();
    TokenType ident();
    TokenType number();
    TokenType punc();
  };

}

#endif