#ifndef STUDYSYNC_QUESTION_SIMILARITY_HPP
#define STUDYSYNC_QUESTION_SIMILARITY_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <iomanip>
#include <utility>

#include "models/Question.hpp"

namespace studysync {

/**
 * @brief Detailed breakdown of pairwise question comparison results.
 */
struct SimilarityResult {
    double jaccardScore;        // [0.0, 1.0] Token-set Jaccard similarity
    double levenshteinScore;    // [0.0, 1.0] Normalized Levenshtein similarity
    double combinedScore;       // [0.0, 1.0] Weighted composite score
    bool isMatch;               // True if combinedScore >= threshold and no conflicts
    bool hasConflict;           // True if mutually exclusive concepts or negations mismatch
    std::string conflictReason; // Human-readable explanation of detected concept conflict
    std::string explanation;    // Comprehensive audit trail explaining decision
    std::vector<std::string> sharedTokens;   // Common content tokens
    std::vector<std::string> uniqueTokensA;  // Tokens unique to text A
    std::vector<std::string> uniqueTokensB;  // Tokens unique to text B
};

/**
 * @brief Represents a cluster of semantically equivalent or near-identical question patterns.
 */
struct QuestionCluster {
    std::string clusterId;
    std::string representativeId;
    std::string representativeText;
    std::vector<std::string> memberQuestionIds;
    std::vector<std::string> coreKeywords;
};

/**
 * @brief Rule defining mutually exclusive or distinct academic concept terms.
 */
struct ConceptConflictRule {
    std::string termA;
    std::string termB;
    std::string description;
};

/**
 * @brief Text normalization and tokenization pipeline for examination questions.
 */
class TextNormalizer {
public:
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::string toLower(const std::string& str) {
        std::string lower = str;
        for (char& c : lower) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return lower;
    }

    static std::string collapseWhitespace(const std::string& str) {
        std::string result;
        result.reserve(str.size());
        bool inSpace = false;
        for (char c : str) {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                if (!inSpace) {
                    result += ' ';
                    inSpace = true;
                }
            } else {
                result += c;
                inSpace = false;
            }
        }
        return trim(result);
    }

    /**
     * @brief Identifies whether a token is a crucial negation or exclusion keyword.
     * Negation terms must NEVER be stripped as stopwords.
     */
    static bool isNegationTerm(const std::string& token) {
        static const std::vector<std::string> negations = {
            "not", "no", "never", "except", "without", "non", "neither", "nor"
        };
        return std::find(negations.begin(), negations.end(), token) != negations.end();
    }

    /**
     * @brief Checks if a token is a generic academic instructional word.
     */
    static bool isAcademicStopword(const std::string& token) {
        if (isNegationTerm(token)) return false; // Safety: Never drop negations

        static const std::vector<std::string> stopwords = {
            "what", "is", "are", "the", "a", "an", "explain", "describe", "define",
            "state", "prove", "write", "short", "note", "on", "with", "suitable",
            "diagram", "diagrams", "example", "examples", "give", "derive",
            "expression", "for", "discuss", "briefly", "illustrate", "differentiate",
            "between", "compare", "contrast", "in", "of", "to", "and", "or", "by",
            "using", "how", "does", "do", "its", "their", "following", "given",
            "working", "principle", "principles", "concept", "concepts", "step", "steps", "procedure"
        };
        return std::find(stopwords.begin(), stopwords.end(), token) != stopwords.end();
    }

