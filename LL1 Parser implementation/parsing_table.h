#ifndef JUCC_PARSER_PARSING_TABLE_H
#define JUCC_PARSER_PARSING_TABLE_H

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "../../third_party/json.hpp"
#include "../grammar/grammar.h"

namespace jucc::parser {

class ParsingTable {
public:
    // Table entry: pair of (production_index, rule_index)
    using TableEntry = std::pair<int, int>;
    
    // Table type: non-terminal -> (terminal -> production)
    using Table = std::unordered_map<std::string, std::unordered_map<std::string, TableEntry>>;

    // Constructor
    ParsingTable() = default;

    // Build the parsing table
    void BuildTable();

    // Get an entry from the table
    std::pair<int, int> GetEntry(const std::string& non_terminal, const std::string& terminal);

    // Dump table to JSON file
    void DumpAsJson(const std::string& filepath) const;

    // Error handling
    std::string GenerateErrorMessage(const std::string& production, const std::string& symbol);

    // Getters and setters
    const Table& GetTable() const { return table_; }
    void SetTable(const Table& table) { table_ = table; }
    void SetProductions(const std::vector<grammar::Production>& productions) { productions_ = productions; }
    void SetFirsts(const std::unordered_map<std::string, std::vector<std::string>>& firsts) { firsts_ = firsts; }
    void SetFollows(const std::unordered_map<std::string, std::vector<std::string>>& follows) { follows_ = follows; }
    void SetTerminals(const std::vector<std::string>& terminals) { terminals_ = terminals; }
    void SetNonTerminals(const std::vector<std::string>& non_terminals) { non_terminals_ = non_terminals; }

private:
    Table table_;
    std::vector<std::string> terminals_;
    std::vector<std::string> non_terminals_;
    std::vector<std::string> errors_;
    std::vector<grammar::Production> productions_;
    std::unordered_map<std::string, std::vector<std::string>> firsts_;
    std::unordered_map<std::string, std::vector<std::string>> follows_;
    
    // Constants
    static constexpr const char* ERROR_TOKEN = "error";
    static constexpr const char* SYNCH_TOKEN = "synch";
};

// JSON serialization
inline void to_json(nlohmann::json& j, const ParsingTable::TableEntry& entry) {
    j = nlohmann::json{{"prod_idx", entry.first}, {"rule_idx", entry.second}};
}

inline void from_json(const nlohmann::json& j, ParsingTable::TableEntry& entry) {
    j.at("prod_idx").get_to(entry.first);
    j.at("rule_idx").get_to(entry.second);
}

} // namespace jucc::parser

#endif // JUCC_PARSER_PARSING_TABLE_H
