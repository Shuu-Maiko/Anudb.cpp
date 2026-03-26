#include "Context.h"
#include "Executor.h"
#include "Parser.h"
#include "Tokenizer.h"
#include "linenoise.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

std::string get_prompt(const DatabaseContext &ctx) {
  if (ctx.hasActiveDatabase())
    return "Anudb [" + ctx.activeDatabase + "]> ";
  else
    return "Anudb > ";
}

int main() {
  DatabaseContext ctx;

  std::cout << "Welcome to Anudb.cpp! Type SQL commands or '.exit' to quit.\n";

  linenoiseHistorySetMaxLen(100);

  char *line;
  while ((line = linenoise(get_prompt(ctx).c_str())) != NULL) {
    if (line[0] != '\0') {
      linenoiseHistoryAdd(line);
    }

    std::string input_buffer(line);
    free(line);

    if (input_buffer == ".exit") {
      std::cout << "Bye!\n";
      break;
    }
    if (input_buffer.empty())
      continue;

    try {
      std::vector<Token> tokens = Tokenizer::tokenize(input_buffer);
      if (tokens.empty())
        continue;

      Parser parser(tokens);
      std::unique_ptr<Statement> statement = parser.parse();

      if (statement) {
        Executor executor(ctx);
        executor.execute(statement.get());
      }
    } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << "\n";
    }
  }
  return 0;
}
