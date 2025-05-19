#include "../include/parser/parser.h"

#include <algorithm>
#include <fstream>
#include<iostream>
namespace jucc::parser
{

  Parser::Parser() : parse_tree_(json::object({}))
  {
    // initialize the stack
    stack_.push(std::string(utils::STRING_ENDMARKER));
    input_string_.clear();
    current_string_.clear();
  }

  std::string Parser::GenerateErrorMessage(const std::string &current_token)
  {
    std::string ret_string;
    ret_string += "parser error: at symbol: " + current_token;
    return ret_string;
  }

  void Parser::SetInputString(std::vector<std::string> inps)
  {
    if (!inps.empty())
    {
      input_string_ = std::move(inps);
      current_string_ = input_string_;
      // augmented string for parsing
      current_string_.emplace_back(std::string(utils::STRING_ENDMARKER));
    }
  }

  void Parser::SetStartSymbol(std::string start)
  {
    start_symbol_ = std::move(start);
    stack_.push(start_symbol_);
  }

  void Parser::SetParsingTable(ParsingTable table) { table_ = std::move(table); }

  bool Parser::IsComplete()
  {
    return (current_step_ == static_cast<int>(current_string_.size())) ||
           stack_.top() == std::string(utils::STRING_ENDMARKER);
  }

  void Parser::ResetParsing()
  {
    while (!stack_.empty())
    {
      stack_.pop();
    }
    stack_.push(std::string(utils::STRING_ENDMARKER));
    stack_.push(start_symbol_);
    current_string_ = input_string_;
    current_step_ = 0;
    current_string_.emplace_back(std::string(utils::STRING_ENDMARKER));
  }

  void Parser::DoNextStep()
  {
    if (!IsComplete())
    {
      current_step_++;
    }
  }

  void Parser::ParseNextStep()
  {
    std::string top_symbol = stack_.top();
    std::string current_token = current_string_[current_step_];
    ParsingTable::Table table = table_.GetTable();
    // skip tokens until it is in the first or is a synch token
    while (!IsComplete() && table[top_symbol][current_token] == std::string(ERROR_TOKEN))
    {
      parser_errors_.push_back(GenerateErrorMessage(current_token));
      DoNextStep();
      if (current_step_ < static_cast<int>(current_string_.size()))
      {
        current_token = current_string_[current_step_];
      }
    }
    if (!IsComplete())
    {
      // if SYNCH TOKEN - We skip the current symbol on stack top
      if (table[top_symbol][current_token] == std::string(SYNCH_TOKEN))
      {
        parser_errors_.push_back(GenerateErrorMessage(current_token));
        stack_.pop();
      }
      else
      {
        auto terminals = table_.GetTerminals();
        // check if current stack top matches the current token
        if (top_symbol == current_token)
        {
          stack_.pop();
          DoNextStep();
        }
        else if (std::find(terminals.begin(), terminals.end(), top_symbol) != terminals.end() &&
                 std::find(terminals.begin(), terminals.end(), current_token) != terminals.end())
        {
          parser_errors_.push_back(GenerateErrorMessage(current_token));
          DoNextStep();
        }
        else
        {
          // we expand the production
          auto prod_rule = table_.GetEntry(top_symbol, current_token);
          auto productions = table_.GetProductions();
          auto req_rule = productions[prod_rule.first].GetRules()[prod_rule.second];
          auto entities = req_rule.GetEntities();
          std::reverse(entities.begin(), entities.end());
          stack_.pop();
          if (!entities.empty() && entities[0] == std::string(grammar::EPSILON))
          {
            production_history_.push_back(prod_rule.first * 100 + prod_rule.second);
            return;
          }
          for (auto &entity : entities)
          {
            stack_.push(entity);
          }
          production_history_.push_back(prod_rule.first * 100 + prod_rule.second);
        }
      }
    }
  }

