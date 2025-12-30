#include "Parser.h"
#include "Tokenizer.h"
#include <stdexcept>

Parser::Parser(const std::vector<Token> &tokens) : tokens(tokens) {}

const Token &Parser::peek() const {
  if (currentPos >= tokens.size()) {
    static Token eof = {TokenType::SYMBOL_SEMICOLON, "", 0};
    return eof;
  }
  return tokens[currentPos];
}

Token Parser::consume() { return tokens[currentPos++]; }

bool Parser::match(TokenType type) {
  if (peek().type == type) {
    currentPos++;
    return true;
  }
  return false;
}

Token Parser::expect(TokenType type, const std::string &errorMessage) {
  if (peek().type == type) {
    return consume();
  }
  throw std::runtime_error(errorMessage);
}

std::unique_ptr<Statement> Parser::parse() {
  if (match(TokenType::KEYWORD_SELECT)) {
    return parseSelect();
  }
  if (match(TokenType::KEYWORD_INSERT)) {
    return parseInsert();
  }
  if (match(TokenType::KEYWORD_CREATE)) {
    return parseCreate();
  }
  throw std::runtime_error("Unknown statement or syntax error");
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
  auto stmt = std::make_unique<SelectStatement>();

  // Parse columns
  if (match(TokenType::SYMBOL_ASTERISK)) {
    stmt->selectAll = true;
  } else {
    do {
      Token col = expect(TokenType::IDENTIFIER, "Expected column name");
      stmt->columns.push_back(col.text);
    } while (match(TokenType::SYMBOL_COMMA));
  }

  expect(TokenType::KEYWORD_FROM, "Expected FROM");
  Token table = expect(TokenType::IDENTIFIER, "Expected table name");
  stmt->table = table.text;
  match(TokenType::SYMBOL_SEMICOLON);

  return stmt;
}

std::unique_ptr<InsertStatement> Parser::parseInsert() {
  auto stmt = std::make_unique<InsertStatement>();

  expect(TokenType::KEYWORD_INTO, "Expected INTO");
  Token table = expect(TokenType::IDENTIFIER, "Expected table name");
  stmt->table = table.text;

  expect(TokenType::KEYWORD_VALUES, "Expected VALUES");
  expect(TokenType::SYMBOL_LPAREN, "Expected '('");

  do {

    Token val = peek();
    if (val.type == TokenType::STRING_LITERAL ||
        val.type == TokenType::INTEGER_LITERAL) {
      consume();
      stmt->values.push_back(val.text);
    } else {
      throw std::runtime_error("Expected a value (string or integer)");
    }
  } while (match(TokenType::SYMBOL_COMMA));

  expect(TokenType::SYMBOL_RPAREN, "Expected ')'");
  match(TokenType::SYMBOL_SEMICOLON); // Optional semicolon

  return stmt;
}

std::unique_ptr<CreateStatement> Parser::parseCreate() {
  auto stmt = std::make_unique<CreateStatement>();

  expect(TokenType::KEYWORD_TABLE, "Expected TABLE");
  Token table = expect(TokenType::IDENTIFIER, "Expected table name");
  stmt->table = table.text;

  expect(TokenType::SYMBOL_LPAREN, "Expected '('");

  do {
    Token col = expect(TokenType::IDENTIFIER, "Expected column name");
    stmt->columns.push_back(col.text);

    Token type = expect(TokenType::IDENTIFIER, "Expected column type");
    stmt->types.push_back(type.text);
  } while (match(TokenType::SYMBOL_COMMA));

  expect(TokenType::SYMBOL_RPAREN, "Expected ')'");
  match(TokenType::SYMBOL_SEMICOLON);

  return stmt;
}
