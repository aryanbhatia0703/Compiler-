#include "../include/parser/parsing_table.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>

namespace jucc::parser {

std::string ParsingTable::GenerateErrorMessage(const std::string &production, const std::string &symbol) {
  std::string ret;
  ret += "parsing table error: duplicate entry in parsing table, ";
  ret += "production: ";
  ret = ret.append(production);
  ret += " symbol: ";
  ret = ret.append(symbol);
  return ret;
}

void ParsingTable::BuildTable() {
  // fill initially all errors
  for (auto &nt : non_terminals_) {
    for (auto &t : terminals_) {
      table_[nt][t] = std::make_pair(-1, -1);  // error entry
    }
  }

  // We consider that the symbols on the Follow(A) to be in the synchronization set
  // BUT ONLY FOR NON-TERMINALS
  for (auto &nt : non_terminals_) {
    if (follows_.count(nt) != 0U) {
      for (const auto &symbol : follows_[nt]) {
        // Only add synch entries for non-terminals, not terminals
        if (std::find(terminals_.begin(), terminals_.end(), nt) == terminals_.end()) {
          if (table_[nt][symbol].first != -1) {
            errors_.push_back(GenerateErrorMessage(nt, symbol));
          }
          table_[nt][symbol] = std::make_pair(-2, -2);  // synch entry
        }
      }
    }
  }

  // Process productions
  for (size_t prod_no = 0; prod_no < productions_.size(); prod_no++) {
    const auto& rules = productions_[prod_no].GetRules();
    for (size_t rule_no = 0; rule_no < rules.size(); rule_no++) {
      const auto& entities = rules[rule_no].GetEntities();
      
      if (entities.empty()) {
        // epsilon production
        if (follows_.count(productions_[prod_no].GetParent()) != 0U) {
          for (const auto &symbol : follows_[productions_[prod_no].GetParent()]) {
            auto& entry = table_[productions_[prod_no].GetParent()][symbol];
            if (entry.first != -1 && entry.first != -2) {
              errors_.push_back(GenerateErrorMessage(productions_[prod_no].GetParent(), symbol));
            }
            entry = std::make_pair(prod_no, rule_no);
          }
        }
        continue;
      }

      std::string first_entity = entities[0];
      // check if first_entity is terminal
      if (std::find(terminals_.begin(), terminals_.end(), first_entity) != terminals_.end()) {
        auto& entry = table_[productions_[prod_no].GetParent()][first_entity];
        if (entry.first != -1 && entry.first != -2) {
          errors_.push_back(GenerateErrorMessage(productions_[prod_no].GetParent(), first_entity));
        }
        entry = std::make_pair(prod_no, rule_no);
      }
      // first entity is a non-terminal
      else if (firsts_.count(first_entity) != 0U) {
        for (const auto &symbol : firsts_[first_entity]) {
          if (symbol != "EPSILON") {
            auto& entry = table_[productions_[prod_no].GetParent()][symbol];
            if (entry.first != -1 && entry.first != -2) {
              errors_.push_back(GenerateErrorMessage(productions_[prod_no].GetParent(), symbol));
            }
            entry = std::make_pair(prod_no, rule_no);
          }
        }
      }
    }
  }
}

std::pair<int, int> ParsingTable::GetEntry(const std::string &non_terminal, const std::string &terminal) {
  return table_[non_terminal][terminal];
}

void ParsingTable::DumpAsJson(const std::string &filepath) const {
  std::ofstream out(filepath);
  if (!out.is_open()) {
    std::cerr << "Failed to open output file: " << filepath << std::endl;
    return;
  }

  out << "{\n";
  bool first_nt = true;
  for (const auto &nt_pair : table_) {
    if (!first_nt) out << ",\n";
    first_nt = false;
    out << "  \"" << nt_pair.first << "\": {";
    
    bool first_t = true;
    for (const auto &t_pair : nt_pair.second) {
      if (!first_t) out << ",";
      first_t = false;
      
      // Convert the pair to a string representation
      std::string value;
      if (t_pair.second.first == -1) {
        value = "error";
      } else if (t_pair.second.first == -2) {
        value = "synch";
      } else {
        value = std::to_string(t_pair.second.first * 100 + t_pair.second.second);
      }
      
      out << "\n    \"" << t_pair.first << "\": \"" << value << "\"";
    }
    out << "\n  }";
  }
  out << "\n}\n";

  out.close();
}

}  // namespace jucc::parser
