#ifndef JUCC_GRAMMAR_TRANSFORM_H
#define JUCC_GRAMMAR_TRANSFORM_H

#include "grammar.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace jucc {
namespace grammar {

class GrammarTransform {
public:
    /**
     * Eliminates immediate left recursion from a single production
     * @param production The production to transform
     * @return A vector of new productions after elimination
     */
    static Productions EliminateImmediateLeftRecursion(const Production& production);

    /**
     * Eliminates all left recursion from the grammar (both immediate and indirect)
     * @param productions The complete set of productions
     * @return Transformed set of productions without left recursion
     */
    static Productions EliminateLeftRecursion(const Productions& productions);

    /**
     * Applies left factoring to a set of productions
     * @param productions The productions to transform
     * @return Left-factored set of productions
     */
    static Productions ApplyLeftFactoring(const Productions& productions);

private:
    /**
     * Generates a new unique non-terminal name
     * @param base The base name for the new non-terminal
     * @param existing_names Set of existing non-terminal names
     * @return A unique non-terminal name
     */
    static std::string GenerateNewNonTerminal(
        const std::string& base,
        const std::unordered_set<std::string>& existing_names
    );

    /**
     * Finds the longest common prefix among a set of rules
     * @param rules The rules to analyze
     * @return The longest common prefix as a vector of symbols
     */
    static std::vector<std::string> FindLongestCommonPrefix(const Rules& rules);
};

} // namespace grammar
} // namespace jucc

#endif // JUCC_GRAMMAR_TRANSFORM_H 