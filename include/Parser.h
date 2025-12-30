#pragma once
#include "Tokenizer.h"
#include <memory>
#include <string>
#include <vector>

struct Statement { // ALL statments base class
  virtual ~Statement() = default;
};

struct SelectStatement : public Statement {
  std::string table;
  std::vector<std::string> columns;
  bool selectAll = false;
};

struct InsertStatement : public Statement {
  std::string table;
  std::vector<std::string>
      values; // Simplifying: storing values as strings for now
              //  TODO: upgrade INSERT struct
};

struct CreateStatement : public Statement {
  std::string table;
  std::vector<std::string> columns;
  std::vector<std::string> types; // FOR NOW ITS STRING TODO: add types
};

class Parser {
public:
  explicit Parser(const std::vector<Token> &tokens);
  std::unique_ptr<Statement> parse();

private:
  const std::vector<Token> &tokens;
  size_t currentPos = 0;

  const Token &peek() const;
  Token consume();
  bool match(TokenType type);
  Token expect(TokenType type, const std::string &errorMessage);

  std::unique_ptr<SelectStatement> parseSelect();
  std::unique_ptr<InsertStatement> parseInsert();
  std::unique_ptr<CreateStatement> parseCreate();
};