    /**
     * @brief Tokenizes text into normalized words, protecting technical identifiers,
     * abbreviations (C++, SQL, BFS, DFS), and complexity notations (e.g. O(n), O(log n)).
     */
    static std::vector<std::string> tokenize(const std::string& text, bool filterStopwords = false) {
        std::string lower = toLower(text);
        std::vector<std::string> tokens;
        std::string current;

        for (size_t i = 0; i < lower.size(); ++i) {
            char c = lower[i];

            // Protect Big-O complexity notation: e.g. "o(n)", "o(log n)", "o(1)", "o(n^2)", "o(v+e)"
            if ((c == 'o' || c == 'o') && i + 1 < lower.size() && lower[i + 1] == '(') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                size_t closeParen = lower.find(')', i + 1);
                if (closeParen != std::string::npos && (closeParen - i) <= 20) {
                    std::string bigO = lower.substr(i, closeParen - i + 1);
                    // Normalize internal spaces in complexity notation
                    bigO.erase(std::remove(bigO.begin(), bigO.end(), ' '), bigO.end());
                    tokens.push_back(bigO);
                    i = closeParen;
                    continue;
                }
            }

            // Protect C++ identifier
            if (c == 'c' && i + 2 < lower.size() && lower[i + 1] == '+' && lower[i + 2] == '+') {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                tokens.push_back("c++");
                i += 2;
                continue;
            }

            // Standard alphanumeric and hyphenated terms
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || (c == '-' && !current.empty())) {
                current += c;
            } else {
                if (!current.empty()) {
                    // Trim trailing hyphen
                    if (current.back() == '-') current.pop_back();
                    if (!current.empty()) {
                        tokens.push_back(current);
                    }
                    current.clear();
                }
            }
        }

        if (!current.empty()) {
            if (current.back() == '-') current.pop_back();
            if (!current.empty()) {
                tokens.push_back(current);
            }
        }

        if (!filterStopwords) {
            return tokens;
        }

        std::vector<std::string> filtered;
        for (const auto& t : tokens) {
            if (!isAcademicStopword(t)) {
                filtered.push_back(t);
            }
        }
        return filtered;
    }

    /**
     * @brief Normalizes raw question text for string-level comparisons.
     */
    static std::string normalizeString(const std::string& text) {
        auto tokens = tokenize(text, false);
        std::string result;
        for (size_t i = 0; i < tokens.size(); ++i) {
            result += tokens[i];
            if (i + 1 < tokens.size()) result += ' ';
        }
        return result;
    }
};

/**
 * @brief Algorithmic engine computing token-set Jaccard similarity,
 * space-optimized Levenshtein edit distance, concept conflicts, and composite scores.
 */
class SimilarityEngine {
public:
    static constexpr double DEFAULT_JACCARD_WEIGHT = 0.60;
    static constexpr double DEFAULT_LEVENSHTEIN_WEIGHT = 0.40;
    static constexpr double DEFAULT_MATCH_THRESHOLD = 0.70;

    /**
     * @brief Space-optimized Levenshtein distance using a two-row buffer: O(min(M, N)) space.
     */
    static size_t levenshteinDistance(const std::string& s1, const std::string& s2) {
        size_t m = s1.size();
        size_t n = s2.size();

        if (m == 0) return n;
        if (n == 0) return m;

        // Ensure s1 is the shorter string for O(min(M, N)) space
        if (m > n) {
            return levenshteinDistance(s2, s1);
        }

        std::vector<size_t> prev(m + 1);
        std::vector<size_t> curr(m + 1);

        for (size_t i = 0; i <= m; ++i) {
            prev[i] = i;
        }

        for (size_t j = 1; j <= n; ++j) {
            curr[0] = j;
            char c2 = s2[j - 1];
            for (size_t i = 1; i <= m; ++i) {
                char c1 = s1[i - 1];
                size_t cost = (c1 == c2) ? 0 : 1;
                curr[i] = std::min({
                    prev[i] + 1,        // Insertion
                    curr[i - 1] + 1,    // Deletion
                    prev[i - 1] + cost  // Substitution
                });
            }
            prev = curr;
        }

        return curr[m];
    }

    /**
     * @brief Normalized Levenshtein similarity bounded in [0.0, 1.0].
     */
    static double levenshteinSimilarity(const std::string& s1, const std::string& s2) {
        if (s1.empty() && s2.empty()) return 1.0;
        if (s1.empty() || s2.empty()) return 0.0;
        if (s1 == s2) return 1.0;

        size_t maxLen = std::max(s1.size(), s2.size());
        size_t dist = levenshteinDistance(s1, s2);
        return 1.0 - (static_cast<double>(dist) / static_cast<double>(maxLen));
    }

