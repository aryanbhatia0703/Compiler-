#include <iostream>
#include <fstream>
#include <string>
#include "include/grammar/grammar.h"
#include "include/utils/first_follow.h"
#include "include/parser/parsing_table.h"

int main() {
    try {
        // Read the grammar.json file
        std::ifstream input_file("grammar.json");
        if (!input_file.is_open()) {
            std::cerr << "Error: Could not open grammar.json\n";
            return 1;
        }

        // Parse JSON manually (simplified for our specific format)
        std::string json_str;
        std::string line;
        while (std::getline(input_file, line)) {
            json_str += line + "\n";
        }
        input_file.close();

        // Parse the grammar components
        jucc::grammar::Productions grammar;
        std::vector<std::string> terminals;
        std::vector<std::string> non_terminals;
        std::string start_symbol;

        // Find terminals array
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
                terminals.push_back(terms_str.substr(pos, end_pos - pos));
                pos = end_pos + 1;
            }
        }

        // Find non-terminals array
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
                non_terminals.push_back(nonterms_str.substr(pos, end_pos - pos));
                pos = end_pos + 1;
            }
        }

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
                    rules.emplace_back(symbols);
                    rule_pos = rules_str.find("]", rule_pos) + 1;
                }

                grammar.emplace_back(parent, rules);
                pos = end_pos;
            }
        }

        // Read first_follow.json for FIRST and FOLLOW sets
        std::ifstream ff_file("first_follow.json");
        if (!ff_file.is_open()) {
            std::cerr << "Error: Could not open first_follow.json\n";
return 1;
}

        std::string ff_str;
        while (std::getline(ff_file, line)) {
            ff_str += line + "\n";
        }
        ff_file.close();

        // Parse FIRST sets
        jucc::utils::SymbolsMap firsts;
        size_t first_start = ff_str.find("\"first\"");
        if (first_start != std::string::npos) {
            first_start = ff_str.find("{", first_start);
            size_t first_end = ff_str.find("}", first_start);
            std::string first_str = ff_str.substr(first_start, first_end - first_start + 1);

            size_t pos = 0;
            while ((pos = first_str.find("\"", pos)) != std::string::npos) {
                pos++;
                size_t end_pos = first_str.find("\"", pos);
                if (end_pos == std::string::npos) break;
                std::string nt = first_str.substr(pos, end_pos - pos);
                
                // Find array of terminals
                pos = first_str.find("[", end_pos);
                end_pos = first_str.find("]", pos);
                std::string terms_str = first_str.substr(pos, end_pos - pos + 1);
                
                std::vector<std::string> terms;
                size_t term_pos = 0;
                while ((term_pos = terms_str.find("\"", term_pos)) != std::string::npos) {
                    term_pos++;
                    size_t term_end = terms_str.find("\"", term_pos);
                    if (term_end == std::string::npos) break;
                    terms.push_back(terms_str.substr(term_pos, term_end - term_pos));
                    term_pos = term_end + 1;
                }
                
                firsts[nt] = terms;
                pos = end_pos;
            }
        }

        // Parse FOLLOW sets
        jucc::utils::SymbolsMap follows;
        size_t follow_start = ff_str.find("\"follow\"");
        if (follow_start != std::string::npos) {
            follow_start = ff_str.find("{", follow_start);
            size_t follow_end = ff_str.find("}", follow_start);
            std::string follow_str = ff_str.substr(follow_start, follow_end - follow_start + 1);

            size_t pos = 0;
            while ((pos = follow_str.find("\"", pos)) != std::string::npos) {
                pos++;
                size_t end_pos = follow_str.find("\"", pos);
                if (end_pos == std::string::npos) break;
                std::string nt = follow_str.substr(pos, end_pos - pos);
                
                // Find array of terminals
                pos = follow_str.find("[", end_pos);
                end_pos = follow_str.find("]", pos);
                std::string terms_str = follow_str.substr(pos, end_pos - pos + 1);
                
                std::vector<std::string> terms;
                size_t term_pos = 0;
                while ((term_pos = terms_str.find("\"", term_pos)) != std::string::npos) {
                    term_pos++;
                    size_t term_end = terms_str.find("\"", term_pos);
                    if (term_end == std::string::npos) break;
                    terms.push_back(terms_str.substr(term_pos, term_end - term_pos));
                    term_pos = term_end + 1;
                }
                
                follows[nt] = terms;
                pos = end_pos;
            }
        }

        // Create and build the parsing table
        jucc::parser::ParsingTable table(terminals, non_terminals);
        table.SetProductions(grammar);
table.SetFirsts(firsts);
table.SetFollows(follows);
table.BuildTable();

        // Save the parsing table to JSON
table.DumpAsJson("parsing_table.json");
        std::cout << "✅ Parsing table generated successfully!\n";

        // Check for any errors during table construction
        const auto& errors = table.GetErrors();
        if (!errors.empty()) {
            std::cout << "\nWarning: Grammar is not LL(1). Found following conflicts:\n";
            for (const auto& error : errors) {
                std::cout << error << "\n";
            }
        }

return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}