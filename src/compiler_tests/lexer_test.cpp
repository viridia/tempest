#include "catch.hpp"
#include "tempest/parse/lexer.hpp"

using namespace tempest::parse;
using tempest::source::Location;

class TestSource : public tempest::source::StringSource {
public:
  TestSource(llvm::StringRef source)
    : tempest::source::StringSource("test.txt", source)
  {}
};

/** A function that scans a single token. */
TokenType lexToken(const char* srcText) {
    TestSource src(srcText);
    Lexer lex(&src);

    TokenType result = lex.next();
    REQUIRE(lex.next() == TOKEN_END);
    return result;
}

/** A function that scans a single token and returns an error. */
TokenType lexTokenError(const char* srcText) {
    TestSource src(srcText);
    Lexer lex(&src);
    return lex.next();
}

TEST_CASE("Lexer", "[parse]") {
  SECTION("SingleTokens") {
    REQUIRE(lexToken("") == TOKEN_END);
    REQUIRE(lexToken(" ") == TOKEN_END);
    REQUIRE(lexToken("\t") == TOKEN_END);
    REQUIRE(lexToken("\r\n") == TOKEN_END);

    // Idents
    REQUIRE(lexToken("_") == TOKEN_ID);
    REQUIRE(lexToken("a") == TOKEN_ID);
    REQUIRE(lexToken("z") == TOKEN_ID);
    REQUIRE(lexToken("azAZ_01") == TOKEN_ID);
    REQUIRE(lexToken(" z ") == TOKEN_ID);

    // Numbers
    REQUIRE(lexToken("0") == TOKEN_DEC_INT_LIT);
    REQUIRE(lexToken(" 0 ") == TOKEN_DEC_INT_LIT);
    REQUIRE(lexToken("1") == TOKEN_DEC_INT_LIT);
    REQUIRE(lexToken("9") == TOKEN_DEC_INT_LIT);
    REQUIRE(lexToken("10") == TOKEN_DEC_INT_LIT);
    REQUIRE(lexToken("0x10af") == TOKEN_HEX_INT_LIT);
    REQUIRE(lexToken("0X10af") == TOKEN_HEX_INT_LIT);
    REQUIRE(lexToken("0.") == TOKEN_FLOAT_LIT);
    REQUIRE(lexToken(" 0. ") == TOKEN_FLOAT_LIT);
    REQUIRE(lexToken(".0") == TOKEN_FLOAT_LIT);
    REQUIRE(lexToken("0f") == TOKEN_FLOAT_LIT);
    REQUIRE(lexToken("0e12") == TOKEN_FLOAT_LIT);
    REQUIRE(lexToken("0e+12") == TOKEN_FLOAT_LIT);
    REQUIRE(lexToken("0e-12") == TOKEN_FLOAT_LIT);
    REQUIRE(lexToken("0.0e12f") == TOKEN_FLOAT_LIT);

    // Grouping tokens
    REQUIRE(lexToken("{") == TOKEN_LBRACE);
    REQUIRE(lexToken("}") == TOKEN_RBRACE);
    REQUIRE(lexToken("(") == TOKEN_LPAREN);
    REQUIRE(lexToken(")") == TOKEN_RPAREN);
    REQUIRE(lexToken("[") == TOKEN_LBRACKET);
    REQUIRE(lexToken("]") == TOKEN_RBRACKET);

    // Delimiters
    REQUIRE(lexToken(";") == TOKEN_SEMI);
    REQUIRE(lexToken(":") == TOKEN_COLON);
    REQUIRE(lexToken(",") == TOKEN_COMMA);
    REQUIRE(lexToken("@") == TOKEN_ATSIGN);

    // Operator tokens
    REQUIRE(lexToken("=") == TOKEN_ASSIGN);
    REQUIRE(lexToken("+=") == TOKEN_ASSIGN_PLUS);
    REQUIRE(lexToken("-=") == TOKEN_ASSIGN_MINUS);
    REQUIRE(lexToken("*=") == TOKEN_ASSIGN_MUL);
    REQUIRE(lexToken("/=") == TOKEN_ASSIGN_DIV);
    REQUIRE(lexToken("%=") == TOKEN_ASSIGN_MOD);
    REQUIRE(lexToken(">>=") == TOKEN_ASSIGN_RSHIFT);
    REQUIRE(lexToken("<<=") == TOKEN_ASSIGN_LSHIFT);
    REQUIRE(lexToken("&=") == TOKEN_ASSIGN_BITAND);
    REQUIRE(lexToken("|=") == TOKEN_ASSIGN_BITOR);
    REQUIRE(lexToken("^=") == TOKEN_ASSIGN_BITXOR);
    //REQUIRE(lexToken("") == TOKEN_ASSIGNOP);
    REQUIRE(lexToken("->") == TOKEN_RETURNS);
    REQUIRE(lexToken("=>") == TOKEN_FAT_ARROW);
    REQUIRE(lexToken("+") == TOKEN_PLUS);
    REQUIRE(lexToken("-") == TOKEN_MINUS);
    REQUIRE(lexToken("*") == TOKEN_MUL);
    REQUIRE(lexToken("/") == TOKEN_DIV);
    REQUIRE(lexToken("&") == TOKEN_AMP);
    REQUIRE(lexToken("%") == TOKEN_MOD);
    REQUIRE(lexToken("|") == TOKEN_VBAR);
    REQUIRE(lexToken("^") == TOKEN_CARET);
  //   REQUIRE(lexToken("~") == TOKEN_TILDE);
  //   REQUIRE(lexToken("!") == TOKEN_EXCLAM);
    REQUIRE(lexToken("?") == TOKEN_QMARK);
    REQUIRE(lexToken("++") == TOKEN_INC);
    REQUIRE(lexToken("--") == TOKEN_DEC);
  //   REQUIRE(lexToken("&&") == TOKEN_DOUBLEAMP);
  //   REQUIRE(lexToken("||") == TOKEN_DOUBLEBAR);
  //   REQUIRE(lexToken("::") == TOKEN_DOUBLECOLON);

    // Relational operators
    REQUIRE(lexToken("<") == TOKEN_LT);
    REQUIRE(lexToken(">") == TOKEN_GT);
    REQUIRE(lexToken("<=") == TOKEN_LE);
    REQUIRE(lexToken(">=") == TOKEN_GE);
    REQUIRE(lexToken("==") == TOKEN_EQ);
    REQUIRE(lexToken("!=") == TOKEN_NE);
    REQUIRE(lexToken("===") == TOKEN_REF_EQ);
    REQUIRE(lexToken("<:") == TOKEN_TYPE_LE);
    REQUIRE(lexToken(":>") == TOKEN_TYPE_GE);

    REQUIRE(lexToken("<<") == TOKEN_LSHIFT);
    REQUIRE(lexToken(">>") == TOKEN_RSHIFT);
    //REQUIRE(lexToken("") == TOKEN_SCOPE);

    // Joiners
    REQUIRE(lexToken(".") == TOKEN_DOT);
    REQUIRE(lexToken("..") == TOKEN_RANGE);
    REQUIRE(lexToken("...") == TOKEN_ELLIPSIS);

    // Operator keywords
    REQUIRE(lexToken("and") == TOKEN_AND);
    REQUIRE(lexToken("or") == TOKEN_OR);
    REQUIRE(lexToken("not") == TOKEN_NOT);
    REQUIRE(lexToken("as") == TOKEN_AS);
    REQUIRE(lexToken("is") == TOKEN_IS);
    REQUIRE(lexToken("in") == TOKEN_IN);

    // Access Keywords
    REQUIRE(lexToken("public") == TOKEN_PUBLIC);
    REQUIRE(lexToken("private") == TOKEN_PRIVATE);
    REQUIRE(lexToken("protected") == TOKEN_PROTECTED);
    REQUIRE(lexToken("internal") == TOKEN_INTERNAL);

    // Primtypes
    REQUIRE(lexToken("bool") == TOKEN_BOOL);
    REQUIRE(lexToken("char") == TOKEN_CHAR);
    REQUIRE(lexToken("int") == TOKEN_INT);
    REQUIRE(lexToken("i8") == TOKEN_I8);
    REQUIRE(lexToken("i16") == TOKEN_I16);
    REQUIRE(lexToken("i32") == TOKEN_I32);
    REQUIRE(lexToken("i64") == TOKEN_I64);
    REQUIRE(lexToken("u8") == TOKEN_U8);
    REQUIRE(lexToken("u16") == TOKEN_U16);
    REQUIRE(lexToken("u32") == TOKEN_U32);
    REQUIRE(lexToken("u64") == TOKEN_U64);
    REQUIRE(lexToken("float") == TOKEN_FLOAT);
    REQUIRE(lexToken("f32") == TOKEN_FLOAT32);
    REQUIRE(lexToken("f64") == TOKEN_FLOAT64);

    // Metatypes
    REQUIRE(lexToken("class") == TOKEN_CLASS);
    REQUIRE(lexToken("struct") == TOKEN_STRUCT);
    REQUIRE(lexToken("enum") == TOKEN_ENUM);
    // REQUIRE(lexToken("var") == TOKEN_VAR);
    REQUIRE(lexToken("let") == TOKEN_LET);
    // REQUIRE(lexToken("def") == TOKEN_DEF);

    REQUIRE(lexToken("import") == TOKEN_IMPORT);
    REQUIRE(lexToken("export") == TOKEN_EXPORT);

    // Statement keywords
    REQUIRE(lexToken("if") == TOKEN_IF);
    REQUIRE(lexToken("else") == TOKEN_ELSE);
    REQUIRE(lexToken("loop") == TOKEN_LOOP);
    REQUIRE(lexToken("for") == TOKEN_FOR);
    REQUIRE(lexToken("while") == TOKEN_WHILE);
    REQUIRE(lexToken("return") == TOKEN_RETURN);

    REQUIRE(lexToken("try") == TOKEN_TRY);
    REQUIRE(lexToken("catch") == TOKEN_CATCH);
    REQUIRE(lexToken("finally") == TOKEN_FINALLY);
    REQUIRE(lexToken("switch") == TOKEN_SWITCH);
    REQUIRE(lexToken("match") == TOKEN_MATCH);

    // String literals
    REQUIRE(lexToken("\"\"") == TOKEN_STRING_LIT);
    REQUIRE(lexToken("'a'") == TOKEN_CHAR_LIT);

    // Erroneous tokens
    REQUIRE(lexTokenError("#") == TOKEN_ERROR);
  }

  SECTION("StringLiterals") {
    {
      TestSource  src("\"\"");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_STRING_LIT);
      REQUIRE(lex.tokenValue().length() == 0);
    }

    {
      TestSource  src("\"abc\\n\\r\\$\"");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_STRING_LIT);
      REQUIRE(lex.tokenValue() == "abc\n\r$");
    }

    {
      TestSource  src("\"\\x01\\xAA\\xBB\"");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_STRING_LIT);
      REQUIRE(lex.tokenValue() == "\x01\xAA\xBB");
    }

  #if 0
    {
      std::string expected("\x01\u00AA\u00BB");
      TestSource  src("\"\\x01\\uAA\\uBB\"");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_STRING_LIT);
      REQUIRE(lex.tokenValue() == expected);
    }

    {
      std::string expected("\u2105");
      TestSource  src("\"\\u2105\"");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_STRING_LIT);
      REQUIRE(lex.tokenValue() == expected);
    }

    {
      std::string expected("\U00012100");
      TestSource  src("\"\\U00012100\"");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_STRING_LIT);
      REQUIRE(lex.tokenValue() == expected);
    }
  #endif
  }

  SECTION("CharLiterals") {

    {
      TestSource  src("\'a\'");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_CHAR_LIT);
      REQUIRE(lex.tokenValue().length() == 1);
    }

    {
      std::string expected("\x01");
      TestSource  src("'\\x01'");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_CHAR_LIT);
      REQUIRE(lex.tokenValue() == expected);
    }

    {
      std::string expected("\xAA");
      TestSource  src("'\\xAA'");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_CHAR_LIT);
      REQUIRE(lex.tokenValue() == expected);
    }
  #if 0
    {
      std::string expected("000000aa");
      TestSource  src("'\\uAA'");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_CHAR_LIT);
      REQUIRE(lex.tokenValue() == expected);
    }

    {
      std::string expected("00002100");
      TestSource  src("'\\u2100\'");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_CHAR_LIT);
      REQUIRE(lex.tokenValue() == expected);
    }

    {
      std::string expected("00012100");
      TestSource  src("'\\U00012100'");
      Lexer       lex(&src);

      REQUIRE(lex.next() == TOKEN_CHAR_LIT);
      REQUIRE(lex.tokenValue() == expected);
    }
  #endif
  }

  SECTION("Comments") {
    REQUIRE(lexToken("/* comment */") == TOKEN_END);
    REQUIRE(lexToken(" /* comment */ ") == TOKEN_END);
    REQUIRE(lexToken("//") == TOKEN_END);
    REQUIRE(lexToken("// comment\n") == TOKEN_END);
    REQUIRE(lexToken(" //\n ") == TOKEN_END);
    REQUIRE(lexToken("  /* comment */10/* comment */ ") == TOKEN_DEC_INT_LIT);
    REQUIRE(lexToken("  /* comment */ 10 /* comment */ ") == TOKEN_DEC_INT_LIT);
    REQUIRE(lexToken("  /// comment\n 10 // comment\n ") == TOKEN_DEC_INT_LIT);
    REQUIRE(lexToken("  /// comment\n10// comment\n ") == TOKEN_DEC_INT_LIT);

    // Unterminated comment
    REQUIRE(lexTokenError("/* comment") == TOKEN_ERROR);
  }

  SECTION("Location") {
    TestSource  src("\n\n   aaaaa    ");
    Lexer       lex(&src);

    REQUIRE(lex.next() == TOKEN_ID);

    REQUIRE(lex.tokenLocation().startLine == 3u);
    REQUIRE(lex.tokenLocation().startCol == 4u);
    REQUIRE(lex.tokenLocation().endLine == 3u);
    REQUIRE(lex.tokenLocation().endCol == 9u);

    std::string line;
    REQUIRE(src.getLine(2, line));
    REQUIRE(line == "   aaaaa    ");
  }
}
