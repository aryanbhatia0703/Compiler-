#ifndef JUCC_PARSER_PARSER_H
#define JUCC_PARSER_PARSER_H

#include <stack>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

namespace jucc {
namespace parser {

class Parser {
private:
    std::vector<std::string> productions_;
    std::vector<std::string> terminals_;
    std::vector<std::string> non_terminals_;
    std::unordered_map<std::string, std::vector<std::string>> firsts_;
    std::unordered_map<std::string, std::vector<std::string>> follows_;
    std::vector<std::string> input_tokens_;
    std::stack<std::string> stack_;
    std::vector<std::string> input_string_;
    std::string start_symbol_;
    int current_step_{0};
    std::vector<int> production_history_;
    std::vector<std::string> current_string_;
    std::vector<std::string> parser_errors_;
    std::vector<std::string> parse_trace_;

public:
    Parser() = default;
    void ParseNextStep();
    void ResetParsing();
    bool IsComplete();
    void DoNextStep();
    void BuildParseTree();
    bool WriteParseTree(const std::string &filepath);
    void SetInputString(std::vector<std::string> inps);
    void SetStartSymbol(std::string start);
    const std::vector<int> &GetProductionHistory() { return production_history_; }
    const std::vector<std::string> &GetParserErrors() { return parser_errors_; }
    const std::vector<std::string> &GetParseTrace() const { return parse_trace_; }
};

}  // namespace parser
  std::vector<grammar::Production> productions_;
  std::vector<std::string> terminals_;
  std::vector<std::string> non_terminals_;
  std::unordered_map<std::string, std::vector<std::string>> firsts_;
  std::unordered_map<std::string, std::vector<std::string>> follows_;
  std::vector<std::string> input_tokens_;
  /**
   *  json pretty print indentation for generated parse tree
   */
  static const int INDENTATION = 4;

  /**
   * parse tree for Treant.js integration
   */
  json parse_tree_;

  /**
   * A stack to put the symbols and perform the actual parsing
   */
  std::stack<std::string> stack_;

  /**
   * The given input string to parse.
   */
  std::vector<std::string> input_string_;

  /**
   * The start symbol for the grammar
   */
  std::string start_symbol_;

  /**
   * Holds the current step of parsing.
   */
  int current_step_{0};

  /**
   * Holds the build up parsing table object
   */
  ParsingTable table_;

  /**
   * Holds the history of the productions parsed during parsing
   */
  std::vector<int> production_history_;

  /**
   * Holds a copy of the input string initially
   * and changes with each step of parsing.
   */
  std::vector<std::string> current_string_;

  /**
   * Errors incurred during the parsing of the given input file.
   */
  std::vector<std::string> parser_errors_;

  /**
   * Helper function to generate error messages for parsing.
   */
  static std::string GenerateErrorMessage(const std::string &current_token);

  /**
   * Supportive function for Parser::FormattedJSON
   * @param value
   * @return { text: { name: "value" } }
   */
  static json GetTextNode(const std::string & /* value */);

  /**
   * Utility recursive function for Parser::FormattedJSON
   * @param body, a json
   * @returns Treant.js formatted JSON
   */
  static json RecRunner(const json & /*main*/, std::string /* key */);
 std::vector<std::string> parse_trace_;
 public:
  /**
   * Constructor for initializing stack and other members.
   */
  Parser();

  /**
   * Used for parsing the next token of the input string
   */
  void ParseNextStep();

  /**
   * Resets the entire parsing process
   */
  void ResetParsing();

  /**
   * Function that returns true when the parsing is completed
   */
  bool IsComplete();

  /**
   * Completes a step of parsing
   */
  void DoNextStep();

  /**
   * Build the parse tree from production history
   * Parse tree not built if parser in error state
   */
  void BuildParseTree();

  /**
   * Dumps the parse tree in given path in json format
   * @param filepath
   * @param formatted (Default is in Treant.js format else raw if value is set "false")
   * @returns true on success
   */
  bool WriteParseTree(const std::string &filepath, bool formatted = true);

  /**
   * Takes a json with no array, ideally received from parser::GetParseTree()
   * Format is given here https://fperucic.github.io/treant-js/
   * @returns a formatted JSON which acts as a input for Treant.js
   */
  [[maybe_unused]] [[nodiscard]] static json FormattedJSON(const json & /* body */);

  /* getters and setters*/
  void SetInputString(std::vector<std::string> inps);
  void SetParsingTable(ParsingTable table);
  void SetStartSymbol(std::string start);
  [[nodiscard]] const std::vector<int> &GetProductionHistory() { return production_history_; }
  [[nodiscard]] const std::vector<std::string> &GetParserErrors() { return parser_errors_; }
  [[nodiscard]] const json &GetParseTree() { return parse_tree_; }
  bool LoadFromJson(const json &grammar,
const json &first_follow,
const json &table,
const json &tokens);


bool SimulateParsing();
inline const std::vector<std::string> &GetParseTrace() const { return parse_trace_; }
};

}  // namespace parser

}  // namespace jucc

#endif
