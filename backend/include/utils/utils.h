#ifndef JUCC_UTILS_UTILS_H
#define JUCC_UTILS_UTILS_H

#include <string>
#include <vector>

#include "../grammar/grammar.h"
#include "first_follow.h"
#include "left_factoring.h"
#include "left_recursion.h"
#include "trie/memory_efficient_trie.h"

namespace jucc {
namespace utils {
/**
 * Makes the grammar non ambiguous.
 * @return A set of production free from left recursions and left factors.
 */
grammar::Productions RemoveAllPossibleAmbiguity(const grammar::Productions & /*prods*/);

/**
 * @returns a list of parents from the given set of productions.
 */
std::vector<std::string> GetAllNonTerminals(const grammar::Productions & /*prods*/);

/**
 * @returns a list of terminal symbols from the given set of productions.
 */
std::vector<std::string> GetAllTerminals(const grammar::Productions & /*prods*/);

}  // namespace utils
}  // namespace jucc
#endif