    /**
     * @brief Jaccard similarity over unique normalized token sets.
     */
    static double jaccardSimilarity(const std::vector<std::string>& tokens1,
                                     const std::vector<std::string>& tokens2) {
        if (tokens1.empty() && tokens2.empty()) return 1.0;
        if (tokens1.empty() || tokens2.empty()) return 0.0;

        std::vector<std::string> setA = tokens1;
        std::vector<std::string> setB = tokens2;

        std::sort(setA.begin(), setA.end());
        setA.erase(std::unique(setA.begin(), setA.end()), setA.end());

        std::sort(setB.begin(), setB.end());
        setB.erase(std::unique(setB.begin(), setB.end()), setB.end());

        std::vector<std::string> intersection;
        std::set_intersection(setA.begin(), setA.end(),
                              setB.begin(), setB.end(),
                              std::back_inserter(intersection));

        std::vector<std::string> unionSet;
        std::set_union(setA.begin(), setA.end(),
                       setB.begin(), setB.end(),
                       std::back_inserter(unionSet));

        if (unionSet.empty()) return 1.0;
        return static_cast<double>(intersection.size()) / static_cast<double>(unionSet.size());
    }

    /**
     * @brief Knowledge base of mutually exclusive or distinct technical concept terms.
     */
    static const std::vector<ConceptConflictRule>& getConflictRules() {
        static const std::vector<ConceptConflictRule> rules = {
            {"binary search", "linear search", "Binary search vs Linear search"},
            {"bfs", "dfs", "BFS (Breadth-First) vs DFS (Depth-First)"},
            {"breadth-first", "depth-first", "Breadth-First vs Depth-First"},
            {"stack", "queue", "Stack (LIFO) vs Queue (FIFO)"},
            {"insertion sort", "selection sort", "Insertion Sort vs Selection Sort"},
            {"insertion sort", "bubble sort", "Insertion Sort vs Bubble Sort"},
            {"selection sort", "bubble sort", "Selection Sort vs Bubble Sort"},
            {"quicksort", "mergesort", "QuickSort vs MergeSort"},
            {"quick sort", "merge sort", "QuickSort vs MergeSort"},
            {"heapsort", "quicksort", "HeapSort vs QuickSort"},
            {"heap sort", "quick sort", "HeapSort vs QuickSort"},
            {"prim", "kruskal", "Prim's vs Kruskal's MST Algorithm"},
            {"min-heap", "max-heap", "Min-Heap vs Max-Heap"},
            {"preorder", "postorder", "Preorder vs Postorder traversal"},
            {"preorder", "inorder", "Preorder vs Inorder traversal"},
            {"inorder", "postorder", "Inorder vs Postorder traversal"},
            {"directed", "undirected", "Directed vs Undirected graph"},
            {"singly", "doubly", "Singly vs Doubly linked list"}
        };
        return rules;
    }

