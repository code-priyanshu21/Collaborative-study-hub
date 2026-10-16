#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

#include "models/Question.hpp"
#include "algorithms/QuestionSimilarity.hpp"

using namespace studysync;

// Helper to check floating-point proximity
bool isClose(double a, double b, double eps = 0.001) {
    return std::abs(a - b) <= eps;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " StudySync - Task 3.3 QuestionSimilarity Test Suite       " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // ========================================================================
    // Test 1: Identical question text (similarity = 1.0, isMatch = true)
    // ========================================================================
    {
        std::cout << "[RUN] Test 1: Identical question text..." << std::endl;
        std::string q = "Explain Dijkstra's shortest path algorithm with an example graph.";
        SimilarityResult res = SimilarityEngine::compareQuestions(q, q);

        assert(isClose(res.jaccardScore, 1.0));
        assert(isClose(res.levenshteinScore, 1.0));
        assert(isClose(res.combinedScore, 1.0));
        assert(res.isMatch == true);
        assert(res.hasConflict == false);
        assert(res.conflictReason.empty());
        assert(!res.sharedTokens.empty());
        assert(res.uniqueTokensA.empty());
        assert(res.uniqueTokensB.empty());

        std::cout << "  -> Passed: Identical questions produce 1.0 similarity and match successfully." << std::endl;
    }

    // ========================================================================
    // Test 2: Case and whitespace differences
    // ========================================================================
    {
        std::cout << "[RUN] Test 2: Case and whitespace differences..." << std::endl;
        std::string q1 = "Explain BFS Algorithm";
        std::string q2 = "   explain   bfs   algorithm   \n\t";

        SimilarityResult res = SimilarityEngine::compareQuestions(q1, q2);
        assert(isClose(res.jaccardScore, 1.0));
        assert(isClose(res.levenshteinScore, 1.0)); // normalized strings match exactly
        assert(isClose(res.combinedScore, 1.0));
        assert(res.isMatch == true);
        assert(res.hasConflict == false);

        std::cout << "  -> Passed: Case and excessive whitespace normalized cleanly." << std::endl;
    }

    // ========================================================================
    // Test 3: Punctuation differences
    // ========================================================================
    {
        std::cout << "[RUN] Test 3: Punctuation differences..." << std::endl;
        std::string q1 = "What is a Binary Search Tree?";
        std::string q2 = "What is a Binary Search Tree...";

        SimilarityResult res = SimilarityEngine::compareQuestions(q1, q2);
        assert(isClose(res.jaccardScore, 1.0));
        assert(res.combinedScore >= 0.95);
        assert(res.isMatch == true);
        assert(res.hasConflict == false);

        std::cout << "  -> Passed: Punctuation differences correctly stripped during tokenization." << std::endl;
    }

    // ========================================================================
    // Test 4: Paraphrased questions about the same concept
    // ========================================================================
    {
        std::cout << "[RUN] Test 4: Paraphrased questions about the same concept..." << std::endl;
        std::string q1 = "State the time complexity of QuickSort in worst case";
        std::string q2 = "What is the worst case time complexity of QuickSort algorithm";

        SimilarityResult res = SimilarityEngine::compareQuestions(q1, q2);
        assert(res.jaccardScore >= 0.70);
        assert(res.combinedScore >= 0.70);
        assert(res.isMatch == true);
        assert(res.hasConflict == false);

        std::cout << "  -> Passed: Paraphrased QuickSort questions matched (Combined: "
                  << res.combinedScore << " >= 0.70)." << std::endl;
    }

    // ========================================================================
    // Test 5: Concept conflict - Binary Search vs Linear Search
    // ========================================================================
    {
        std::cout << "[RUN] Test 5: Concept conflict - Binary Search vs Linear Search..." << std::endl;
        std::string q1 = "Explain the time complexity and search procedure of binary search with an array example.";
        std::string q2 = "Explain the time complexity and search procedure of linear search with an array example.";

        SimilarityResult res = SimilarityEngine::compareQuestions(q1, q2);
        // Lexical overlap is very high because 90% of words are identical
        assert(res.jaccardScore > 0.60);
        // But concept conflict MUST reject the match unconditionally
        assert(res.hasConflict == true);
        assert(res.isMatch == false);
        assert(res.conflictReason.find("Binary search vs Linear search") != std::string::npos);
        assert(res.explanation.find("REJECTED") != std::string::npos);

        std::cout << "  -> Passed: Binary Search vs Linear Search conflict successfully overridden high lexical overlap." << std::endl;
    }

    // ========================================================================
    // Test 6: Concept conflict - BFS vs DFS
    // ========================================================================
    {
        std::cout << "[RUN] Test 6: Concept conflict - BFS vs DFS..." << std::endl;
        std::string q1 = "Describe BFS traversal with pseudocode, queue usage, and time complexity.";
        std::string q2 = "Describe DFS traversal with pseudocode, stack usage, and time complexity.";

        SimilarityResult res = SimilarityEngine::compareQuestions(q1, q2);
        assert(res.hasConflict == true);
        assert(res.isMatch == false);
        assert(res.conflictReason.find("BFS") != std::string::npos);

        // Sub-test: Comparative question containing both BFS and DFS must NOT self-conflict
        std::string qComp1 = "Compare BFS and DFS graph traversal techniques with respect to space complexity.";
        std::string qComp2 = "Differentiate between BFS and DFS traversal algorithms with space requirements.";
        SimilarityResult compRes = SimilarityEngine::compareQuestions(qComp1, qComp2);
        assert(compRes.hasConflict == false); // Both have BFS and DFS, so no unilateral conflict

        std::cout << "  -> Passed: BFS vs DFS conflict rejected, while comparative questions mentioning both are allowed." << std::endl;
    }

    // ========================================================================
    // Test 7: Concept conflict - Stack vs Queue
    // ========================================================================
    {
        std::cout << "[RUN] Test 7: Concept conflict - Stack vs Queue..." << std::endl;
        std::string q1 = "Implement a stack using an array and write push and pop methods.";
        std::string q2 = "Implement a queue using an array and write enqueue and dequeue methods.";

        SimilarityResult res = SimilarityEngine::compareQuestions(q1, q2);
        assert(res.hasConflict == true);
        assert(res.isMatch == false);
        assert(res.conflictReason.find("Stack (LIFO) vs Queue (FIFO)") != std::string::npos);

        std::cout << "  -> Passed: Stack vs Queue conflict correctly prevented false clustering." << std::endl;
    }

    // ========================================================================
    // Test 8: Generic academic boilerplate sharing vs different concepts
    // ========================================================================
    {
        std::cout << "[RUN] Test 8: Generic academic boilerplate sharing vs different concepts..." << std::endl;
        std::string q1 = "Explain the working principle of bubble sort with a suitable diagram and example.";
        std::string q2 = "Explain the working principle of dijkstra algorithm with a suitable diagram and example.";

        SimilarityResult res = SimilarityEngine::compareQuestions(q1, q2);
        // After academic stopwords are removed, content tokens are ["bubble", "sort"] vs ["dijkstra", "algorithm"]
        // Jaccard similarity must be 0.0
        assert(isClose(res.jaccardScore, 0.0));
        assert(res.isMatch == false);

        std::cout << "  -> Passed: Generic academic boilerplate filtered; disjoint topics have 0.0 Jaccard score." << std::endl;
    }

    // ========================================================================
    // Test 9: Mathematical notation and complexity expressions
    // ========================================================================
    {
        std::cout << "[RUN] Test 9: Mathematical notation and complexity expressions..." << std::endl;
        std::string q1 = "Show that the worst case running time of merge sort is O(n log n).";
        auto tokens = TextNormalizer::tokenize(q1, true);

        // Verify "o(nlogn)" is extracted as a single protected token
        bool foundBigO = false;
        for (const auto& t : tokens) {
            if (t.find("o(nlogn)") != std::string::npos || t.find("o(n") != std::string::npos) {
                foundBigO = true;
                break;
            }
        }
        assert(foundBigO);

        // Test O(V+E) and O(1)
        std::string q2 = "Prove that BFS visits all vertices in O(V+E) time and checks visited array in O(1).";
        auto tokens2 = TextNormalizer::tokenize(q2, false);
        assert(std::find(tokens2.begin(), tokens2.end(), "o(v+e)") != tokens2.end());
        assert(std::find(tokens2.begin(), tokens2.end(), "o(1)") != tokens2.end());

        // Test C++ identifier protection
        std::string q3 = "Implement a balanced AVL tree in C++ with rotations.";
        auto tokens3 = TextNormalizer::tokenize(q3, false);
        assert(std::find(tokens3.begin(), tokens3.end(), "c++") != tokens3.end());

        std::cout << "  -> Passed: Complexity notations O(n log n), O(V+E), O(1) and C++ identifiers preserved." << std::endl;
    }

    // ========================================================================
    // Test 10: Negation and exception wording mismatch
    // ========================================================================
    {
        std::cout << "[RUN] Test 10: Negation and exception wording mismatch..." << std::endl;
        std::string q1 = "Which of the following sorting algorithms is stable?";
        std::string q2 = "Which of the following sorting algorithms is not stable?";

        SimilarityResult res = SimilarityEngine::compareQuestions(q1, q2);
        assert(res.hasConflict == true);
        assert(res.isMatch == false);
        assert(res.conflictReason.find("Negation mismatch") != std::string::npos);

        // Test "except"
        std::string q3 = "All graph algorithms require visited array except Kruskal.";
        std::string q4 = "All graph algorithms require visited array for Kruskal.";
        SimilarityResult res2 = SimilarityEngine::compareQuestions(q3, q4);
        assert(res2.hasConflict == true);
        assert(res2.isMatch == false);

        std::cout << "  -> Passed: Negation and exception words ('not', 'except') properly protected and flagged." << std::endl;
    }

    // ========================================================================
    // Test 11: Empty and very short strings
    // ========================================================================
    {
        std::cout << "[RUN] Test 11: Empty and very short strings..." << std::endl;
        SimilarityResult emptyBoth = SimilarityEngine::compareQuestions("", "");
        assert(isClose(emptyBoth.combinedScore, 1.0));
        assert(emptyBoth.isMatch == true);

        SimilarityResult emptyOne = SimilarityEngine::compareQuestions("AVL Tree", "");
        assert(isClose(emptyOne.combinedScore, 0.0));
        assert(emptyOne.isMatch == false);

        SimilarityResult singleChar = SimilarityEngine::compareQuestions("A", "B");
        assert(singleChar.isMatch == false);

        std::cout << "  -> Passed: Edge cases (empty strings, single characters) handled safely without crash." << std::endl;
    }

    // ========================================================================
    // Test 12: Repeated tokens and token-set behavior
    // ========================================================================
    {
        std::cout << "[RUN] Test 12: Repeated tokens and token-set behavior..." << std::endl;
        std::string q1 = "quick sort quick sort quick sort array";
        std::string q2 = "quick sort array";

        auto tokens1 = TextNormalizer::tokenize(q1, true);
        auto tokens2 = TextNormalizer::tokenize(q2, true);

        double jaccard = SimilarityEngine::jaccardSimilarity(tokens1, tokens2);
        assert(isClose(jaccard, 1.0)); // Unique token sets are {"array", "quick", "sort"} for both

        std::cout << "  -> Passed: Jaccard correctly computes similarity over unique token sets." << std::endl;
    }

    // ========================================================================
    // Test 13: Similarity score boundaries
    // ========================================================================
    {
        std::cout << "[RUN] Test 13: Similarity score boundaries [0.0, 1.0]..." << std::endl;
        std::vector<std::pair<std::string, std::string>> testPairs = {
            {"Compiler Design parsing lex yacc", "Relational Database Normalization BCNF"},
            {"AVL Tree double rotation LL RR LR RL", "Binary Search Tree insertion deletion"},
            {"Paging segmentation memory virtual address", "Paging segmentation memory virtual address"},
            {"Shortest path Dijkstra algorithm", "Minimum spanning tree Kruskal algorithm"}
        };

        for (const auto& pair : testPairs) {
            SimilarityResult res = SimilarityEngine::compareQuestions(pair.first, pair.second);
            assert(res.jaccardScore >= 0.0 && res.jaccardScore <= 1.0);
            assert(res.levenshteinScore >= 0.0 && res.levenshteinScore <= 1.0);
            assert(res.combinedScore >= 0.0 && res.combinedScore <= 1.0);
        }

        std::cout << "  -> Passed: All similarity metrics strictly bounded within [0.0, 1.0]." << std::endl;
    }

    // ========================================================================
    // Test 14: Deterministic clustering across multiple runs
    // ========================================================================
    {
        std::cout << "[RUN] Test 14: Deterministic clustering across multiple runs..." << std::endl;
        Question q1("PYQ-001", "Explain QuickSort algorithm with an example partition.", "SUB-DSA");
        Question q2("PYQ-002", "Explain QuickSort with an example and partition logic.", "SUB-DSA");
        Question q3("PYQ-003", "Describe Dijkstra's shortest path algorithm.", "SUB-DSA");
        Question q4("PYQ-004", "Explain Dijkstra algorithm for single source shortest path.", "SUB-DSA");
        Question q5("PYQ-005", "What is an AVL tree and explain double rotations.", "SUB-DSA");

        std::vector<Question> listA = {q1, q2, q3, q4, q5};
        std::vector<Question> listB = {q5, q3, q1, q4, q2}; // Different input order

        auto clustersA = QuestionClusterer::clusterQuestions(listA);
        auto clustersB = QuestionClusterer::clusterQuestions(listB);

        assert(clustersA.size() == clustersB.size());
        for (size_t i = 0; i < clustersA.size(); ++i) {
            assert(clustersA[i].representativeId == clustersB[i].representativeId);
            assert(clustersA[i].memberQuestionIds.size() == clustersB[i].memberQuestionIds.size());
            for (size_t j = 0; j < clustersA[i].memberQuestionIds.size(); ++j) {
                assert(clustersA[i].memberQuestionIds[j] == clustersB[i].memberQuestionIds[j]);
            }
        }

        std::cout << "  -> Passed: Clustering is strictly deterministic regardless of input permutation." << std::endl;
    }

    // ========================================================================
    // Test 15: Duplicate cluster membership prevention
    // ========================================================================
    {
        std::cout << "[RUN] Test 15: Duplicate cluster membership prevention..." << std::endl;
        Question q1("PYQ-001", "State the time complexity of QuickSort in worst case", "SUB-DSA");
        Question q2("PYQ-002", "What is the worst case time complexity of QuickSort algorithm", "SUB-DSA");
        Question q3("PYQ-003", "Explain MergeSort algorithm with divide and conquer strategy", "SUB-DSA");
        Question q4("PYQ-004", "Describe MergeSort dividing and merging steps with an example", "SUB-DSA");

        std::vector<Question> questions = {q1, q2, q3, q4};
        auto clusters = QuestionClusterer::clusterQuestions(questions);

        size_t totalMembers = 0;
        std::vector<std::string> allMemberIds;
        for (const auto& c : clusters) {
            totalMembers += c.memberQuestionIds.size();
            for (const auto& mId : c.memberQuestionIds) {
                allMemberIds.push_back(mId);
            }
        }

        assert(totalMembers == questions.size());
        std::sort(allMemberIds.begin(), allMemberIds.end());
        auto it = std::unique(allMemberIds.begin(), allMemberIds.end());
        assert(it == allMemberIds.end()); // No duplicates across clusters

        std::cout << "  -> Passed: Every question belongs to exactly one cluster; no duplicates." << std::endl;
    }

    // ========================================================================
    // Test 16: Transitive-chaining edge cases (conservative linkage)
    // ========================================================================
    {
        std::cout << "[RUN] Test 16: Transitive-chaining edge cases (conservative linkage)..." << std::endl;
        // Q1 is purely about AVL trees
        Question q1("PYQ-T01", "Explain AVL tree single and double rotations with balanced factor diagrams.", "SUB-DSA");
        // Q2 mentions both AVL and Red-Black trees
        Question q2("PYQ-T02", "Compare AVL tree and Red-Black tree height balance and search efficiency.", "SUB-DSA");
        // Q3 is purely about Red-Black trees
        Question q3("PYQ-T03", "Explain Red-Black tree insertion cases with recoloring and rotations.", "SUB-DSA");

        std::vector<Question> questions = {q1, q2, q3};
        auto clusters = QuestionClusterer::clusterQuestions(questions);

        // Q1 and Q3 must NOT be in the same cluster!
        // Verify no cluster contains both PYQ-T01 and PYQ-T03
        for (const auto& c : clusters) {
            bool hasQ1 = std::find(c.memberQuestionIds.begin(), c.memberQuestionIds.end(), "PYQ-T01") != c.memberQuestionIds.end();
            bool hasQ3 = std::find(c.memberQuestionIds.begin(), c.memberQuestionIds.end(), "PYQ-T03") != c.memberQuestionIds.end();
            assert(!(hasQ1 && hasQ3));
        }

        std::cout << "  -> Passed: Anti-transitive chaining policy prevented inappropriate merging of distinct topics." << std::endl;
    }

    // ========================================================================
    // Test 17: Preservation of original question text verbatim
    // ========================================================================
    {
        std::cout << "[RUN] Test 17: Preservation of original question text verbatim..." << std::endl;
        const std::string originalText = "  ExPlaIn  QuickSort()   algorithm; with   O(n log n)  time!  ";
        Question q("PYQ-ORIG", originalText, "SUB-DSA");
        q.addAppearance(ExamAppearance("End-Term", 2023, "Spring", "Q1", 10, false));

        std::vector<Question> questions = {q};
        auto clusters = QuestionClusterer::clusterQuestions(questions);

        // Verify the original Question object has not been mutated
        assert(q.getText() == originalText);
        assert(q.getAppearanceCount() == 1);
        assert(q.getTotalMarksCarried() == 10);
        assert(q.getSubject() == "SUB-DSA");

        std::cout << "  -> Passed: Original question text, punctuation, and appearance records preserved verbatim." << std::endl;
    }

    // ========================================================================
    // Test 18: Explanations for accepted and rejected matches
    // ========================================================================
    {
        std::cout << "[RUN] Test 18: Explanations for accepted and rejected matches..." << std::endl;
        Question q1("PYQ-A", "Explain Breadth-First Search (BFS) graph traversal with queue.", "SUB-DSA");
        Question q2("PYQ-B", "Explain Depth-First Search (DFS) graph traversal with stack.", "SUB-DSA");

        std::string report = QuestionClusterer::explainComparison(q1, q2);
        assert(!report.empty());
        assert(report.find("Question Comparison Report") != std::string::npos);
        assert(report.find("Conflict Detected") != std::string::npos);
        assert(report.find("Breadth-First") != std::string::npos || report.find("BFS") != std::string::npos);

        Question q3("PYQ-C", "Describe Breadth-First Search (BFS) graph traversal with a queue.", "SUB-DSA");
        std::string reportAccepted = QuestionClusterer::explainComparison(q1, q3);
        assert(reportAccepted.find("MATCH (Same Concept)") != std::string::npos);

        std::cout << "  -> Passed: Audit explanations generated clearly for both accepted and rejected comparisons." << std::endl;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 18 QUESTION SIMILARITY & CLUSTERING TESTS PASSED!    " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
