#include "Metadata.h"
#include "Parser.h"
#include "Tokenizer.h"
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
namespace fs = std::filesystem;

void print_prompt() { std::cout << "Anudb > "; }

int main() {
  std::string input_buffer;

  while (true) {
    print_prompt();

    if (!std::getline(std::cin, input_buffer)) {
      break; // Exit on EOF (Ctrl+D)
    }

    if (input_buffer == ".exit") {
      std::cout << "Bye!\n";
      break;
    }

    if (input_buffer.empty())
      continue;

    try {
      std::vector<Token> tokens = Tokenizer::tokenize(input_buffer);
      Parser parser(tokens);
      std::unique_ptr<Statement> statement = parser.parse();

      if (auto selectStmt = dynamic_cast<SelectStatement *>(statement.get())) {
        std::cout << "Parsed SELECT statement\n";
        std::cout << "  Table: " << selectStmt->table << "\n";
        std::cout << "  Columns: ";
        if (selectStmt->selectAll) {
          std::cout << "*";
        } else {
          for (const auto &col : selectStmt->columns)
            std::cout << col << " ";
        }
        std::cout << "\n";
      } else if (auto insertStmt =
                     dynamic_cast<InsertStatement *>(statement.get())) {
        std::cout << "Parsed INSERT statement\n";
        std::cout << "  Table: " << insertStmt->table << "\n";
        std::cout << "  Values: ";
        for (const auto &val : insertStmt->values)
          std::cout << val << " ";
        std::cout << "\n";
      } else if (auto createStmt =
                     dynamic_cast<CreateStatement *>(statement.get())) {
        std::cout << "Parsed CREATE statement\n";
        std::cout << "  Table: " << createStmt->table << "\n";
        std::cout << "  Columns: \n";
        for (const auto &col : createStmt->columns) {
          std::string typeName;
          if (col.type == ColumnType::INT)
            typeName = "INT";
          else if (col.type == ColumnType::TEXT)
            typeName = "TEXT";
          else if (col.type == ColumnType::FLOAT)
            typeName = "FLOAT";

          std::string constraints;
          if (col.isPrimary)
            constraints += " PRIMARY KEY";
          else if (col.isUnique)
            constraints += " UNIQUE";

          std::cout << "    " << col.name << " " << typeName << constraints
                    << "\n";
        }
      } else if (auto createDbStmt =
                     dynamic_cast<CreateDatabaseStatement *>(statement.get())) {
        const std::string &dbName = createDbStmt->databseName;
        std::string filename = dbName + ".anudb";
        bool validName = true;
        for (char c : dbName) {
          if (!std::isalnum(c) && c != '_') {
            validName = false;
            break;
          }
        }
        if (!validName) {
          std::cerr << "Error: Invalid database name '" << dbName
                    << "'. Use only letters, numbers, and underscores.\n";
        } else if (fs::exists(filename)) {
          std::cerr << "Error: Database '" << dbName << "' already exists.\n";
        } else {
          try {
            MetaDataHandler metadata(dbName);
            metadata.open();
            metadata.close();
            std::cout << "Database '" << dbName << "' created successfully. \n";
          } catch (const std::exception &e) {
            std::cerr << "Error creating databse : " << e.what() << "\n";
          }
        }
      }
    } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << "\n";
    }
  }
  return 0;
}
