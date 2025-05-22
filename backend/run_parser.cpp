#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "parser/ll_parser.h"
#include "third_party/json.hpp"

int main() {
    try {
        // Read the parsing table
        std::ifstream table_file("test_parsing_table.json");
        if (!table_file.is_open()) {
            std::cerr << "Error: Could not open test_parsing_table.json\n";
            return 1;
        }

        // Read the input tokens
        std::ifstream input_file("test_input_tokens.json");
        if (!input_file.is_open()) {
            std::cerr << "Error: Could not open test_input_tokens.json\n";
    return 1;
  }

        // Read the grammar for production rules
        std::ifstream grammar_file("test_grammar.json");
        if (!grammar_file.is_open()) {
            std::cerr << "Error: Could not open test_grammar.json\n";
            return 1;
        }

        // Parse the input tokens from test_input_tokens.json
        std::vector<std::string> input_tokens;
        try {
            nlohmann::json tokens_json;
            input_file >> tokens_json;
            
            for (const auto& token : tokens_json) {
                input_tokens.push_back(token["type"].get<std::string>());
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing test_input_tokens.json: " << e.what() << "\n";
    return 1;
  }

        // Add end marker
        input_tokens.push_back("$");

        // Create and initialize the parser
        jucc::parser::LLParser parser;
        try {
            parser.Initialize(grammar_file, table_file);
        } catch (const std::exception& e) {
            std::cerr << "Error initializing parser: " << e.what() << "\n";
            return 1;
        }
        
        // Parse the input
        bool success = parser.Parse(input_tokens);

        // Write the parsing trace to JSON
        std::ofstream trace_file("parse_trace.json");
        if (trace_file.is_open()) {
            parser.DumpTraceAsJson(trace_file);
            trace_file.close();
  }

        if (success) {
            std::cout << "✅ Input successfully parsed!\n";
  return 0;
        } else {
            std::cout << "❌ Parsing failed. Check parse_trace.json for details.\n";
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
