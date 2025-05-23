#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "include/grammar/grammar.h"
#include "include/utils/first_follow.h"

int main() {
    try {
// Load grammar.json generated earlier
std::ifstream input_file("grammar.json");
if (!input_file.is_open()) {
std::cerr << "Error: Could not open grammar.json\n";
return 1;
}

        // Read the entire file into a string
        std::string json_str;
        std::string line;
        while (std::getline(input_file, line)) {
            json_str += line + "\n";
        }
input_file.close();

        // Parse JSON manually (simplified for our specific format)
jucc::grammar::Productions grammar;
        std::string start_symbol;

        // Find start_symbol
        size_t start_pos = json_str.find("\"start_symbol\"");
        if (start_pos != std::string::npos) {
            start_pos = json_str.find("\"", start_pos + 14) + 1;
            size_t end_pos = json_str.find("\"", start_pos);
            start_symbol = json_str.substr(start_pos, end_pos - start_pos);
        }

        // Find productions array
        size_t prod_start = json_str.find("\"productions\"");
        if (prod_start != std::string::npos) {
            prod_start = json_str.find("[", prod_start);
            size_t prod_end = json_str.find_last_of("]");
            std::string prods_str = json_str.substr(prod_start, prod_end - prod_start + 1);

            // Parse each production
            size_t pos = 0;
            while ((pos = prods_str.find("\"parent\"", pos)) != std::string::npos) {
                // Get parent
                pos = prods_str.find("\"", pos + 8) + 1;
                size_t end_pos = prods_str.find("\"", pos);
                std::string parent = prods_str.substr(pos, end_pos - pos);

                // Get rules
                pos = prods_str.find("\"rules\"", pos);
                pos = prods_str.find("[", pos);
                end_pos = prods_str.find("]", pos);
                while (prods_str[end_pos + 1] == ',') {
                    end_pos = prods_str.find("]", end_pos + 1);
                }
                std::string rules_str = prods_str.substr(pos, end_pos - pos + 1);

std::vector<jucc::grammar::Rule> rules;
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
                    if (symbols.size() == 1 && symbols[0] == "EPSILON") {
                        rules.emplace_back(std::vector<std::string>());
                    } else {
  rules.emplace_back(symbols);
                    }
                    rule_pos = rules_str.find("]", rule_pos) + 1;
}

grammar.emplace_back(parent, rules);
                pos = end_pos;
            }
}

// Compute FIRST and FOLLOW sets
auto nullables = jucc::utils::CalcNullables(grammar);
auto firsts = jucc::utils::CalcFirsts(grammar, nullables);
auto follows = jucc::utils::CalcFollows(grammar, firsts, nullables, start_symbol);

        // Write results to first_follow.json
std::ofstream out("first_follow.json");
if (!out.is_open()) {
std::cerr << "Error writing to first_follow.json\n";
return 1;
}

        out << "{\n  \"first\": {\n";
        bool first_nt = true;
        for (const auto &prod : grammar) {
            if (!first_nt) out << ",\n";
            first_nt = false;
            const auto &nt = prod.GetParent();
            out << "    \"" << nt << "\": [";
            bool first_term = true;
            for (const auto &term : firsts[nt]) {
                if (!first_term) out << ", ";
                first_term = false;
                out << "\"" << term << "\"";
            }
            out << "]";
        }
        out << "\n  },\n  \"follow\": {\n";

        first_nt = true;
        for (const auto &prod : grammar) {
            if (!first_nt) out << ",\n";
            first_nt = false;
            const auto &nt = prod.GetParent();
            out << "    \"" << nt << "\": [";
            bool first_term = true;
            for (const auto &term : follows[nt]) {
                if (!first_term) out << ", ";
                first_term = false;
                out << "\"" << term << "\"";
            }
            out << "]";
        }
        out << "\n  }\n}\n";

out.close();
std::cout << "✅ FIRST and FOLLOW sets saved to first_follow.json\n";
return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}