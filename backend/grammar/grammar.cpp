#include "../include/grammar/grammar.h"
#include "../include/grammar/grammar_transform.h"
#include "../include/json_writer.h"
#include<bits/stdc++.h>
#include <iostream>  
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace jucc::grammar {

bool HasParent(const grammar::Productions &productions, const std::string &parent) {
  return std::any_of(productions.begin(), productions.end(),
                     [&](const grammar::Production &prod) { return prod.GetParent() == parent; });
}

grammar::Rules GetRulesForParent(const grammar::Productions &productions, const std::string &parent) {
  for (const auto &production : productions) {
    if (production.GetParent() == parent) {
      return production.GetRules();
    }
  }
  return grammar::Rules();
}

Parser::Parser(const char *filepath) { file_ = std::ifstream(filepath); }

Parser::~Parser() {
  if (file_.is_open()) {
    file_.close();
  }
}

std::vector<std::string> Parser::FastTokenize(const std::string &s) {
  std::vector<std::string> res;
  std::stringstream ss(s);
  std::string token;
  while (ss >> token) {
    res.push_back(token);
  }
  return res;
}

/**
 * This is based on a basic state machine that implicitly uses a grammar to parse.
 * The parse states represent a block of a .g grammar file.
 * Eg. ParseState TERMINALS imples we have seen a %terminals token and yet to see
 * a % block closing token.
 * RuleState keeps track of additonal states required to parse a rule inside of a
 * %rule block.
 */
bool Parser::Parse() {
  enum ParseState { BASIC, TERMINALS, NON_TERMINALS, START, RULES };
  enum RuleState { LEFT, COLON, ENTITY };

  if (!file_.is_open()) {
    error_ = "grammar parsing error: file not found";
    return false;
  }

    ParseState curr_parse_state = BASIC;
    std::string line;
    std::unordered_map<std::string, std::vector<std::vector<std::string>>> grammar;

      std::string production_parent;
      std::vector<std::string> rule_entities;
  RuleState curr_rule_state = LEFT;

  while (getline(file_, line)) {
    // Skip empty lines and comments
    if (line.empty() || line[0] == '#') {
        continue;
      }
    
      std::vector<std::string> tokens = FastTokenize(line);
    if (tokens.empty()) continue;

    // Handle section markers
    if (tokens[0][0] == '%') {
      if (tokens[0] == "%end") {
        if (curr_parse_state == RULES && !rule_entities.empty()) {
          grammar[production_parent].push_back(rule_entities);
          rule_entities.clear();
        }
        curr_parse_state = BASIC;
      } else if (tokens[0] == "%terminals") {
        curr_parse_state = TERMINALS;
      } else if (tokens[0] == "%non_terminals") {
        curr_parse_state = NON_TERMINALS;
      } else if (tokens[0] == "%start") {
        curr_parse_state = START;
      } else if (tokens[0] == "%rules") {
        curr_parse_state = RULES;
        curr_rule_state = LEFT;
      } else {
        error_ = "grammar parsing error: invalid section marker: " + tokens[0];
          return false;
        }
      continue;
    }

    // Process tokens based on current state
    if (curr_parse_state == BASIC) {
      error_ = "grammar parsing error: token outside section: " + tokens[0];
      return false;
    }

      for (const auto &token : tokens) {
        switch (curr_parse_state) {
          case TERMINALS:
            if (token == std::string(EPSILON)) {
              error_ = "grammar parsing error: EPSILON is reserved";
              return false;
            }
          std::cout << "Adding terminal: '" << token << "'" << std::endl;
            terminals_.push_back(token);
            break;

          case NON_TERMINALS:
            if (token == std::string(EPSILON)) {
              error_ = "grammar parsing error: EPSILON is reserved";
              return false;
            }
            non_terminals_.push_back(token);
            break;

          case START:
          if (!start_symbol_.empty()) {
              error_ = "grammar parsing error: ambiguous start symbol";
              return false;
            }
            start_symbol_ = token;
            break;

          case RULES:
            switch (curr_rule_state) {
              case LEFT:
                if (token == std::string(EPSILON)) {
                  error_ = "grammar parsing error: production cannot start with EPSILON";
                  return false;
                }
                production_parent = token;
                curr_rule_state = COLON;
                break;

              case COLON:
                if (token != ":") {
                  error_ = "grammar parsing error: rules syntax error ':' expected: " + token;
                  return false;
                }
                curr_rule_state = ENTITY;
                break;

              case ENTITY:
                rule_entities.push_back(token);
                break;
            }
            break;

        case BASIC:
          error_ = "grammar parsing error: invalid token outside block: " + token;
          return false;
        }
    }

    // Handle end of rule line
    if (curr_parse_state == RULES && curr_rule_state == ENTITY && !rule_entities.empty()) {
      grammar[production_parent].push_back(rule_entities);
      rule_entities.clear();
      curr_rule_state = LEFT;
    }
  }

  // Validate grammar
  std::unordered_set<std::string> terminals(terminals_.begin(), terminals_.end());
  std::unordered_set<std::string> non_terminals(non_terminals_.begin(), non_terminals_.end());

    if (terminals.size() != terminals_.size()) {
    error_ = "grammar parsing error: duplicate terminals";
      return false;
    }

  if (non_terminals.size() != non_terminals_.size()) {
    error_ = "grammar parsing error: duplicate non-terminals";
        return false;
    }

  for (const auto& nt : non_terminals) {
    if (terminals.find(nt) != terminals.end()) {
      error_ = "grammar parsing error: symbol '" + nt + "' cannot be both terminal and non-terminal";
      return false;
    }
  }

  // Convert grammar to Productions
  for (const auto& entry : grammar) {
    const std::string& parent = entry.first;
    const auto& rules = entry.second;
    
    if (non_terminals.find(parent) == non_terminals.end()) {
        error_ = "grammar parsing error: undefined non-terminal: " + parent;
        return false;
      }

    Rules prod_rules;
    for (const auto& rule : rules) {
        for (const auto& symbol : rule) {
            if (symbol != EPSILON && 
                terminals.find(symbol) == terminals.end() && 
                non_terminals.find(symbol) == non_terminals.end()) {
                error_ = "grammar parsing error: undefined symbol: " + symbol;
            return false;
          }
        }
        prod_rules.push_back(Rule(rule));
    }
    grammar_.push_back(Production(parent, prod_rules));
    }

    return true;
}

std::string Rule::ToString() const {
  std::stringstream ss;
    for (size_t i = 0; i < entities_.size(); ++i) {
        if (i > 0) ss << " ";
        ss << entities_[i];
  }
  return ss.str();
}

bool Rule::HasPrefix(const Rule& prefix) const {
    const auto& prefix_entities = prefix.GetEntities();
    if (prefix_entities.size() > entities_.size()) {
    return false;
  }

    return std::equal(prefix_entities.begin(), prefix_entities.end(), entities_.begin());
  }

void Parser::DumpGrammarAsJson(const std::string &filepath) {
    std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> productions_data;
    
    // Convert productions to the format expected by JsonWriter
    for (const auto& prod : grammar_) {
        std::vector<std::vector<std::string>> rules_data;
        for (const auto& rule : prod.GetRules()) {
            rules_data.push_back(rule.GetEntities());
      }
        productions_data.emplace_back(prod.GetParent(), rules_data);
  }

    // Write to JSON using our JsonWriter
    JsonWriter::WriteGrammarToJson(
        terminals_,
        non_terminals_,
        start_symbol_,
        productions_data,
        filepath
    );
}

void DumpGrammarAsJson(const Productions &productions, const std::string &filepath) {
    std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> productions_data;
    
    // Convert productions to the format expected by JsonWriter
    for (const auto& prod : productions) {
        std::vector<std::vector<std::string>> rules_data;
        for (const auto& rule : prod.GetRules()) {
            rules_data.push_back(rule.GetEntities());
        }
        productions_data.emplace_back(prod.GetParent(), rules_data);
    }
    
    // Write to JSON using our JsonWriter
    JsonWriter::WriteGrammarToJson(
        std::vector<std::string>(),  // empty terminals
        std::vector<std::string>(),  // empty non-terminals
        "",                          // empty start symbol
        productions_data,
        filepath
    );
}

bool Parser::EliminateLeftRecursion() {
    try {
        grammar_ = GrammarTransform::EliminateLeftRecursion(grammar_);
        
        // Update non-terminals list with new ones
        std::unordered_set<std::string> non_term_set(non_terminals_.begin(), non_terminals_.end());
        for (const auto& prod : grammar_) {
            if (non_term_set.find(prod.GetParent()) == non_term_set.end()) {
                non_terminals_.push_back(prod.GetParent());
                non_term_set.insert(prod.GetParent());
            }
        }
        return true;
    } catch (const std::exception& e) {
        error_ = "Error during left recursion elimination: " + std::string(e.what());
        return false;
    }
}

bool Parser::ApplyLeftFactoring() {
    try {
        grammar_ = GrammarTransform::ApplyLeftFactoring(grammar_);
        
        // Update non-terminals list with new ones
        std::unordered_set<std::string> non_term_set(non_terminals_.begin(), non_terminals_.end());
        for (const auto& prod : grammar_) {
            if (non_term_set.find(prod.GetParent()) == non_term_set.end()) {
                non_terminals_.push_back(prod.GetParent());
                non_term_set.insert(prod.GetParent());
}
        }
        return true;
    } catch (const std::exception& e) {
        error_ = "Error during left factoring: " + std::string(e.what());
        return false;
}
}

}  // namespace jucc::grammar
