#ifndef JUCC_LEXER_LEXER_H
#define JUCC_LEXER_LEXER_H

#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "../symbol_table/symbol_table.h"

namespace jucc {
namespace lexer {

// Token types that can be extended at runtime
enum Token {
  // Special tokens
  TOK_EOF = -1,
  TOK_ERROR = -100,
  TOK_UNKNOWN = -101,

  // Data types
  TOK_INT = -2,
  TOK_FLOAT = -3,
  TOK_VOID = -4,

  // Keywords
  TOK_IF = -5,
  TOK_ELSE = -6,
  TOK_MAIN = -7,
  TOK_COUT = -8,
  TOK_CIN = -9,

  // Identifiers and literals
  TOK_IDENTIFIER = -10,
  TOK_DECIMAL = -11,
  TOK_FRACTIONAL = -12,
  TOK_LITERAL = -13,

  // Operators
  TOK_PLUS = -20,
  TOK_MINUS = -21,
  TOK_MULTIPLY = -22,
  TOK_DIVIDE = -23,
  TOK_MODULUS = -24,
  TOK_ASSIGNMENT = -25,
  TOK_EQUAL_TO = -26,
  TOK_NOT_EQUAL_TO = -27,
  TOK_LESS_THAN = -28,
  TOK_GREATER_THAN = -29,
  TOK_LESS_THAN_OR_EQUALS = -30,
  TOK_GREATER_THAN_OR_EQUALS = -31,
  TOK_LEFT_SHIFT = -32,
  TOK_RIGHT_SHIFT = -33,
  TOK_NOT = -34,

  // Delimiters
  TOK_SEMICOLON = -40,
  TOK_COMMA = -41,
  TOK_DOT = -42,
  TOK_PAREN_OPEN = -43,
  TOK_PAREN_CLOSE = -44,
  TOK_CURLY_OPEN = -45,
  TOK_CURLY_CLOSE = -46,
};

struct TokenInfo {
  std::string type;      // Token type name (for display and grammar matching)
  std::string value;     // Actual lexeme
  int line;             // Line number
  bool error;           // Error flag

  TokenInfo(std::string t, std::string v, int l, bool e = false)
      : type(std::move(t)), value(std::move(v)), line(l), error(e) {}
};

class Lexer {
private:
  std::string identifier_string_;
  std::string error_string_;
  std::string literal_string_;
  int intval_;
  double floatval_;
  int current_nesting_level_{0};
  std::vector<std::string> duplicate_symbol_errors_;
  std::vector<std::string> undeclared_symbol_errors_;
  std::string current_datatype_;
  symbol_table::SymbolTable symbol_table_;
  bool direct_before_datatype_{false};
  int current_line_{1};

  std::vector<TokenInfo> tokens_;  // for JSON output

 public:
  Lexer() = default;

  int GetToken(std::istream &is);
  std::string GetCurrentDatatype();
  static std::string GetTokenType(int token);
  int GetCurrentNestingLevel() const;
  std::vector<std::string> GetUndeclaredSymbolErrors();
  std::vector<std::string> GetDuplicateSymbolErrors();
  const bool &GetDirectBeforeDatatypeFlag() const { return direct_before_datatype_; }

  void DumpTokensAsJson() const;
  void AddToken(int token, const std::string &value, bool error = false);
};

}  // namespace lexer
}  // namespace jucc

#endif