  void Parser::BuildParseTree()
  {
    // if errors cannot build tree
    if (!parser_errors_.empty())
    {
      return;
    }

    // init parse tree state
    parse_tree_[start_symbol_] = json::object({});
    std::stack<json *> parent_node_stack;
    parent_node_stack.push(&parse_tree_[start_symbol_]);

    auto productions = table_.GetProductions();
    auto terminals = table_.GetTerminals();
    auto non_terminals = table_.GetNonTerminals();
    // iterate over production history and build tree
    for (const auto &prod : production_history_)
    {
      int production_index = prod / 100;
      int rule_index = prod % 100;
      auto parent = productions[production_index].GetParent();
      auto rule = productions[production_index].GetRules()[rule_index];
      auto entities = rule.GetEntities();
      json *parent_node = parent_node_stack.top();
      /**
       * rename entities to handle duplicates
       * Example:
       * change entities from {"A", "A", "B", "A", "C", "B"} to
       * {"A", "A_1", "B", "A_2", "C", "B_1"} inplace
       */
      std::unordered_map<std::string, int> symbol_count;
      // store an reverse map for name of entities before renaming
      std::unordered_map<std::string, std::string> default_name;
      for (auto &entity : entities)
      {
        auto p_entity = entity;
        if (symbol_count[entity]++ != 0)
        {
          entity += "_" + std::to_string(symbol_count[entity] - 1);
        }
        default_name[entity] = p_entity;

        // add renamed entities to current parent node
        if (std::find(terminals.begin(), terminals.end(), p_entity) != terminals.end())
        {
          (*parent_node)[entity] = json();
        }
        else
        {
          (*parent_node)[entity] = json::object({});
        }
      }

      // update parent_node_stack
      parent_node_stack.pop();
      for (auto it = entities.rbegin(); it < entities.rend(); it++)
      {
        if (std::find(non_terminals.begin(), non_terminals.end(), default_name[*it]) != non_terminals.end())
        {
          parent_node_stack.push(&((*parent_node)[*it]));
        }
      }
    }
  }

  bool Parser::WriteParseTree(const std::string &filepath, bool formatted)
  {
    std::ofstream ofs(filepath);
    if (ofs.is_open())
    {
      ofs << (formatted ? FormattedJSON(parse_tree_) : parse_tree_).dump(INDENTATION) << '\n';
      return true;
    }
    return false;
  }

  json Parser::GetTextNode(const std::string &value)
  {
    json j;
    j["text"]["name"] = value;
    return j;
  }

  json Parser::RecRunner(const json &main, std::string key = "")
  {
    if (main.empty())
    {
      return GetTextNode(key);
    }
    auto body = main;
    if (key.empty())
    {
      for (auto it = main.begin(); it != main.end();)
      {
        key = it.key();
        body = it.value();
        break;
      }
    }

    json j = GetTextNode(key);
    for (auto it = body.begin(); it != body.end(); it++)
    {
      j["children"].push_back(RecRunner(it.value(), it.key()));
    }
    return j;
  }

  json Parser::FormattedJSON(const json &body) { return Parser::RecRunner(body); }

