#pragma once
#include <cstddef>
#include <string>
#include <vector>

enum class TokenType {
  KEYWORD_SELECT,  // SELECT
  KEYWORD_INSERT,  // INSERT
  KEYWORD_VALUES,  // VALUES
  KEYWORD_FROM,    // FROM
  KEYWORD_INTO,    // INTO
  KEYWORD_CREATE,  // CREATE
  KEYWORD_TABLE,   // TABLE
  KEYWORD_INT,     // INT
  KEYWORD_TEXT,    // TEXT
  KEYWORD_FLOAT,   // FLOAT
  KEYWORD_NULL,    // NULL
  IDENTIFIER,      // Table names, column names (e.g., "users")
  STRING_LITERAL,  // Values like "anu"
  INTEGER_LITERAL, // Values like 123
  FLOAT_LITERAL,   // Values like 12.34
  SYMBOL_ASTERISK, // *
  SYMBOL_COMMA,    // ,
  SYMBOL_SEMICOLON,// ;
  SYMBOL_LPAREN,   // (
  SYMBOL_RPAREN    // )
};

struct Token {
  TokenType type;
  std::string text;
  size_t cursor;
};

class Tokenizer {
public:
  static std::vector<Token> tokenize(const std::string &input);
  static void printToken(const Token &T);
};
