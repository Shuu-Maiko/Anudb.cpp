#include <emscripten.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "Context.h"
#include "Tokenizer.h"
#include "Parser.h"
#include "Executor.h"

static DatabaseContext* globalContext = nullptr;

extern "C" {

EMSCRIPTEN_KEEPALIVE
void init_db() {
  if (globalContext) {
    delete globalContext;
  }
  globalContext = new DatabaseContext();
}

EMSCRIPTEN_KEEPALIVE
const char* execute_sql(const char* sql) {
  if (!globalContext) {
    globalContext = new DatabaseContext();
  }

  std::string input(sql);
  static std::string outputBuffer;
  outputBuffer.clear();

  std::stringstream redirectStream;
  std::streambuf* oldCout = std::cout.rdbuf(redirectStream.rdbuf());
  std::streambuf* oldCerr = std::cerr.rdbuf(redirectStream.rdbuf());

  try {
    std::vector<Token> tokens = Tokenizer::tokenize(input);
    if (!tokens.empty()) {
      Parser parser(tokens);
      std::unique_ptr<Statement> statement = parser.parse();
      if (statement) {
        Executor executor(*globalContext);
        executor.execute(statement.get());
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }

  std::cout.rdbuf(oldCout);
  std::cerr.rdbuf(oldCerr);

  outputBuffer = redirectStream.str();
  if (outputBuffer.empty()) {
    outputBuffer = "Command executed successfully.\n";
  }

  return outputBuffer.c_str();
}

}
