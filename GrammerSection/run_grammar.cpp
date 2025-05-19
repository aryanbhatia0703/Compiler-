#include "include/grammar/grammar.h"
#include "include/grammar/grammar_transform.h"
#include <iostream>
#include <string>

int main() {
    // Get the current working directory
    std::string input_file = "grammar.txt";
    std::string output_file = "grammar.json";

// Step 1: Parse the grammar file
    jucc::grammar::Parser parser(input_file.c_str());
if (!parser.Parse()) {
std::cerr << "Failed to parse grammar: " << parser.GetError() << std::endl;
return 1;
}

    // Step 2: Remove left recursion
    if (!parser.EliminateLeftRecursion()) {
        std::cerr << "Failed to eliminate left recursion: " << parser.GetError() << std::endl;
        return 1;
    }

    // Step 3: Apply left factoring
    if (!parser.ApplyLeftFactoring()) {
        std::cerr << "Failed to apply left factoring: " << parser.GetError() << std::endl;
        return 1;
}

// Step 4: Dump final grammar as JSON
    parser.DumpGrammarAsJson(output_file);

    std::cout << "Transformed grammar written to " << output_file << "\n";
return 0;
}



