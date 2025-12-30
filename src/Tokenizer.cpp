#include "Tokenizer.h"
#include <cctype>
#include <iostream>

bool is_symbol(char c) {
  return c == '(' || c == ')' || c == ',' || c == '*' || c == ';';
}

std::vector<Token> Tokenizer::tokenize(const std::string &input) {
  std::vector<Token> tokens;
  size_t i = 0;

  while (i < input.length()) {
    char c = input[i];

    if (std::isspace(c)) {
      i++;
      continue;
    }

    Token token;
    token.cursor = i;

    if (is_symbol(c)) {
      token.text = std::string(1, c);
      if (c == '*')
        token.type = TokenType::SYMBOL_ASTERISK;
      else if (c == ',')
        token.type = TokenType::SYMBOL_COMMA;
      else if (c == ';')
        token.type = TokenType::SYMBOL_SEMICOLON;
      else if (c == '(')
        token.type = TokenType::SYMBOL_LPAREN;
      else if (c == ')')
        token.type = TokenType::SYMBOL_RPAREN;
      tokens.push_back(token);
      i++;
      continue;
    }
    if (input[i] == '\'') {
      size_t start = ++i;
      while (i < input.length() && !std::isspace(input[i]) &&
             !is_symbol(input[i]) && input[i] != '\'') {
        i++;
      }

      if (i < input.length() && input[i] == '\'') {
        std::string word = input.substr(start, i - start);
        token.text = word;
        token.type = TokenType::STRING_LITERAL;
        tokens.push_back(token);
        i++;
        continue;
      }
    }
    size_t start = i;
    while (i < input.length() && !std::isspace(input[i]) &&
           !is_symbol(input[i])) {
      i++;
    }
    std::string word = input.substr(start, i - start);
    token.text = word;

    if (word == "select" || word == "SELECT")
      token.type = TokenType::KEYWORD_SELECT;
    else if (word == "insert" || word == "INSERT")
      token.type = TokenType::KEYWORD_INSERT;
    else if (word == "values" || word == "VALUES")
      token.type = TokenType::KEYWORD_VALUES;
    else if (word == "from" || word == "FROM")
      token.type = TokenType::KEYWORD_FROM;
    else if (word == "into" || word == "INTO")
      token.type = TokenType::KEYWORD_INTO;
    else if (word == "create" || word == "CREATE")
      token.type = TokenType::KEYWORD_CREATE;
    else if (word == "table" || word == "TABLE")
      token.type = TokenType::KEYWORD_TABLE;
    else if (word == "int" || word == "INT")
      token.type = TokenType::KEYWORD_INT;
    else if (word == "text" || word == "TEXT")
      token.type = TokenType::KEYWORD_TEXT;
    else if (word == "float" || word == "FLOAT")
      token.type = TokenType::KEYWORD_FLOAT;
    else if (word == "null" || word == "NULL")
      token.type = TokenType::KEYWORD_NULL;
    else if (isdigit(word[0])) {
      if (word.find('.') != std::string::npos)
        token.type = TokenType::FLOAT_LITERAL;
      else
        token.type = TokenType::INTEGER_LITERAL;
    } else
      token.type = TokenType::IDENTIFIER;

    tokens.push_back(token);
  }

  return tokens;
}

void Tokenizer::printToken(const Token &T) {
  std::string sttype = "UNKNOWN TYPE";

  switch (T.type) {
  case TokenType::KEYWORD_SELECT:
    sttype = "KEYWORD_SELECT";
    break;
  case TokenType::KEYWORD_INSERT:
    sttype = "KEYWORD_INSERT";
    break;
  case TokenType::KEYWORD_VALUES:
    sttype = "KEYWORD_VALUES";
    break;
  case TokenType::KEYWORD_FROM:
    sttype = "KEYWORD_FROM";
    break;
  case TokenType::KEYWORD_INTO:
    sttype = "KEYWORD_INTO";
    break;
  case TokenType::KEYWORD_CREATE:
    sttype = "KEYWORD_CREATE";
    break;
  case TokenType::KEYWORD_TABLE:
    sttype = "KEYWORD_TABLE";
    break;
  case TokenType::KEYWORD_INT:
    sttype = "KEYWORD_INT";
    break;
  case TokenType::KEYWORD_TEXT:
    sttype = "KEYWORD_TEXT";
    break;
  case TokenType::KEYWORD_FLOAT:
    sttype = "KEYWORD_FLOAT";
    break;
  case TokenType::KEYWORD_NULL:
    sttype = "KEYWORD_NULL";
    break;
  case TokenType::IDENTIFIER:
    sttype = "IDENTIFIER";
    break;
  case TokenType::STRING_LITERAL:
    sttype = "STRING_LITERAL";
    break;
  case TokenType::INTEGER_LITERAL:
    sttype = "INTEGER_LITERAL";
    break;
  case TokenType::FLOAT_LITERAL:
    sttype = "FLOAT_LITERAL";
    break;
  case TokenType::SYMBOL_ASTERISK:
    sttype = "SYMBOL_ASTERISK";
    break;
  case TokenType::SYMBOL_COMMA:
    sttype = "SYMBOL_COMMA";
    break;
  case TokenType::SYMBOL_SEMICOLON:
    sttype = "SYMBOL_SEMICOLON";
    break;
  case TokenType::SYMBOL_LPAREN:
    sttype = "SYMBOL_LPAREN";
    break;
  case TokenType::SYMBOL_RPAREN:
    sttype = "SYMBOL_RPAREN";
    break;
  }

  std::cout << sttype << ": " << T.text << " at " << T.cursor << std::endl;
}
