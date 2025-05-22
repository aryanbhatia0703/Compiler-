#ifndef JUCC_JSON_WRITER_H
#define JUCC_JSON_WRITER_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>

namespace jucc {

class JsonWriter {
public:
    static void WriteGrammarToJson(
        const std::vector<std::string>& terminals,
        const std::vector<std::string>& non_terminals,
        const std::string& start_symbol,
        const std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>>& productions,
        const std::string& filepath
    ) {
        std::ofstream out(filepath);
        if (!out.is_open()) {
            throw std::runtime_error("Failed to open output file: " + filepath);
        }

        out << "{\n";
        
        // Write terminals
        out << "  \"terminals\": [\n";
        for (size_t i = 0; i < terminals.size(); ++i) {
            out << "    \"" << terminals[i] << "\"";
            if (i < terminals.size() - 1) out << ",";
            out << "\n";
        }
        out << "  ],\n";

        // Write non-terminals
        out << "  \"non_terminals\": [\n";
        for (size_t i = 0; i < non_terminals.size(); ++i) {
            out << "    \"" << non_terminals[i] << "\"";
            if (i < non_terminals.size() - 1) out << ",";
            out << "\n";
        }
        out << "  ],\n";

        // Write start symbol
        out << "  \"start_symbol\": \"" << start_symbol << "\",\n";

        // Write productions
        out << "  \"productions\": [\n";
        for (size_t i = 0; i < productions.size(); ++i) {
            out << "    {\n";
            out << "      \"parent\": \"" << productions[i].first << "\",\n";
            out << "      \"rules\": [\n";
            
            const auto& rules = productions[i].second;
            for (size_t j = 0; j < rules.size(); ++j) {
                out << "        [";
                const auto& rule = rules[j];
                for (size_t k = 0; k < rule.size(); ++k) {
                    out << "\"" << rule[k] << "\"";
                    if (k < rule.size() - 1) out << ", ";
                }
                out << "]";
                if (j < rules.size() - 1) out << ",";
                out << "\n";
            }
            
            out << "      ]\n";
            out << "    }";
            if (i < productions.size() - 1) out << ",";
            out << "\n";
        }
        out << "  ]\n";
        
        out << "}\n";
    }
};

} // namespace jucc

#endif // JUCC_JSON_WRITER_H 