 bool Parser::LoadFromJson(const json &grammar,
                          const json &first_follow,
                          const json &table,
                          const json &tokens) {
  // Load grammar rules
  for (size_t i = 0; i < grammar.size(); ++i) {
    const auto &entry = grammar[i];
    grammar::Production prod;
    prod.SetParent(entry["parent"]);
    grammar::Rules rules;
    for (size_t j = 0; j < entry["rules"].size(); ++j) {
      const auto &rhs = entry["rules"][j];
      grammar::Rule rule;
      std::vector<std::string> entities;
      for (size_t k = 0; k < rhs.size(); ++k) {
        entities.push_back(rhs[k]);
      }
      rule.SetEntities(entities);
      rules.push_back(rule);
    }
    prod.SetRules(rules);
    this->productions_.push_back(prod);
  }

  // Load terminals and non-terminals
  for (size_t i = 0; i < this->productions_.size(); ++i) {
    const auto &prod = this->productions_[i];
    if (std::find(this->non_terminals_.begin(), this->non_terminals_.end(), prod.GetParent()) == this->non_terminals_.end()) {
      this->non_terminals_.push_back(prod.GetParent());
    }
    const auto &rules = prod.GetRules();
    for (size_t j = 0; j < rules.size(); ++j) {
      const auto &entities = rules[j].GetEntities();
      for (size_t k = 0; k < entities.size(); ++k) {
        const auto &sym = entities[k];
        if (sym != grammar::EPSILON &&
            std::find(this->non_terminals_.begin(), this->non_terminals_.end(), sym) == this->non_terminals_.end() &&
            std::find(this->terminals_.begin(), this->terminals_.end(), sym) == this->terminals_.end()) {
          this->terminals_.push_back(sym);
        }
      }

    }
  }
  this->terminals_.push_back(utils::STRING_ENDMARKER);

  // Load First and Follow sets
  auto first_map = first_follow["first"];
  for (auto it = first_map.begin(); it != first_map.end(); ++it) {
    this->firsts_[it.key()] = it.value().get<std::vector<std::string>>();
  }

  auto follow_map = first_follow["follow"];
  for (auto it = follow_map.begin(); it != follow_map.end(); ++it) {
    this->follows_[it.key()] = it.value().get<std::vector<std::string>>();
  }

  // Load parsing table
  for (auto it = table.begin(); it != table.end(); ++it) {
    std::string nt = it.key(); // Non-terminal
    for (auto jt = it.value().begin(); jt != it.value().end(); ++jt) {
      std::string term = jt.key();       // Terminal
      std::string rule_str = jt.value(); // Rule string like "200", "synch", etc.
      this->table_.SetEntry(nt, term, rule_str);
    }
  }

  // Load tokens with type remapping
  for (size_t i = 0; i < tokens.size(); ++i) {
    std::string type = tokens[i]["type"];
    if (type == "identifier" || type == "int") {
      this->input_tokens_.push_back("id");
    } else {
      this->input_tokens_.push_back(type);
    }
  }
  this->input_tokens_.push_back(utils::STRING_ENDMARKER);

  // Set start symbol
  if (!productions_.empty()) {
    this->start_symbol_ = 'E';
    this->stack_ = std::stack<std::string>();  // Clear the stack
    this->stack_.push(utils::STRING_ENDMARKER);
    this->stack_.push(this->start_symbol_);
  }

  std::cout << "Start symbol: " << this->start_symbol_ << std::endl;
  return true;
}


  bool Parser::SimulateParsing()
  {
    std::stack<std::string> parse_stack;
    parse_stack.push(utils::STRING_ENDMARKER);
    parse_stack.push("E"); // Start symbol

    size_t index = 0;
    std::ofstream out("parse_trace.txt");

while (!parse_stack.empty()) {
  const std::string &top = parse_stack.top();
  const std::string &current = this->input_tokens_[index];

  if (top == current) {
    out << "Matched: " << top << "\n";
    parse_stack.pop();
    index++;
  }
  else if (std::find(this->terminals_.begin(), this->terminals_.end(), top) != this->terminals_.end()) {
    // top is a terminal, but not equal to current
    out << "Error: expected " << top << " but found " << current << "\n";
    return false;
  }
  else {
    // top is a non-terminal → consult parsing table
    const std::string &rule_str = this->table_.GetTable().at(top).at(current);

    if (rule_str == "error" || rule_str == "synch" || rule_str.empty()) {
      out << "Error: no rule for (" << top << ", " << current << ")\n";
      return false;
    }

    int prod_index = std::stoi(rule_str) / 100;
    int rule_index = std::stoi(rule_str) % 100;
    const auto &rhs = this->productions_[prod_index].GetRules()[rule_index].GetEntities();

    parse_stack.pop();
    for (auto it = rhs.rbegin(); it != rhs.rend(); ++it) {
      if (*it != grammar::EPSILON)
        parse_stack.push(*it);
    }

    out << "Applied: " << this->productions_[prod_index].GetParent() << " → ";
    for (const auto &sym : rhs) out << sym << " ";
    out << "\n";
  }
}
   std::cout << "Production history size: " << production_history_.size() << std::endl;


    out << "Parsing completed successfully.\n";
    out.close();
    return true;
  }

} // namespace jucc::parser

