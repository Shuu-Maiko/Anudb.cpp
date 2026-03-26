#pragma once
#include "Tokenizer.h"
#include <memory>
#include <string>
#include <vector>

struct Statement { // ALL statments base class
  virtual ~Statement() = default;
};

struct WhereClause {
  std::string column;
  std::string value;
  bool isNull = false;
};

struct SelectStatement : public Statement {
  std::string table;
  std::vector<std::string> columns;
  bool selectAll = false;
  WhereClause where;
  bool hasWhere = false;
};

struct InsertStatement : public Statement {
  std::string table;
  std::vector<std::string> values; //  storing values as strings for now
                                   //  TODO: upgrade INSERT struct
};

struct DeleteStatement : public Statement {
  std::string table;
  WhereClause where;
  bool hasWhere = false;
};

struct UpdateStatement : public Statement {
  std::string table;
  std::string column;
  std::string value;
  WhereClause where;
  bool hasWhere = false;
};

enum class ColumnType { INT, TEXT, FLOAT };

struct ColumnDefinition {
  std::string name;
  ColumnType type;
  bool isPrimary = false;
  bool isUnique = false;
};

struct CreateStatement : public Statement {
  std::string table;
  std::vector<ColumnDefinition> columns;
};

struct CreateDatabaseStatement : public Statement {
  std::string databseName;
};

struct ShowDatabasesStatement : public Statement {};

struct UseDatabaseStatement : public Statement {
  std::string databaseName;
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
  std::unique_ptr<UpdateStatement> parseUpdate();
  std::unique_ptr<DeleteStatement> parseDelete();
  std::unique_ptr<CreateStatement> parseCreate();
  std::unique_ptr<CreateDatabaseStatement> parseCreateDatabase();
  std::unique_ptr<ShowDatabasesStatement> parseShowDatabases();
  std::unique_ptr<UseDatabaseStatement> parseUseDatabase();

  WhereClause parseWhere();
};
