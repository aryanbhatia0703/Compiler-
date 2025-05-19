#include "lexer.h"
#include <iostream>

namespace jucc::lexer {

void Lexer::AddToken(int token, const std::string &value, bool error) {
  tokens_.emplace_back(GetTokenType(token), value, current_line_, error);
}

int Lexer::GetToken(std::istream &is) {
  static char last_char = ' ';

  while (!is.eof() && (isspace(last_char) != 0)) {
    if (last_char == '\n') current_line_++;
    is.get(last_char);
  }

  if (is.eof()) return TOK_EOF;

  // Handle comments
  if (last_char == '/') {
    is.get(last_char);
    if (last_char == '/') {  // Single-line comment
      while (!is.eof() && last_char != '\n') {
        is.get(last_char);
      }
      if (last_char == '\n') current_line_++;
      is.get(last_char);
      return GetToken(is);  // Get next token
    } else if (last_char == '*') {  // Multi-line comment
      bool comment_ended = false;
      while (!is.eof() && !comment_ended) {
        is.get(last_char);
        if (last_char == '\n') current_line_++;
        if (last_char == '*') {
          is.get(last_char);
          if (last_char == '/') {
            comment_ended = true;
          }
        }
      }
      is.get(last_char);
      return GetToken(is);  // Get next token
    } else {
      // It's a division operator
      AddToken(TOK_DIVIDE, "/");
      return TOK_DIVIDE;
    }
  }

  if (isalpha(last_char) || last_char == '_') {
    identifier_string_ = last_char;
    int ret_token = TOK_IDENTIFIER;
    while (is.get(last_char) && (isalnum(last_char) || last_char == '_')) {
      identifier_string_ += last_char;
    }

    if (identifier_string_ == "int") {
      current_datatype_ = identifier_string_;
      direct_before_datatype_ = true;
      ret_token = TOK_INT;
    } else if (identifier_string_ == "float") {
      current_datatype_ = identifier_string_;
      direct_before_datatype_ = true;
      ret_token = TOK_FLOAT;
    } else if (identifier_string_ == "void") {
      current_datatype_ = identifier_string_;
      direct_before_datatype_ = true;
      ret_token = TOK_VOID;
    } else if (identifier_string_ == "if") {
      current_datatype_ = "";
      direct_before_datatype_ = false;
      ret_token = TOK_IF;
    } else if (identifier_string_ == "else") {
      current_datatype_ = "";
      direct_before_datatype_ = false;
      ret_token = TOK_ELSE;
    } else if (identifier_string_ == "cout") {
      current_datatype_ = "";
      direct_before_datatype_ = false;
      ret_token = TOK_COUT;
    } else if (identifier_string_ == "cin") {
      current_datatype_ = "";
      direct_before_datatype_ = false;
      ret_token = TOK_CIN;
    } else if (identifier_string_ == "main") {
      current_datatype_ = "";
      direct_before_datatype_ = false;
      ret_token = TOK_MAIN;
    } else {
      ret_token = TOK_IDENTIFIER;
      auto *node = new symbol_table::Node(identifier_string_, current_datatype_, current_nesting_level_);
      symbol_table_.CheckAndAddEntry(node, direct_before_datatype_);
      delete node;
      if (!symbol_table_.GetDuplicateSymbols().empty()) {
        duplicate_symbol_errors_ = symbol_table_.GetDuplicateSymbols();
      }
      if (!symbol_table_.GetUndeclaredSymbols().empty()) {
        undeclared_symbol_errors_ = symbol_table_.GetUndeclaredSymbols();
      }
    }
    AddToken(ret_token, identifier_string_);
    return ret_token;
  }

  if (isdigit(last_char) || last_char == '.') {
    std::string num_string;
    direct_before_datatype_ = false;
    bool has_dot = (last_char == '.');
    if (has_dot) {
      num_string = "0.";
      is.get(last_char);
    } else {
      num_string = last_char;
    }
    
    while (is.get(last_char) && (isdigit(last_char) || (!has_dot && last_char == '.'))) {
      if (last_char == '.') has_dot = true;
      num_string += last_char;
    }

    if (isalpha(last_char) || last_char == '_') {
      while (!is.eof() && (isalnum(last_char) || last_char == '_')) {
        num_string += last_char;
        is.get(last_char);
      }
      error_string_ = "Invalid number format: " + num_string;
      AddToken(TOK_ERROR, num_string, true);
      return TOK_ERROR;
    }

    int ret_token;
    if (has_dot) {
      floatval_ = strtod(num_string.c_str(), nullptr);
      ret_token = TOK_FRACTIONAL;
    } else {
      intval_ = (int)strtod(num_string.c_str(), nullptr);
      ret_token = TOK_DECIMAL;
    }
    AddToken(ret_token, num_string);
    return ret_token;
  }

  if (last_char == '"') {
    literal_string_ = "";
    while (is.get(last_char) && last_char != '"') {
      if (last_char == '\\') {
        is.get(last_char);
        switch (last_char) {
          case 'n': literal_string_ += '\n'; break;
          case 't': literal_string_ += '\t'; break;
          case 'r': literal_string_ += '\r'; break;
          case '"': literal_string_ += '"'; break;
          case '\\': literal_string_ += '\\'; break;
          default: literal_string_ += last_char;
        }
      } else {
        literal_string_ += last_char;
      }
    }
    if (last_char != '"') {
      error_string_ = "Unterminated string literal";
      AddToken(TOK_ERROR, literal_string_, true);
      return TOK_ERROR;
    }
    is.get(last_char);
    AddToken(TOK_LITERAL, literal_string_);
    return TOK_LITERAL;
  }

  if (ispunct(last_char)) {
    int ret_token = TOK_ERROR;
    char ch = last_char;
    std::string val(1, ch);

    switch (ch) {
      case ';': ret_token = TOK_SEMICOLON; break;
      case '+': ret_token = TOK_PLUS; break;
      case '-': ret_token = TOK_MINUS; break;
      case '*': ret_token = TOK_MULTIPLY; break;
      case '%': ret_token = TOK_MODULUS; break;
      case '(': ret_token = TOK_PAREN_OPEN; break;
      case ')': ret_token = TOK_PAREN_CLOSE; break;
      case '{': ret_token = TOK_CURLY_OPEN; current_nesting_level_++; break;
      case '}': ret_token = TOK_CURLY_CLOSE; symbol_table_.RemoveNodesOnScopeEnd(current_nesting_level_); current_nesting_level_--; break;
      case '<':
        is.get(last_char);
        if (last_char == '=') {
          ret_token = TOK_LESS_THAN_OR_EQUALS;
          val = "<=";
        } else if (last_char == '<') {
          ret_token = TOK_LEFT_SHIFT;
          val = "<<";
        } else {
          ret_token = TOK_LESS_THAN;
          is.unget();
        }
        break;
      case '>':
        is.get(last_char);
        if (last_char == '=') {
          ret_token = TOK_GREATER_THAN_OR_EQUALS;
          val = ">=";
        } else if (last_char == '>') {
          ret_token = TOK_RIGHT_SHIFT;
          val = ">>";
        } else {
          ret_token = TOK_GREATER_THAN;
          is.unget();
        }
        break;
      case '=':
        is.get(last_char);
        if (last_char == '=') {
          ret_token = TOK_EQUAL_TO;
          val = "==";
        } else {
          ret_token = TOK_ASSIGNMENT;
          is.unget();
        }
        break;
      case '!':
        is.get(last_char);
        if (last_char == '=') {
          ret_token = TOK_NOT_EQUAL_TO;
          val = "!=";
        } else {
          ret_token = TOK_NOT;
          is.unget();
        }
        break;
    }

    if (ret_token != TOK_ERROR) {
      is.get(last_char);
      AddToken(ret_token, val);
      return ret_token;
    }
  }

  // Unknown character
  std::string unknown(1, last_char);
  error_string_ = "Unknown character: " + unknown;
  AddToken(TOK_ERROR, unknown, true);
  is.get(last_char);
  return TOK_ERROR;
}

std::string Lexer::GetTokenType(int token) {
  switch (token) {
    case TOK_INT: return "int";
    case TOK_FLOAT: return "float";
    case TOK_VOID: return "void";
    case TOK_IF: return "if";
    case TOK_ELSE: return "else";
    case TOK_IDENTIFIER: return "identifier";
    case TOK_DECIMAL: return "integer constant";
    case TOK_FRACTIONAL: return "float constant";
    case TOK_SEMICOLON: return ";";
    case TOK_PLUS: return "+";
    case TOK_MINUS: return "-";
    case TOK_MULTIPLY: return "*";
    case TOK_DIVIDE: return "/";
    case TOK_MODULUS: return "%";
    case TOK_ASSIGNMENT: return "=";
    case TOK_EQUAL_TO: return "==";
    case TOK_NOT_EQUAL_TO: return "!=";
    case TOK_LESS_THAN: return "<";
    case TOK_GREATER_THAN: return ">";
    case TOK_LESS_THAN_OR_EQUALS: return "<=";
    case TOK_GREATER_THAN_OR_EQUALS: return ">=";
    case TOK_LEFT_SHIFT: return "<<";
    case TOK_RIGHT_SHIFT: return ">>";
    case TOK_NOT: return "!";
    case TOK_PAREN_OPEN: return "(";
    case TOK_PAREN_CLOSE: return ")";
    case TOK_CURLY_OPEN: return "{";
    case TOK_CURLY_CLOSE: return "}";
    case TOK_COUT: return "cout";
    case TOK_CIN: return "cin";
    case TOK_MAIN: return "main";
    case TOK_LITERAL: return "string literal";
    case TOK_ERROR: return "error";
    default: return "unknown";
  }
}

void Lexer::DumpTokensAsJson() const {
    std::cout << "[\n";
    for (size_t i = 0; i < tokens_.size(); ++i) {
        const auto& token = tokens_[i];
        std::cout << "  {\n";
        std::cout << "    \"type\": \"" << token.type << "\",\n";
        std::cout << "    \"value\": \"" << token.value << "\",\n";
        std::cout << "    \"line\": " << token.line << ",\n";
        std::cout << "    \"error\": " << (token.error ? "true" : "false") << "\n";
        std::cout << "  }" << (i < tokens_.size() - 1 ? "," : "") << "\n";
    }
    std::cout << "]\n";
}

std::string Lexer::GetCurrentDatatype() { return current_datatype_; }
std::vector<std::string> Lexer::GetDuplicateSymbolErrors() { return duplicate_symbol_errors_; }
std::vector<std::string> Lexer::GetUndeclaredSymbolErrors() { return undeclared_symbol_errors_; }
int Lexer::GetCurrentNestingLevel() const { return current_nesting_level_; }

}  // namespace jucc::lexer
