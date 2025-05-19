#ifndef JUCC_PARSER_LL_PARSER_H
#define JUCC_PARSER_LL_PARSER_H

#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include "parser/parsing_table.h"

namespace jucc::parser {

class LLParser {
public:
    LLParser() = default;

    // Initialize the parser with grammar and parsing table
    bool Initialize(std::ifstream& grammar_file, std::ifstream& table_file);

    // Parse the input tokens
    bool Parse(const std::vector<std::string>& input_tokens);

    // Write parsing trace to JSON file
    void DumpTraceAsJson(std::ofstream& out_file) const;
    
    // Generate and write parse tree to JSON file
    void DumpTreeAsJson(std::ofstream& out_file) const;

private:
    // Grammar components
    using Rule = std::vector<std::string>;
    using Rules = std::vector<Rule>;
    using Productions = std::unordered_map<std::string, Rules>;
    
    Productions productions_;
    std::string start_symbol_;
    std::vector<std::string> terminals_;
    std::vector<std::string> non_terminals_;

    // Parsing table
    using TableEntry = std::pair<int, int>;
    std::unordered_map<std::string, std::unordered_map<std::string, TableEntry>> parsing_table_;

    // Parsing trace for debugging/visualization
    struct TraceEntry {
        std::string stack_top;
        std::vector<std::string> full_stack;
        std::string current_input;
        std::string action;  // "match", "expand", "error", or production rule
    };
    std::vector<TraceEntry> parse_trace_;
    
    // Parse tree structure
    struct TreeNode {
        std::string symbol;
        std::vector<TreeNode> children;
    };
    TreeNode parse_tree_;
    
    // Helper functions
    void AddTraceEntry(const std::string& stack_top, const std::vector<std::string>& full_stack,
                     const std::string& current_input, const std::string& action);
    std::vector<std::string> GetStackContents(std::stack<std::string> stack_copy) const;
    bool IsTerminal(const std::string& symbol) const;
    bool IsNonTerminal(const std::string& symbol) const;
    std::string GetProductionString(const std::string& non_terminal, const std::vector<std::string>& rule) const;
    
    // Helper function to build the parse tree from the trace
    void BuildParseTree();
};

} // namespace jucc::parser

#endif // JUCC_PARSER_LL_PARSER_H 