    /**
     * @brief Detects if two questions have a direct concept conflict.
     * High lexical similarity must NOT override an explicit concept conflict.
     */
    static bool detectConceptConflict(const std::string& normA,
                                      const std::string& normB,
                                      std::string& conflictReason) {
        for (const auto& rule : getConflictRules()) {
            bool aHasTermA = normA.find(rule.termA) != std::string::npos;
            bool aHasTermB = normA.find(rule.termB) != std::string::npos;
            bool bHasTermA = normB.find(rule.termA) != std::string::npos;
            bool bHasTermB = normB.find(rule.termB) != std::string::npos;

            // Conflict occurs when Question A specifically tests termA (not termB)
            // and Question B specifically tests termB (not termA).
            // (If both questions test BOTH terms, e.g. "Compare Stack and Queue", no conflict).
            if ((aHasTermA && !aHasTermB && bHasTermB && !bHasTermA) ||
                (aHasTermB && !aHasTermA && bHasTermA && !bHasTermB)) {
                conflictReason = "Concept conflict detected: " + rule.description;
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Detects negation polarity mismatches (e.g. "Which is in-place" vs "Which is NOT in-place").
     */
    static bool detectNegationMismatch(const std::vector<std::string>& rawTokensA,
                                       const std::vector<std::string>& rawTokensB,
                                       std::string& mismatchReason) {
        bool aHasNegation = std::any_of(rawTokensA.begin(), rawTokensA.end(), TextNormalizer::isNegationTerm);
        bool bHasNegation = std::any_of(rawTokensB.begin(), rawTokensB.end(), TextNormalizer::isNegationTerm);

        if (aHasNegation != bHasNegation) {
            mismatchReason = "Negation mismatch: One question contains a negative/exception term ('not', 'except', etc.) while the other does not.";
            return true;
        }
        return false;
    }

    /**
     * @brief Computes comprehensive multi-tier similarity between two question texts.
     */
    static SimilarityResult compareQuestions(const std::string& textA,
                                             const std::string& textB,
                                             double matchThreshold = DEFAULT_MATCH_THRESHOLD) {
        SimilarityResult result;
        result.jaccardScore = 0.0;
        result.levenshteinScore = 0.0;
        result.combinedScore = 0.0;
        result.isMatch = false;
        result.hasConflict = false;

        std::string normA = TextNormalizer::normalizeString(textA);
        std::string normB = TextNormalizer::normalizeString(textB);

        // Raw tokens (for negation checks)
        auto rawTokensA = TextNormalizer::tokenize(textA, false);
        auto rawTokensB = TextNormalizer::tokenize(textB, false);

        // Content tokens (filtered for semantic comparison)
        auto contentTokensA = TextNormalizer::tokenize(textA, true);
        auto contentTokensB = TextNormalizer::tokenize(textB, true);

        // Collect shared and unique tokens
        std::vector<std::string> setA = contentTokensA;
        std::vector<std::string> setB = contentTokensB;
        std::sort(setA.begin(), setA.end());
        setA.erase(std::unique(setA.begin(), setA.end()), setA.end());
        std::sort(setB.begin(), setB.end());
        setB.erase(std::unique(setB.begin(), setB.end()), setB.end());

        std::set_intersection(setA.begin(), setA.end(), setB.begin(), setB.end(),
                              std::back_inserter(result.sharedTokens));
        std::set_difference(setA.begin(), setA.end(), setB.begin(), setB.end(),
                            std::back_inserter(result.uniqueTokensA));
        std::set_difference(setB.begin(), setB.end(), setA.begin(), setA.end(),
                            std::back_inserter(result.uniqueTokensB));

        // 1. Calculate Component Scores
        result.jaccardScore = jaccardSimilarity(contentTokensA, contentTokensB);
        result.levenshteinScore = levenshteinSimilarity(normA, normB);
        result.combinedScore = (DEFAULT_JACCARD_WEIGHT * result.jaccardScore) +
                               (DEFAULT_LEVENSHTEIN_WEIGHT * result.levenshteinScore);

        // 2. Conflict Checks
        std::string conflictMsg;
        if (detectConceptConflict(normA, normB, conflictMsg)) {
            result.hasConflict = true;
            result.conflictReason = conflictMsg;
            result.isMatch = false;
            result.explanation = "REJECTED: " + conflictMsg + " (Combined Score: " +
                                 std::to_string(result.combinedScore) + " overridden)";
            return result;
        }

        std::string negationMsg;
        if (detectNegationMismatch(rawTokensA, rawTokensB, negationMsg)) {
            result.hasConflict = true;
            result.conflictReason = negationMsg;
            result.isMatch = false;
            result.explanation = "REJECTED: " + negationMsg + " (Combined Score: " +
                                 std::to_string(result.combinedScore) + " overridden)";
            return result;
        }

        // 3. Match Evaluation
        result.isMatch = (result.combinedScore >= matchThreshold);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3);
        oss << "Score: " << result.combinedScore
            << " (Jaccard: " << result.jaccardScore
            << ", Levenshtein: " << result.levenshteinScore
            << ", Threshold: " << matchThreshold << ") -> "
            << (result.isMatch ? "ACCEPTED MATCH" : "BELOW THRESHOLD");
        result.explanation = oss.str();

        return result;
    }
};

/**
 * @brief Deterministic, conservative question clustering engine.
 * 
 * Clustering Policy:
 * 1. Questions are processed in deterministic order (sorted by Question ID).
 * 2. To join an existing cluster, a question MUST match the Cluster Representative (score >= threshold).
 * 3. Conservative Anti-Chaining Check: Must NOT trigger any concept conflict with ANY member
 *    already in the cluster, and must maintain pairwise similarity >= 0.50 with all existing members.
 * 4. Cluster representative is deterministically the founding question ID.
 * 5. Original Question text and historical appearance records are never modified.
 */
class QuestionClusterer {
public:
    static constexpr double DEFAULT_MATCH_THRESHOLD = 0.70;
    static constexpr double MIN_INTRA_CLUSTER_SIMILARITY = 0.50;

    static std::vector<QuestionCluster> clusterQuestions(
        const std::vector<Question>& questions,
        double matchThreshold = DEFAULT_MATCH_THRESHOLD) {

        if (questions.empty()) return {};

        // 1. Sort candidate pointers deterministically by ID
        std::vector<const Question*> sortedCandidates;
        sortedCandidates.reserve(questions.size());
        for (const auto& q : questions) {
            sortedCandidates.push_back(&q);
        }
        std::sort(sortedCandidates.begin(), sortedCandidates.end(),
            [](const Question* a, const Question* b) { return a->getId() < b->getId(); });

        std::vector<QuestionCluster> clusters;

        for (const auto* candidate : sortedCandidates) {
            bool placed = false;

            for (auto& cluster : clusters) {
                // Primary Check: Compare with Cluster Representative
                SimilarityResult repComp = SimilarityEngine::compareQuestions(
                    candidate->getText(), cluster.representativeText, matchThreshold);

                if (!repComp.isMatch || repComp.hasConflict) {
                    continue; // Does not match representative
                }

                // Anti-Chaining / Complete Linkage Check:
                // Verify candidate does NOT conflict with ANY existing member of the cluster
                bool memberConflict = false;
                for (const auto& memberId : cluster.memberQuestionIds) {
                    // Find member question text
                    auto it = std::find_if(sortedCandidates.begin(), sortedCandidates.end(),
                        [&memberId](const Question* q) { return q->getId() == memberId; });
                    if (it != sortedCandidates.end()) {
                        SimilarityResult memberComp = SimilarityEngine::compareQuestions(
                            candidate->getText(), (*it)->getText(), MIN_INTRA_CLUSTER_SIMILARITY);
                        if (memberComp.hasConflict || memberComp.combinedScore < MIN_INTRA_CLUSTER_SIMILARITY) {
                            memberConflict = true;
                            break;
                        }
                    }
                }

                if (memberConflict) {
                    continue; // Rejected by conservative anti-chaining policy
                }

                // Safe to merge into cluster
                cluster.memberQuestionIds.push_back(candidate->getId());
                placed = true;
                break;
            }

            if (!placed) {
                // Form a new cluster with this candidate as the representative
                QuestionCluster newCluster;
                newCluster.clusterId = "CLUSTER-" + std::to_string(clusters.size() + 1);
                newCluster.representativeId = candidate->getId();
                newCluster.representativeText = candidate->getText();
                newCluster.memberQuestionIds.push_back(candidate->getId());
                newCluster.coreKeywords = TextNormalizer::tokenize(candidate->getText(), true);
                clusters.push_back(std::move(newCluster));
            }
        }

        return clusters;
    }

    /**
     * @brief Generates an explanatory audit trace comparing two questions.
     */
    static std::string explainComparison(const Question& q1, const Question& q2) {
        SimilarityResult res = SimilarityEngine::compareQuestions(q1.getText(), q2.getText());
        std::ostringstream oss;
        oss << "=== Question Comparison Report ===\n"
            << "Question A [" << q1.getId() << "]: " << q1.getText() << "\n"
            << "Question B [" << q2.getId() << "]: " << q2.getText() << "\n"
            << "Jaccard Token Similarity : " << res.jaccardScore << "\n"
            << "Levenshtein Similarity   : " << res.levenshteinScore << "\n"
            << "Combined Weighted Score  : " << res.combinedScore << "\n"
            << "Decision Status          : " << (res.isMatch ? "MATCH (Same Concept)" : "DISTINCT") << "\n";
        if (res.hasConflict) {
            oss << "Conflict Detected        : " << res.conflictReason << "\n";
        }
        oss << "Shared Content Tokens    : ";
        for (const auto& t : res.sharedTokens) oss << t << " ";
        oss << "\n";
        return oss.str();
    }
};

} // namespace studysync

#endif // STUDYSYNC_QUESTION_SIMILARITY_HPP
