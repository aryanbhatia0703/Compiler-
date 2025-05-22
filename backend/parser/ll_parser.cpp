#include "../include/parser/ll_parser.h"
#include <sstream>
#include <iostream>

namespace jucc::parser {

bool LLParser::Initialize(std::ifstream& grammar_file, std::ifstream& table_file) {
    try {
        // Read and parse the grammar file
        std::string json_str;
        std::string line;
        while (std::getline(grammar_file, line)) {
            json_str += line + "\n";
        }

        // Parse terminals
        size_t terms_start = json_str.find("\"terminals\"");
        if (terms_start != std::string::npos) {
            terms_start = json_str.find("[", terms_start);
            size_t terms_end = json_str.find("]", terms_start);
            std::string terms_str = json_str.substr(terms_start, terms_end - terms_start + 1);
            
            size_t pos = 0;
            while ((pos = terms_str.find("\"", pos)) != std::string::npos) {
                pos++;
                size_t end_pos = terms_str.find("\"", pos);
                if (end_pos == std::string::npos) break;
                terminals_.push_back(terms_str.substr(pos, end_pos - pos));
                pos = end_pos + 1;
            }
        }

        // Parse non-terminals
        size_t nonterms_start = json_str.find("\"non_terminals\"");
        if (nonterms_start != std::string::npos) {
            nonterms_start = json_str.find("[", nonterms_start);
            size_t nonterms_end = json_str.find("]", nonterms_start);
            std::string nonterms_str = json_str.substr(nonterms_start, nonterms_end - nonterms_start + 1);
            
            size_t pos = 0;
            while ((pos = nonterms_str.find("\"", pos)) != std::string::npos) {
                pos++;
                size_t end_pos = nonterms_str.find("\"", pos);
                if (end_pos == std::string::npos) break;
                non_terminals_.push_back(nonterms_str.substr(pos, end_pos - pos));
                pos = end_pos + 1;
            }
        }

        // Parse start symbol
        size_t start_pos = json_str.find("\"start_symbol\"");
        if (start_pos != std::string::npos) {
            start_pos = json_str.find("\"", start_pos + 14) + 1;
            size_t end_pos = json_str.find("\"", start_pos);
            start_symbol_ = json_str.substr(start_pos, end_pos - start_pos);
        }

        // Parse productions
        size_t prod_start = json_str.find("\"productions\"");
        if (prod_start != std::string::npos) {
            prod_start = json_str.find("[", prod_start);
            size_t prod_end = json_str.find_last_of("]");
            std::string prods_str = json_str.substr(prod_start, prod_end - prod_start + 1);

            size_t pos = 0;
            while ((pos = prods_str.find("\"parent\"", pos)) != std::string::npos) {
                pos = prods_str.find("\"", pos + 8) + 1;
                size_t end_pos = prods_str.find("\"", pos);
                std::string parent = prods_str.substr(pos, end_pos - pos);

                pos = prods_str.find("\"rules\"", pos);
                pos = prods_str.find("[", pos);
                end_pos = prods_str.find("]", pos);
                while (prods_str[end_pos + 1] == ',') {
                    end_pos = prods_str.find("]", end_pos + 1);
                }
                std::string rules_str = prods_str.substr(pos, end_pos - pos + 1);

                std::vector<grammar::Rule> rules;
                size_t rule_pos = 0;
                while ((rule_pos = rules_str.find("[", rule_pos)) != std::string::npos) {
                    std::vector<std::string> symbols;
                    size_t symbol_start = rule_pos;
                    while ((symbol_start = rules_str.find("\"", symbol_start + 1)) != std::string::npos) {
                        size_t symbol_end = rules_str.find("\"", symbol_start + 1);
                        std::string symbol = rules_str.substr(symbol_start + 1, symbol_end - symbol_start - 1);
                        symbols.push_back(symbol);
                        symbol_start = symbol_end;
                        if (rules_str[symbol_end + 1] == ']') break;
                    }
                    rules.emplace_back(symbols);
                    rule_pos = rules_str.find("]", rule_pos) + 1;
                }

                productions_.emplace_back(parent, rules);
                pos = end_pos;
            }
        }

        // Read and parse the parsing table
        json_str.clear();
        while (std::getline(table_file, line)) {
            json_str += line + "\n";
        }

        // Parse the table entries
        size_t pos = 0;
        while ((pos = json_str.find("\"", pos)) != std::string::npos) {
            pos++;
            size_t end_pos = json_str.find("\"", pos);
            if (end_pos == std::string::npos) break;
            std::string nt = json_str.substr(pos, end_pos - pos);
            
            pos = json_str.find("{", end_pos);
            if (pos == std::string::npos) break;
            size_t table_end = json_str.find("}", pos);
            std::string table_str = json_str.substr(pos, table_end - pos + 1);
            
            size_t entry_pos = 0;
            while ((entry_pos = table_str.find("\"", entry_pos)) != std::string::npos) {
                entry_pos++;
                size_t entry_end = table_str.find("\"", entry_pos);
                if (entry_end == std::string::npos) break;
                std::string terminal = table_str.substr(entry_pos, entry_end - entry_pos);
                
                entry_pos = table_str.find("\"", entry_end + 1) + 1;
                entry_end = table_str.find("\"", entry_pos);
                if (entry_end == std::string::npos) break;
                std::string value = table_str.substr(entry_pos, entry_end - entry_pos);
                
                parsing_table_[nt][terminal] = value;
                entry_pos = entry_end + 1;
            }
            
            pos = table_end + 1;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing parser: " << e.what() << "\n";
        return false;
    }
}

bool LLParser::Parse(const std::vector<std::string>& input_tokens) {
    if (input_tokens.empty()) {
        std::cerr << "Error: Empty input\n";
        return false;
    }

    // Clear previous trace
    parse_trace_.clear();

    // Initialize parsing stack with end marker and start symbol
    std::stack<std::string> stack;
    stack.push("$");
    stack.push(start_symbol_);

    size_t input_pos = 0;
    bool has_error = false;

    while (!stack.empty() && input_pos < input_tokens.size()) {
        std::string stack_top = stack.top();
        std::string current_input = input_tokens[input_pos];
        
        // Get full stack for trace
        std::vector<std::string> full_stack = GetStackContents(stack);

        // If stack top matches current input
        if (stack_top == current_input) {
            AddTraceEntry(stack_top, full_stack, current_input, "match");
            stack.pop();
            input_pos++;
            continue;
        }

        // If stack top is a non-terminal
        if (IsNonTerminal(stack_top)) {
            std::string table_entry = parsing_table_[stack_top][current_input];
            
            if (table_entry == "error") {
                AddTraceEntry(stack_top, full_stack, current_input, "error: no production rule");
                has_error = true;
                break;
            }
            
            if (table_entry == "synch") {
                AddTraceEntry(stack_top, full_stack, current_input, "sync: skipping non-terminal");
                stack.pop();
                continue;
            }

            // Convert table entry to production indices
            int prod_idx = std::stoi(table_entry) / 100;
            int rule_idx = std::stoi(table_entry) % 100;

            // Get the production rule
            const auto& rule = productions_[prod_idx].GetRules()[rule_idx];
            std::string action = GetProductionString(prod_idx, rule_idx);
            AddTraceEntry(stack_top, full_stack, current_input, action);

            // Replace the non-terminal with its production (in reverse order)
            stack.pop();
            const auto& entities = rule.GetEntities();
            if (!entities.empty() && entities[0] != "EPSILON") {
                for (auto it = entities.rbegin(); it != entities.rend(); ++it) {
                    stack.push(*it);
                }
            }
        } else {
            // Stack top is a terminal but doesn't match input
            AddTraceEntry(stack_top, full_stack, current_input, "error: terminal mismatch");
            has_error = true;
            break;
        }
    }

    // Check if we've successfully parsed the entire input
    bool success = !has_error && stack.size() == 1 && stack.top() == "$" && input_pos == input_tokens.size();
    if (success) {
        std::vector<std::string> final_stack;
        final_stack.push_back("$");
        AddTraceEntry("$", final_stack, "$", "accept");
    }
    return success;
}

// Helper to get stack contents as vector
std::vector<std::string> LLParser::GetStackContents(std::stack<std::string> stack_copy) const {
    std::vector<std::string> contents;
    while (!stack_copy.empty()) {
        contents.push_back(stack_copy.top());
        stack_copy.pop();
    }
    // Reverse to get correct order (bottom to top)
    std::reverse(contents.begin(), contents.end());
    return contents;
}

void LLParser::AddTraceEntry(const std::string& stack_top, const std::vector<std::string>& full_stack, 
                           const std::string& current_input, const std::string& action) {
    parse_trace_.push_back({stack_top, full_stack, current_input, action});
}

void LLParser::DumpTraceAsJson(std::ofstream& out_file) const {
    out_file << "[\n";
    for (size_t i = 0; i < parse_trace_.size(); ++i) {
        out_file << "  {\n";
        out_file << "    \"stack_top\": \"" << parse_trace_[i].stack_top << "\",\n";
        
        // Add full stack array
        out_file << "    \"full_stack\": [";
        for (size_t j = 0; j < parse_trace_[i].full_stack.size(); ++j) {
            out_file << "\"" << parse_trace_[i].full_stack[j] << "\"";
            if (j < parse_trace_[i].full_stack.size() - 1) {
                out_file << ", ";
            }
        }
        out_file << "],\n";
        
        out_file << "    \"input\": \"" << parse_trace_[i].current_input << "\",\n";
        out_file << "    \"action\": \"" << parse_trace_[i].action << "\"\n";
        out_file << "  }" << (i < parse_trace_.size() - 1 ? "," : "") << "\n";
    }
    out_file << "]\n";
}

bool LLParser::IsTerminal(const std::string& symbol) const {
    return std::find(terminals_.begin(), terminals_.end(), symbol) != terminals_.end();
}

bool LLParser::IsNonTerminal(const std::string& symbol) const {
    return std::find(non_terminals_.begin(), non_terminals_.end(), symbol) != non_terminals_.end();
}

std::string LLParser::GetProductionString(int prod_idx, int rule_idx) const {
    const auto& production = productions_[prod_idx];
    const auto& rule = production.GetRules()[rule_idx];
    std::string result = production.GetParent() + " → ";
    const auto& entities = rule.GetEntities();
    if (entities.empty()) {
        result += "ε";
    } else {
        for (size_t i = 0; i < entities.size(); ++i) {
            if (i > 0) result += " ";
            result += entities[i];
        }
    }
    return result;
}

} // namespace jucc::parser 