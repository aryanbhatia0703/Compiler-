#include "../include/grammar/grammar_transform.h"
#include <algorithm>
#include <sstream>

namespace jucc {
namespace grammar {

Productions GrammarTransform::EliminateImmediateLeftRecursion(const Production& production) {
    Productions result;
    Rules recursive_rules;
    Rules non_recursive_rules;
    
    // Separate recursive and non-recursive rules
    for (const auto& rule : production.GetRules()) {
        const auto& entities = rule.GetEntities();
        if (!entities.empty() && entities[0] == production.GetParent()) {
            recursive_rules.push_back(rule);
        } else {
            non_recursive_rules.push_back(rule);
        }
    }
    
    // If no left recursion, return original production
    if (recursive_rules.empty()) {
        result.push_back(production);
        return result;
    }
    
    // Create new non-terminal name (A')
    std::string new_non_terminal = production.GetParent() + "_prime";
    
    // Create new rules for original non-terminal (A → βA' | β)
    Rules new_parent_rules;
    for (const auto& rule : non_recursive_rules) {
        std::vector<std::string> new_entities = rule.GetEntities();
        new_entities.push_back(new_non_terminal);
        new_parent_rules.push_back(Rule(new_entities));
    }
    
    // If no non-recursive rules, add epsilon
    if (new_parent_rules.empty()) {
        new_parent_rules.push_back(Rule({new_non_terminal}));
    }
    
    // Create rules for new non-terminal (A' → αA' | ε)
    Rules prime_rules;
    for (const auto& rule : recursive_rules) {
        std::vector<std::string> new_entities(rule.GetEntities().begin() + 1, rule.GetEntities().end());
        new_entities.push_back(new_non_terminal);
        prime_rules.push_back(Rule(new_entities));
    }
    prime_rules.push_back(Rule({EPSILON}));
    
    // Add both productions to result
    result.push_back(Production(production.GetParent(), new_parent_rules));
    result.push_back(Production(new_non_terminal, prime_rules));
    
    return result;
}

Productions GrammarTransform::EliminateLeftRecursion(const Productions& productions) {
    Productions result = productions;
    
    // For each non-terminal Ai
    for (size_t i = 0; i < result.size(); i++) {
        // Replace each production of the form Ai → Ajγ where j < i
        for (size_t j = 0; j < i; j++) {
            Production& curr_prod = result[i];
            const Production& prev_prod = result[j];
            
            Rules new_rules;
            for (const auto& rule : curr_prod.GetRules()) {
                const auto& entities = rule.GetEntities();
                if (!entities.empty() && entities[0] == prev_prod.GetParent()) {
                    // Replace Aj with its rules
                    for (const auto& prev_rule : prev_prod.GetRules()) {
                        std::vector<std::string> new_entities = prev_rule.GetEntities();
                        new_entities.insert(new_entities.end(), 
                                         entities.begin() + 1, 
                                         entities.end());
                        new_rules.push_back(Rule(new_entities));
                    }
                } else {
                    new_rules.push_back(rule);
                }
            }
            curr_prod.SetRules(new_rules);
        }
        
        // Eliminate immediate left recursion
        auto transformed = EliminateImmediateLeftRecursion(result[i]);
        if (transformed.size() > 1) {
            result[i] = transformed[0];
            result.insert(result.begin() + i + 1, transformed[1]);
        }
    }
    
    return result;
}

std::string GrammarTransform::GenerateNewNonTerminal(
    const std::string& base,
    const std::unordered_set<std::string>& existing_names) {
    
    std::string new_name = base;
    int suffix = 1;
    
    while (existing_names.find(new_name) != existing_names.end()) {
        std::stringstream ss;
        ss << base << "_" << suffix++;
        new_name = ss.str();
    }
    
    return new_name;
}

std::vector<std::string> GrammarTransform::FindLongestCommonPrefix(const Rules& rules) {
    if (rules.empty()) return {};
    
    std::vector<std::string> prefix;
    size_t min_length = SIZE_MAX;
    
    // Find minimum length rule
    for (const auto& rule : rules) {
        min_length = std::min(min_length, rule.GetEntities().size());
    }
    
    // Compare entities at each position
    for (size_t i = 0; i < min_length; i++) {
        const std::string& curr_entity = rules[0].GetEntities()[i];
        bool is_common = true;
        
        for (size_t j = 1; j < rules.size(); j++) {
            if (rules[j].GetEntities()[i] != curr_entity) {
                is_common = false;
                break;
            }
        }
        
        if (is_common) {
            prefix.push_back(curr_entity);
        } else {
            break;
        }
    }
    
    return prefix;
}

Productions GrammarTransform::ApplyLeftFactoring(const Productions& productions) {
    Productions result;
    std::unordered_set<std::string> non_terminals;
    
    // Collect all non-terminal names
    for (const auto& prod : productions) {
        non_terminals.insert(prod.GetParent());
    }
    
    // Process each production
    for (const auto& prod : productions) {
        const Rules& rules = prod.GetRules();
        
        // Find rules with common prefixes
        std::vector<bool> processed(rules.size(), false);
        Rules new_rules;
        
        for (size_t i = 0; i < rules.size(); i++) {
            if (processed[i]) continue;
            
            // Find rules with common prefix
            Rules common_prefix_rules = {rules[i]};
            for (size_t j = i + 1; j < rules.size(); j++) {
                if (!processed[j] && rules[i].HasPrefix(rules[j])) {
                    common_prefix_rules.push_back(rules[j]);
                    processed[j] = true;
                }
            }
            
            if (common_prefix_rules.size() > 1) {
                // Extract common prefix
                auto prefix = FindLongestCommonPrefix(common_prefix_rules);
                if (!prefix.empty()) {
                    // Create new non-terminal
                    std::string new_nt = GenerateNewNonTerminal(
                        prod.GetParent() + "_fact",
                        non_terminals
                    );
                    non_terminals.insert(new_nt);
                    
                    // Create new rule with prefix and new non-terminal
                    std::vector<std::string> new_rule_entities = prefix;
                    new_rule_entities.push_back(new_nt);
                    new_rules.push_back(Rule(new_rule_entities));
                    
                    // Create new production for the new non-terminal
                    Rules new_nt_rules;
                    for (const auto& rule : common_prefix_rules) {
                        const auto& entities = rule.GetEntities();
                        if (entities.size() > prefix.size()) {
                            new_nt_rules.push_back(Rule(
                                std::vector<std::string>(
                                    entities.begin() + prefix.size(),
                                    entities.end()
                                )
                            ));
                        } else {
                            new_nt_rules.push_back(Rule({EPSILON}));
                        }
                    }
                    result.push_back(Production(new_nt, new_nt_rules));
                }
            } else {
                new_rules.push_back(rules[i]);
            }
            processed[i] = true;
        }
        
        result.push_back(Production(prod.GetParent(), new_rules));
    }
    
    return result;
}

} // namespace grammar
} // namespace jucc 