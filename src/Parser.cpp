#include "Parser.h"
#include "Tokenizer.h"
#include <memory>
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
  if (match(TokenType::KEYWORD_UPDATE)) {
    return parseUpdate();
  }
  if (match(TokenType::KEYWORD_DELETE)) {
    return parseDelete();
  }
  if (match(TokenType::KEYWORD_CREATE)) {
    if (match(TokenType::KEYWORD_DATABASE)) {
      return parseCreateDatabase();
    }
    return parseCreate();
  }
  if (match(TokenType::KEYWORD_SHOW)) {
    return parseShowDatabases();
  }
  if (match(TokenType::KEYWORD_USE)) {
    return parseUseDatabase();
  }
  throw std::runtime_error("Unknown statement or syntax error");
}

std::unique_ptr<SelectStatement> Parser::parseSelect() {
  auto stmt = std::make_unique<SelectStatement>();

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

  if (match(TokenType::KEYWORD_WHERE)) {
    stmt->where = parseWhere();
    stmt->hasWhere = true;
  }

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
        val.type == TokenType::INTEGER_LITERAL ||
        val.type == TokenType::FLOAT_LITERAL ||
        val.type == TokenType::KEYWORD_NULL) {
      consume();
      stmt->values.push_back(val.text);
    } else {
      throw std::runtime_error(
          "Expected a value (string, integer, float, or null)");
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

    ColumnType colType;
    if (match(TokenType::KEYWORD_INT)) {
      colType = ColumnType::INT;
    } else if (match(TokenType::KEYWORD_TEXT)) {
      colType = ColumnType::TEXT;
    } else if (match(TokenType::KEYWORD_FLOAT)) {
      colType = ColumnType::FLOAT;
    } else {
      throw std::runtime_error("Expected column type (INT, TEXT, or FLOAT)");
    }

    ColumnDefinition colDef;
    colDef.name = col.text;
    colDef.type = colType;
    colDef.isPrimary = false;
    colDef.isUnique = false;

    while (peek().type == TokenType::KEYWORD_PRIMARY ||
           peek().type == TokenType::KEYWORD_UNIQUE) {
      if (match(TokenType::KEYWORD_PRIMARY)) {
        expect(TokenType::KEYWORD_KEY, "Expected KEY after PRIMARY");
        colDef.isPrimary = true;
        colDef.isUnique = true;
      } else if (match(TokenType::KEYWORD_UNIQUE)) {
        colDef.isUnique = true;
      }
    }

    stmt->columns.push_back(colDef);
  } while (match(TokenType::SYMBOL_COMMA));

  expect(TokenType::SYMBOL_RPAREN, "Expected ')'");
  match(TokenType::SYMBOL_SEMICOLON);

  return stmt;
}

std::unique_ptr<CreateDatabaseStatement> Parser::parseCreateDatabase() {
  auto stmt = std::make_unique<CreateDatabaseStatement>();
  Token dbName = expect(TokenType::IDENTIFIER, "Expected a Database Name");
  stmt->databseName = dbName.text;
  match(TokenType::SYMBOL_SEMICOLON);
  return stmt;
}

std::unique_ptr<ShowDatabasesStatement> Parser::parseShowDatabases() {
  auto stmt = std::make_unique<ShowDatabasesStatement>();
  expect(TokenType::KEYWORD_DATABASES, "Expected DATABASES after SHOW ");
  match(TokenType::SYMBOL_SEMICOLON);
  return stmt;
}

std::unique_ptr<UseDatabaseStatement> Parser::parseUseDatabase() {
  auto stmt = std::make_unique<UseDatabaseStatement>();

  Token dbName =
      expect(TokenType::IDENTIFIER, "Expected database name after USE");
  stmt->databaseName = dbName.text;
  match(TokenType::SYMBOL_SEMICOLON);

  return stmt;
}

std::unique_ptr<UpdateStatement> Parser::parseUpdate() {
  auto stmt = std::make_unique<UpdateStatement>();

  Token table = expect(TokenType::IDENTIFIER, "Expected table name");
  stmt->table = table.text;

  expect(TokenType::KEYWORD_SET, "Expected SET");
  Token col = expect(TokenType::IDENTIFIER, "Expected column name");
  stmt->column = col.text;

  expect(TokenType::SYMBOL_EQUALS, "Expected '='");

  Token val = consume();
  if (val.type != TokenType::STRING_LITERAL &&
      val.type != TokenType::INTEGER_LITERAL &&
      val.type != TokenType::FLOAT_LITERAL) {
    throw std::runtime_error("Expected a literal value in SET");
  }
  stmt->value = val.text;

  if (match(TokenType::KEYWORD_WHERE)) {
    stmt->where = parseWhere();
    stmt->hasWhere = true;
  }

  match(TokenType::SYMBOL_SEMICOLON);
  return stmt;
}

std::unique_ptr<DeleteStatement> Parser::parseDelete() {
  auto stmt = std::make_unique<DeleteStatement>();

  expect(TokenType::KEYWORD_FROM, "Expected FROM");
  Token table = expect(TokenType::IDENTIFIER, "Expected table name");
  stmt->table = table.text;

  if (match(TokenType::KEYWORD_WHERE)) {
    stmt->where = parseWhere();
    stmt->hasWhere = true;
  }

  match(TokenType::SYMBOL_SEMICOLON);
  return stmt;
}

WhereClause Parser::parseWhere() {
  WhereClause where;
  Token col = expect(TokenType::IDENTIFIER, "Expected column name in WHERE");
  where.column = col.text;

  expect(TokenType::SYMBOL_EQUALS, "Expected '=' in WHERE");

  Token val = consume();
  if (val.type == TokenType::KEYWORD_NULL) {
    where.isNull = true;
  } else if (val.type == TokenType::STRING_LITERAL ||
             val.type == TokenType::INTEGER_LITERAL ||
             val.type == TokenType::FLOAT_LITERAL) {
    where.value = val.text;
  } else {
    throw std::runtime_error("Expected a value or NULL in WHERE");
  }

  return where;
}
