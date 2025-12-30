#include "Parser.h"
#include "Tokenizer.h"
#include <iostream>
#include <string>
#include <vector>

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
      } else if (auto insertStmt = dynamic_cast<InsertStatement *>(statement.get())) {
        std::cout << "Parsed INSERT statement\n";
        std::cout << "  Table: " << insertStmt->table << "\n";
        std::cout << "  Values: ";
        for (const auto &val : insertStmt->values)
          std::cout << val << " ";
        std::cout << "\n";
      } else if (auto createStmt = dynamic_cast<CreateStatement *>(statement.get())) {
        std::cout << "Parsed CREATE statement\n";
        std::cout << "  Table: " << createStmt->table << "\n";
        std::cout << "  Columns: \n";
        for (size_t i = 0; i < createStmt->columns.size(); ++i) {
          std::cout << "    " << createStmt->columns[i] << " " << createStmt->types[i] << "\n";
        }
      }

    } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << "\n";
    }
  }
  return 0;
}