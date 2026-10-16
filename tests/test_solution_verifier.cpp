#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "models/Question.hpp"
#include "analytics/SolutionVerifier.hpp"

using namespace studysync;

bool isClose(double a, double b, double eps = 0.05) {
    return std::abs(a - b) <= eps;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " StudySync - Task 3.6 SolutionVerifier Test Suite         " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // ========================================================================
    // Test 1: Empty submitted answer
    // ========================================================================
    {
        std::cout << "[RUN] Test 1: Empty submitted answer..." << std::endl;
        ReferenceSolution ref(
            "QuickSort is a divide and conquer algorithm.",
            {"pivot", "partition", "recursion"},
            {"O(n log n)"},
            ""
        );
        Question q("Q-01", "Explain QuickSort.", "SUB-DSA", 1, "Sorting", "QuickSort", {}, CognitiveLevel::UNDERSTAND, {}, ref);

        auto res = SolutionVerifier::verifyAnswer(q, "");
        assert(res.overallScore == 0.0);
        assert(res.isInsufficientEvidence == true);
        assert(res.matchedKeyTerms.empty());
        assert(res.missingKeyTerms.size() == 3);
        assert(res.missingStructuralKeywords.size() == 1);
        assert(!res.summaryFeedback.empty());

        std::cout << "  -> Passed: Empty submitted answer correctly returns 0.0 score and lists all terms missing." << std::endl;
    }

    // ========================================================================
    // Test 2: Whitespace-only submitted answer
    // ========================================================================
    {
        std::cout << "[RUN] Test 2: Whitespace-only answer..." << std::endl;
        ReferenceSolution ref("BFS uses a queue.", {"queue", "breadth"}, {}, "");
        auto res = SolutionVerifier::verifyAnswer(ref, "   \t\n  \r  ", "Q-02");

        assert(res.overallScore == 0.0);
        assert(res.isInsufficientEvidence == true);
        assert(res.matchedKeyTerms.empty());
        assert(res.missingKeyTerms.size() == 2);

        std::cout << "  -> Passed: Whitespace-only answer handled safely as empty submission." << std::endl;
    }

    // ========================================================================
    // Test 3: Missing reference solution (empty ReferenceSolution)
    // ========================================================================
    {
        std::cout << "[RUN] Test 3: Missing reference solution..." << std::endl;
        Question qNoRef("Q-NO-REF", "What is an AVL Tree?", "SUB-DSA"); // Empty ReferenceSolution

        auto res = SolutionVerifier::verifyAnswer(qNoRef, "An AVL tree is a balanced binary search tree.");
        assert(res.hasReferenceSolution == false);
        assert(res.isInsufficientEvidence == true);
        assert(res.overallScore == 0.0);
        assert(res.insufficientEvidenceReason.find("No reference solution") != std::string::npos);

        std::cout << "  -> Passed: Missing reference solution flagged with explicit insufficient-evidence status." << std::endl;
    }

    // ========================================================================
    // Test 4: Empty keyword list (rubric still evaluates other active criteria)
    // ========================================================================
    {
        std::cout << "[RUN] Test 4: Empty keyword list..." << std::endl;
        // Only structural keyword and numerical answer defined
        ReferenceSolution ref("", {}, {"O(V+E)"}, "14");
        std::string submission = "The algorithm runs in O(V+E) time and the maximum flow is 14.";

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-04");
        assert(res.hasReferenceSolution == true);
        assert(res.matchedKeyTerms.empty());
        assert(res.missingKeyTerms.empty());
        assert(res.matchedStructuralKeywords.size() == 1);
        assert(res.numericalOrFormulaMatched == true);
        assert(isClose(res.overallScore, 100.0));

        std::cout << "  -> Passed: Empty keyword list redistributes weights to active criteria without error." << std::endl;
    }

    // ========================================================================
    // Test 5: Empty model answer rubric
    // ========================================================================
    {
        std::cout << "[RUN] Test 5: Empty model answer rubric..." << std::endl;
        ReferenceSolution ref("", {"binary search", "logarithmic"}, {"sorted array"}, "");
        std::string submission = "Binary search requires a sorted array and runs in logarithmic time.";

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-05");
        assert(res.hasReferenceSolution == true);
        assert(res.matchedKeyTerms.size() == 2);
        assert(res.matchedStructuralKeywords.size() == 1);
        assert(isClose(res.overallScore, 100.0));

        std::cout << "  -> Passed: Empty model answer redistributes weights cleanly to defined keywords." << std::endl;
    }

    // ========================================================================
    // Test 6: All required keywords present
    // ========================================================================
    {
        std::cout << "[RUN] Test 6: All required keywords present..." << std::endl;
        ReferenceSolution ref(
            "Dijkstra algorithm computes shortest paths from a single source vertex using a priority queue.",
            {"dijkstra", "shortest path", "priority queue", "relaxation"},
            {"greedy"},
            ""
        );
        std::string submission = "Dijkstra algorithm is a greedy method that computes the shortest path using a priority queue and edge relaxation.";

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-06");
        assert(res.matchedKeyTerms.size() == 4);
        assert(res.missingKeyTerms.empty());
        assert(res.matchedStructuralKeywords.size() == 1);
        assert(res.missingStructuralKeywords.empty());
        assert(res.keyTermScore == 100.0);
        assert(res.structuralScore == 100.0);
        assert(res.overallScore >= 90.0);

        std::cout << "  -> Passed: All keywords present yields full key term score and high overall score ("
                  << res.overallScore << "/100.0)." << std::endl;
    }

    // ========================================================================
    // Test 7: Some required keywords missing
    // ========================================================================
    {
        std::cout << "[RUN] Test 7: Some required keywords missing..." << std::endl;
        ReferenceSolution ref("", {"stack", "lifo", "push", "pop"}, {}, "");
        std::string submission = "A stack operates on LIFO principle where we push elements."; // 'pop' is missing!

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-07");
        assert(res.matchedKeyTerms.size() == 3);
        assert(res.missingKeyTerms.size() == 1);
        assert(res.missingKeyTerms[0] == "pop");
        assert(isClose(res.keyTermScore, 75.0)); // 3 of 4 = 75.0%

        std::cout << "  -> Passed: Missing keyword correctly identified (missing: 'pop', score: 75.0%)." << std::endl;
    }

    // ========================================================================
    // Test 8: Repeated keywords in submitted answer
    // ========================================================================
    {
        std::cout << "[RUN] Test 8: Repeated keywords in submitted answer..." << std::endl;
        ReferenceSolution ref("", {"tree", "root", "leaf"}, {}, "");
        // Student repeats "tree tree tree"
        std::string submission = "A tree tree tree has a root.";

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-08");
        assert(res.matchedKeyTerms.size() == 2); // 'tree', 'root'
        assert(res.missingKeyTerms.size() == 1); // 'leaf' is still missing
        // Score should be 2/3 = 66.67%, NOT inflated by repeating 'tree'
        assert(isClose(res.keyTermScore, 66.67));

        std::cout << "  -> Passed: Repeating keywords does not artificially inflate match counts." << std::endl;
    }

    // ========================================================================
    // Test 9: Case differences
    // ========================================================================
    {
        std::cout << "[RUN] Test 9: Case differences..." << std::endl;
        ReferenceSolution ref("", {"Breadth-First Search", "Queue", "FIFO"}, {}, "");
        std::string submission = "breadth-first search uses a queue with fifo order.";

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-09");
        assert(res.matchedKeyTerms.size() == 3);
        assert(res.missingKeyTerms.empty());
        assert(isClose(res.keyTermScore, 100.0));

        std::cout << "  -> Passed: Case differences normalized cleanly." << std::endl;
    }

    // ========================================================================
    // Test 10: Punctuation differences
    // ========================================================================
    {
        std::cout << "[RUN] Test 10: Punctuation differences..." << std::endl;
        ReferenceSolution ref("", {"divide and conquer", "pivot element"}, {}, "");
        std::string submission = "QuickSort uses 'divide and conquer' with a (pivot element).";

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-10");
        assert(res.matchedKeyTerms.size() == 2);
        assert(res.missingKeyTerms.empty());

        std::cout << "  -> Passed: Quotes and parentheses stripped safely during token sequence search." << std::endl;
    }

    // ========================================================================
    // Test 11: Partial-word false matches
    // ========================================================================
    {
        std::cout << "[RUN] Test 11: Partial-word false matches..." << std::endl;
        // Keyword is "sort"
        ReferenceSolution ref("", {"sort"}, {}, "");
        // Submission has "resort" and "assortment", but NOT the distinct word "sort"!
        std::string submission1 = "We visited a holiday resort with an assortment of gifts.";

        auto res1 = SolutionVerifier::verifyAnswer(ref, submission1, "Q-11");
        assert(res1.matchedKeyTerms.empty());
        assert(res1.missingKeyTerms.size() == 1);
        assert(res1.keyTermScore == 0.0);

        // Keyword is "tree", submission has "street"
        ReferenceSolution refTree("", {"tree"}, {}, "");
        std::string submission2 = "Walk across the street.";
        auto res2 = SolutionVerifier::verifyAnswer(refTree, submission2, "Q-11B");
        assert(res2.matchedKeyTerms.empty());
        assert(res2.missingKeyTerms.size() == 1);

        std::cout << "  -> Passed: Boundary-aware matching prevents partial-word false positives ('resort' != 'sort', 'street' != 'tree')." << std::endl;
    }

    // ========================================================================
    // Test 12: Technical terms and complexity notation
    // ========================================================================
    {
        std::cout << "[RUN] Test 12: Technical terms and complexity notation..." << std::endl;
        ReferenceSolution ref("", {"c++", "sql"}, {"O(n log n)", "O(1)"}, "");
        std::string submission = "Implemented in C++ and SQL with worst case time O(n log n) and auxiliary space O(1).";

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-12");
        assert(res.matchedKeyTerms.size() == 2);
        assert(res.matchedStructuralKeywords.size() == 2);
        assert(isClose(res.keyTermScore, 100.0));
        assert(isClose(res.structuralScore, 100.0));

        std::cout << "  -> Passed: Complexity notations O(n log n), O(1) and technical terms C++, SQL matched accurately." << std::endl;
    }

    // ========================================================================
    // Test 13: Formula reference where supported
    // ========================================================================
    {
        std::cout << "[RUN] Test 13: Formula reference where supported..." << std::endl;
        ReferenceSolution ref("", {"recurrence relation"}, {}, "T(n) = 2T(n/2) + O(n)");
        std::string submission = "By Master Theorem, the recurrence relation is T(n) = 2T(n/2) + O(n).";

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-13");
        assert(res.hasNumericalOrFormulaReference == true);
        assert(res.numericalOrFormulaMatched == true);
        assert(res.matchedKeyTerms.size() == 1);

        std::cout << "  -> Passed: Recurrence formula T(n) = 2T(n/2) + O(n) matched successfully." << std::endl;
    }

    // ========================================================================
    // Test 14: Numerical answer match and mismatch
    // ========================================================================
    {
        std::cout << "[RUN] Test 14: Numerical answer match and mismatch..." << std::endl;
        ReferenceSolution ref("", {"max flow"}, {}, "14");

        // Correct numerical submission
        std::string subCorrect = "The calculated max flow is 14.";
        auto resCorrect = SolutionVerifier::verifyAnswer(ref, subCorrect, "Q-14A");
        assert(resCorrect.hasNumericalOrFormulaReference == true);
        assert(resCorrect.numericalOrFormulaMatched == true);
        assert(isClose(resCorrect.numericalFormulaScore, 100.0));

        // Incorrect numerical submission
        std::string subWrong = "The calculated max flow is 28.";
        auto resWrong = SolutionVerifier::verifyAnswer(ref, subWrong, "Q-14B");
        assert(resWrong.hasNumericalOrFormulaReference == true);
        assert(resWrong.numericalOrFormulaMatched == false);
        assert(isClose(resWrong.numericalFormulaScore, 0.0));

        std::cout << "  -> Passed: Numerical answer correctly verified (14 matches, 28 rejected)." << std::endl;
    }

    // ========================================================================
    // Test 15: Missing numerical reference
    // ========================================================================
    {
        std::cout << "[RUN] Test 15: Missing numerical reference..." << std::endl;
        ReferenceSolution ref("A hash table uses hashing.", {"hash function"}, {}, "");
        std::string submission = "A hash table maps keys using a hash function.";

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-15");
        assert(res.hasNumericalOrFormulaReference == false);
        assert(res.numericalOrFormulaMatched == false);
        // Student is not penalized for absent numerical criteria
        assert(res.overallScore >= 80.0);

        std::cout << "  -> Passed: Non-numerical question does not penalize student for absent numerical answer." << std::endl;
    }

    // ========================================================================
    // Test 16: Score bounds [0.0, 100.0]
    // ========================================================================
    {
        std::cout << "[RUN] Test 16: Score bounds [0.0, 100.0]..." << std::endl;
        ReferenceSolution ref(
            "Model answer text with concepts.",
            {"term1", "term2", "term3"},
            {"step1", "step2"},
            "42"
        );

        std::vector<std::string> testAnswers = {
            "",
            "   ",
            "completely unrelated gibberish without matching words",
            "term1 step1 42",
            "term1 term2 term3 step1 step2 42 Model answer text with concepts."
        };

        for (const auto& ans : testAnswers) {
            auto res = SolutionVerifier::verifyAnswer(ref, ans, "Q-BOUNDS");
            assert(res.overallScore >= 0.0 && res.overallScore <= 100.0);
            assert(res.keyTermScore >= 0.0 && res.keyTermScore <= 100.0);
            assert(res.structuralScore >= 0.0 && res.structuralScore <= 100.0);
            assert(res.rubricCoverageScore >= 0.0 && res.rubricCoverageScore <= 100.0);
            assert(res.numericalFormulaScore >= 0.0 && res.numericalFormulaScore <= 100.0);
        }

        std::cout << "  -> Passed: All score components strictly bounded within [0.0, 100.0]." << std::endl;
    }

    // ========================================================================
    // Test 17: Deterministic repeated evaluation
    // ========================================================================
    {
        std::cout << "[RUN] Test 17: Deterministic repeated evaluation..." << std::endl;
        ReferenceSolution ref(
            "AVL trees perform LL, RR, LR, RL rotations to maintain height balance.",
            {"avl tree", "rotation", "balance factor"},
            {"O(log n)"},
            ""
        );
        std::string submission = "An AVL tree maintains balance factor using single and double rotation in O(log n) time.";

        auto res1 = SolutionVerifier::verifyAnswer(ref, submission, "Q-DET");
        for (int i = 0; i < 5; ++i) {
            auto res2 = SolutionVerifier::verifyAnswer(ref, submission, "Q-DET");
            assert(isClose(res1.overallScore, res2.overallScore));
            assert(res1.matchedKeyTerms == res2.matchedKeyTerms);
            assert(res1.missingKeyTerms == res2.missingKeyTerms);
            assert(res1.matchedStructuralKeywords == res2.matchedStructuralKeywords);
            assert(res1.missingStructuralKeywords == res2.missingStructuralKeywords);
        }

        std::cout << "  -> Passed: Evaluation outputs strictly identical across repeated runs." << std::endl;
    }

    // ========================================================================
    // Test 18: Preservation of original question and reference text
    // ========================================================================
    {
        std::cout << "[RUN] Test 18: Preservation of original question and reference text..." << std::endl;
        const std::string originalModel = "  Model Answer: QuickSort() with O(n log n).  ";
        const std::vector<std::string> originalTerms = {"QuickSort", "Divide and Conquer"};
        ReferenceSolution ref(originalModel, originalTerms, {"O(n log n)"}, "O(n log n)");

        Question q("Q-IMMUTABLE", "Original question text?", "SUB", 1, "Unit 1", "Topic", {}, CognitiveLevel::APPLY, {}, ref);

        auto res = SolutionVerifier::verifyAnswer(q, "quicksort divide and conquer o(n log n)");

        // Verify Question and ReferenceSolution objects are unmutated
        assert(q.getReferenceSolution().getModelAnswer() == originalModel);
        assert(q.getReferenceSolution().getMandatoryKeyTerms() == originalTerms);
        assert(q.getText() == "Original question text?");
        assert(res.overallScore > 0.0);

        std::cout << "  -> Passed: Question and ReferenceSolution objects remain immutable." << std::endl;
    }

    // ========================================================================
    // Test 19: Explanations consistent with computed scores
    // ========================================================================
    {
        std::cout << "[RUN] Test 19: Explanations consistent with computed scores..." << std::endl;
        ReferenceSolution ref(
            "MergeSort divides array into halves and merges them.",
            {"mergesort", "divide", "merge"},
            {"O(n log n)"},
            ""
        );
        std::string submission = "MergeSort will divide the input into two parts."; // missing 'merge' and 'O(n log n)'

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-EXP");
        assert(!res.summaryFeedback.empty());
        assert(res.summaryFeedback.find("Score:") != std::string::npos);

        bool foundMissingTerm = false;
        bool foundMissingStructural = false;
        for (const auto& fp : res.feedbackPoints) {
            if (fp.find("Missing: ['merge']") != std::string::npos) {
                foundMissingTerm = true;
            }
            if (fp.find("O(n log n)") != std::string::npos) {
                foundMissingStructural = true;
            }
        }
        assert(foundMissingTerm);
        assert(foundMissingStructural);
        assert(!res.disclaimer.empty());

        std::cout << "  -> Passed: Feedback strings transparently describe missing elements consistent with scores." << std::endl;
    }

    // ========================================================================
    // Test 20: Missing key terms and structural keywords lists correctly populated
    // ========================================================================
    {
        std::cout << "[RUN] Test 20: Missing key terms and structural lists..." << std::endl;
        ReferenceSolution ref("", {"alpha", "beta", "gamma"}, {"step1", "step2"}, "");
        std::string submission = "We have alpha and step2.";

        auto res = SolutionVerifier::verifyAnswer(ref, submission, "Q-20");
        assert(res.matchedKeyTerms == std::vector<std::string>({"alpha"}));
        assert(res.missingKeyTerms == std::vector<std::string>({"beta", "gamma"}));
        assert(res.matchedStructuralKeywords == std::vector<std::string>({"step2"}));
        assert(res.missingStructuralKeywords == std::vector<std::string>({"step1"}));

        std::cout << "  -> Passed: Matched and missing lists accurately populated." << std::endl;
    }

    // ========================================================================
    // PART 2: TASK 3.6.1 TARGETED NUMERICAL TOLERANCE TESTS (Tests 21 - 33)
    // ========================================================================

    // Test 21: Exact equality
    {
        std::cout << "[RUN] Test 21: Exact numerical equality..." << std::endl;
        assert(SolutionVerifier::isNumericallyClose(14.0, 14.0));
        assert(SolutionVerifier::isNumericallyClose(0.0, 0.0));
        assert(SolutionVerifier::isNumericallyClose(-273.15, -273.15));
        std::cout << "  -> Passed: Exact numerical equality verified." << std::endl;
    }

    // Test 22: Difference below absolute tolerance
    {
        std::cout << "[RUN] Test 22: Difference below absolute tolerance..." << std::endl;
        assert(SolutionVerifier::isNumericallyClose(10.0, 10.0005, 0.001, 0.001));
        assert(SolutionVerifier::isNumericallyClose(1.0, 0.9995, 0.001, 0.001));
        std::cout << "  -> Passed: Differences below absolute tolerance matched." << std::endl;
    }

    // Test 23: Difference exactly at allowed tolerance boundary
    {
        std::cout << "[RUN] Test 23: Difference exactly at allowed tolerance boundary..." << std::endl;
        assert(SolutionVerifier::isNumericallyClose(10.0, 10.001, 0.001, 0.00001));
        assert(SolutionVerifier::isNumericallyClose(10.0, 9.999, 0.001, 0.00001));
        std::cout << "  -> Passed: Boundary differences on tolerance threshold matched." << std::endl;
    }

    // Test 24: Difference above absolute tolerance
    {
        std::cout << "[RUN] Test 24: Difference above absolute tolerance..." << std::endl;
        assert(!SolutionVerifier::isNumericallyClose(10.0, 10.002, 0.001, 0.00001));
        assert(!SolutionVerifier::isNumericallyClose(10.0, 9.998, 0.001, 0.00001));
        std::cout << "  -> Passed: Differences strictly exceeding tolerance rejected." << std::endl;
    }

    // Test 25: Large values where relative tolerance applies
    {
        std::cout << "[RUN] Test 25: Large values where relative tolerance applies..." << std::endl;
        double largeVal = 1000000.0;
        double closeLarge = 1000500.0; // diff 500 <= relTol * maxMag (0.001 * 1000500 = 1000.5)
        double farLarge = 1002000.0;   // diff 2000 > 1002
        assert(SolutionVerifier::isNumericallyClose(closeLarge, largeVal, 0.001, 0.001));
        assert(!SolutionVerifier::isNumericallyClose(farLarge, largeVal, 0.001, 0.001));
        std::cout << "  -> Passed: Relative tolerance correctly governs large magnitude values." << std::endl;
    }

    // Test 26: Small values near zero
    {
        std::cout << "[RUN] Test 26: Small values near zero..." << std::endl;
        assert(SolutionVerifier::isNumericallyClose(0.0001, 0.0002, 0.001, 0.001));
        assert(!SolutionVerifier::isNumericallyClose(0.0001, 0.0002, 1e-5, 1e-5));
        std::cout << "  -> Passed: Near-zero values handled safely via absolute tolerance." << std::endl;
    }

    // Test 27: Positive and negative values
    {
        std::cout << "[RUN] Test 27: Positive and negative values..." << std::endl;
        assert(SolutionVerifier::isNumericallyClose(-14.0, -14.0005, 0.001, 0.001));
        assert(!SolutionVerifier::isNumericallyClose(-14.0, 14.0, 0.001, 0.001));
        assert(!SolutionVerifier::isNumericallyClose(14.0, -14.0, 0.001, 0.001));
        std::cout << "  -> Passed: Opposite signs rejected and negative numbers evaluated correctly." << std::endl;
    }

    // Test 28: Zero compared with zero
    {
        std::cout << "[RUN] Test 28: Zero compared with zero..." << std::endl;
        assert(SolutionVerifier::isNumericallyClose(0.0, 0.0, 0.001, 0.001));
        assert(SolutionVerifier::isNumericallyClose(0.0, -0.0, 0.001, 0.001));
        assert(SolutionVerifier::isNumericallyClose(-0.0, -0.0, 0.001, 0.001));
        std::cout << "  -> Passed: Zero and negative-zero equality verified." << std::endl;
    }

    // Test 29: Zero compared with small nonzero value
    {
        std::cout << "[RUN] Test 29: Zero compared with small nonzero value..." << std::endl;
        assert(SolutionVerifier::isNumericallyClose(0.0, 0.0005, 0.001, 0.001));
        assert(!SolutionVerifier::isNumericallyClose(0.0, 0.002, 0.001, 0.001));
        std::cout << "  -> Passed: Small nonzero offsets against zero checked accurately." << std::endl;
    }

    // Test 30: NaN and positive/negative infinity handling
    {
        std::cout << "[RUN] Test 30: NaN and positive/negative infinity handling..." << std::endl;
        double qnan = std::numeric_limits<double>::quiet_NaN();
        double pinf = std::numeric_limits<double>::infinity();
        double ninf = -std::numeric_limits<double>::infinity();

        assert(!SolutionVerifier::isNumericallyClose(qnan, 10.0));
        assert(!SolutionVerifier::isNumericallyClose(10.0, qnan));
        assert(!SolutionVerifier::isNumericallyClose(qnan, qnan));
        assert(!SolutionVerifier::isNumericallyClose(pinf, 10.0));
        assert(!SolutionVerifier::isNumericallyClose(10.0, pinf));
        assert(!SolutionVerifier::isNumericallyClose(pinf, pinf));
        assert(!SolutionVerifier::isNumericallyClose(ninf, -10.0));
        assert(!SolutionVerifier::isNumericallyClose(ninf, ninf));

        double dummy = 0.0;
        assert(!SolutionVerifier::tryParseNumber("nan", dummy));
        assert(!SolutionVerifier::tryParseNumber("NaN", dummy));
        assert(!SolutionVerifier::tryParseNumber("inf", dummy));
        assert(!SolutionVerifier::tryParseNumber("-infinity", dummy));
        std::cout << "  -> Passed: Non-finite inputs safely prevented from passing." << std::endl;
    }

    // Test 31: Invalid tolerance configuration throws std::invalid_argument
    {
        std::cout << "[RUN] Test 31: Invalid tolerance configuration validation..." << std::endl;
        double qnan = std::numeric_limits<double>::quiet_NaN();
        double pinf = std::numeric_limits<double>::infinity();

        bool threw1 = false;
        try { SolutionVerifier::isNumericallyClose(10.0, 10.0, -0.001, 0.001); } catch (const std::invalid_argument&) { threw1 = true; }
        assert(threw1);

        bool threw2 = false;
        try { SolutionVerifier::isNumericallyClose(10.0, 10.0, 0.001, -0.001); } catch (const std::invalid_argument&) { threw2 = true; }
        assert(threw2);

        bool threw3 = false;
        try { SolutionVerifier::isNumericallyClose(10.0, 10.0, qnan, 0.001); } catch (const std::invalid_argument&) { threw3 = true; }
        assert(threw3);

        bool threw4 = false;
        try { SolutionVerifier::isNumericallyClose(10.0, 10.0, 0.001, pinf); } catch (const std::invalid_argument&) { threw4 = true; }
        assert(threw4);

        std::cout << "  -> Passed: Invalid or negative tolerances correctly throw std::invalid_argument." << std::endl;
    }

    // Test 32: Deterministic repeated numerical comparisons
    {
        std::cout << "[RUN] Test 32: Deterministic repeated numerical comparisons..." << std::endl;
        bool first = SolutionVerifier::isNumericallyClose(1000500.0, 1000000.0, 0.001, 0.001);
        for (int i = 0; i < 500; ++i) {
            assert(SolutionVerifier::isNumericallyClose(1000500.0, 1000000.0, 0.001, 0.001) == first);
        }
        std::cout << "  -> Passed: Repeated numerical evaluation is strictly deterministic." << std::endl;
    }

    // Test 33: Explanatory feedback for numerical matches and mismatches
    {
        std::cout << "[RUN] Test 33: Explanatory feedback for numerical matches and mismatches..." << std::endl;
        ReferenceSolution refNum("", {"vertex"}, {}, "42.0");

        auto resMatch = SolutionVerifier::verifyAnswer(refNum, "The total count is 42.0 meters.", "Q-NUM-OK");
        assert(resMatch.numericalOrFormulaMatched);
        assert(resMatch.numericalFormulaScore == 100.0);
        bool foundNumDerivationNote = false;
        for (const auto& fp : resMatch.feedbackPoints) {
            if (fp.find("Intermediate mathematical derivation steps were not verified") != std::string::npos) {
                foundNumDerivationNote = true;
            }
        }
        assert(foundNumDerivationNote);

        auto resFail = SolutionVerifier::verifyAnswer(refNum, "The total count is 99.0 meters.", "Q-NUM-FAIL");
        assert(!resFail.numericalOrFormulaMatched);
        assert(resFail.numericalFormulaScore == 0.0);
        bool foundNumFailNote = false;
        for (const auto& fp : resFail.feedbackPoints) {
            if (fp.find("Expected numerical target '42.0' was not detected within allowed tolerance") != std::string::npos) {
                foundNumFailNote = true;
            }
        }
        assert(foundNumFailNote);
        std::cout << "  -> Passed: Clear derivation disclaimer and tolerance mismatch feedback produced." << std::endl;
    }

    // ========================================================================
    // PART 3: TASK 3.6.1 TARGETED FORMULA NORMALIZATION & COMPARISON TESTS (Tests 34 - 47)
    // ========================================================================

    // Test 34: Identical formulas
    {
        std::cout << "[RUN] Test 34: Identical formulas..." << std::endl;
        std::string detected;
        assert(SolutionVerifier::checkNumericalOrFormula("a + b", "a + b", detected));
        assert(SolutionVerifier::checkNumericalOrFormula("T(n) = 2T(n/2) + O(n)", "T(n) = 2T(n/2) + O(n)", detected));
        std::cout << "  -> Passed: Identical formulas match successfully." << std::endl;
    }

    // Test 35: Whitespace-only differences (Examples A & B)
    {
        std::cout << "[RUN] Test 35: Whitespace-only differences (Examples A & B)..." << std::endl;
        std::string det;
        assert(SolutionVerifier::checkNumericalOrFormula("a + b", "a+b", det)); // Example A
        assert(SolutionVerifier::checkNumericalOrFormula("2 * x", "2*x", det)); // Example B
        assert(SolutionVerifier::checkNumericalOrFormula("2*x", "  2   *   x  ", det));
        assert(SolutionVerifier::checkNumericalOrFormula("x / 2", "x/2", det));
        std::cout << "  -> Passed: Whitespace-only variations around operators match." << std::endl;
    }

    // Test 36: Different arithmetic operators (Example C)
    {
        std::cout << "[RUN] Test 36: Different arithmetic operators (Example C)..." << std::endl;
        std::string det;
        assert(!SolutionVerifier::checkNumericalOrFormula("a + b", "a - b", det)); // Example C
        assert(!SolutionVerifier::checkNumericalOrFormula("a * b", "a / b", det));
        assert(!SolutionVerifier::checkNumericalOrFormula("a + b", "a * b", det));
        std::cout << "  -> Passed: Different operators correctly rejected." << std::endl;
    }

    // Test 37: Different variables (Example E)
    {
        std::cout << "[RUN] Test 37: Different variables (Example E)..." << std::endl;
        std::string det;
        assert(!SolutionVerifier::checkNumericalOrFormula("a + b", "a + c", det)); // Example E
        assert(!SolutionVerifier::checkNumericalOrFormula("2 * x", "2 * y", det));
        assert(!SolutionVerifier::checkNumericalOrFormula("A + b", "a + b", det)); // Casing of distinct variables preserved
        std::cout << "  -> Passed: Variable differences and variable casing distinctions preserved." << std::endl;
    }

    // Test 38: Different numerical constants and divisors (Examples D & G)
    {
        std::cout << "[RUN] Test 38: Different numerical constants and divisors (Examples D & G)..." << std::endl;
        std::string det;
        assert(!SolutionVerifier::checkNumericalOrFormula("x / 2", "x / 3", det)); // Example D
        assert(!SolutionVerifier::checkNumericalOrFormula("2*x", "3*x", det));     // Example G
        assert(!SolutionVerifier::checkNumericalOrFormula("x/2", "x/20", det));    // Partial boundary false match prevented
        std::cout << "  -> Passed: Distinct numeric literals, divisors, and boundary extensions rejected." << std::endl;
    }

    // Test 39: Different parentheses and grouping (Example F)
    {
        std::cout << "[RUN] Test 39: Different parentheses and grouping (Example F)..." << std::endl;
        std::string det;
        assert(!SolutionVerifier::checkNumericalOrFormula("a * (b + c)", "a * b + c", det)); // Example F
        assert(!SolutionVerifier::checkNumericalOrFormula("a * b + c", "a * (b + c)", det));
        std::cout << "  -> Passed: Associativity and grouping differences correctly rejected." << std::endl;
    }

    // Test 40: Exponent notation
    {
        std::cout << "[RUN] Test 40: Exponent notation..." << std::endl;
        std::string det;
        assert(SolutionVerifier::checkNumericalOrFormula("x^2 + y^2", "x^2 + y^2", det));
        assert(SolutionVerifier::checkNumericalOrFormula("x^2 + y^2", "x^2+y^2", det));
        assert(!SolutionVerifier::checkNumericalOrFormula("x^2 + y^2", "x^3 + y^2", det));
        std::cout << "  -> Passed: Exponent notation preserved and evaluated." << std::endl;
    }

    // Test 41: Decimal notation
    {
        std::cout << "[RUN] Test 41: Decimal notation..." << std::endl;
        std::string det;
        assert(SolutionVerifier::checkNumericalOrFormula("3.14 * r", "3.14 * r", det));
        assert(SolutionVerifier::checkNumericalOrFormula("3.14 * r", "3.14*r", det));
        assert(!SolutionVerifier::checkNumericalOrFormula("3.14 * r", "3.1415 * r", det));
        std::cout << "  -> Passed: Decimal constants preserved in formulas." << std::endl;
    }

    // Test 42: Empty formulas (Example I)
    {
        std::cout << "[RUN] Test 42: Empty formulas (Example I)..." << std::endl;
        std::string det;
        assert(!SolutionVerifier::checkNumericalOrFormula("", "", det)); // Example I
        assert(!SolutionVerifier::checkNumericalOrFormula("a + b", "", det));
        assert(!SolutionVerifier::checkNumericalOrFormula("", "a + b", det));
        std::cout << "  -> Passed: Empty formulas do not match." << std::endl;
    }

    // Test 43: Whitespace-only formulas
    {
        std::cout << "[RUN] Test 43: Whitespace-only formulas..." << std::endl;
        std::string det;
        assert(!SolutionVerifier::checkNumericalOrFormula("   \t\n", "   ", det));
        assert(!SolutionVerifier::checkNumericalOrFormula("a + b", "    ", det));
        assert(!SolutionVerifier::checkNumericalOrFormula("    ", "a + b", det));
        std::cout << "  -> Passed: Whitespace-only formulas safely rejected." << std::endl;
    }

    // Test 44: Malformed or unsupported formula strings (Example J)
    {
        std::cout << "[RUN] Test 44: Malformed or unsupported formula strings (Example J)..." << std::endl;
        std::string det;
        assert(!SolutionVerifier::isWellFormedFormula("((a + b)")); // Unbalanced
        assert(!SolutionVerifier::isWellFormedFormula("a + b)"));   // Unbalanced
        assert(!SolutionVerifier::isWellFormedFormula("a + ()"));   // Empty parens
        assert(!SolutionVerifier::isWellFormedFormula("* x"));      // Dangling binary op at start
        assert(!SolutionVerifier::isWellFormedFormula("a + "));     // Dangling binary op at end
        assert(!SolutionVerifier::checkNumericalOrFormula("((a + b)", "((a + b)", det)); // Malformed rejected safely
        std::cout << "  -> Passed: Malformed formula expressions safely handled as unsupported mismatches." << std::endl;
    }

    // Test 45: Technical expressions and Big-O notation
    {
        std::cout << "[RUN] Test 45: Technical expressions and Big-O notation..." << std::endl;
        std::string det;
        assert(SolutionVerifier::checkNumericalOrFormula("T(n) = 2T(n/2) + O(n)", "T(n) = 2T(n/2) + O(n)", det));
        assert(SolutionVerifier::checkNumericalOrFormula("O(n log n)", "o(n log n)", det));
        assert(SolutionVerifier::checkNumericalOrFormula("O(n log n)", "O(nlogn)", det));
        assert(SolutionVerifier::checkNumericalOrFormula("sqrt(n)", "SQRT(n)", det));
        assert(SolutionVerifier::checkNumericalOrFormula("log(n)", "LOG(n)", det));
        std::cout << "  -> Passed: Standard CS asymptotic notations and functions normalized cleanly." << std::endl;
    }

    // Test 46: Feedback consistent with formula comparison result
    {
        std::cout << "[RUN] Test 46: Feedback consistent with formula comparison result..." << std::endl;
        ReferenceSolution refForm("", {"recurrence"}, {}, "T(n) = 2T(n/2) + O(n)");

        auto resFormOK = SolutionVerifier::verifyAnswer(refForm, "The recurrence is T(n) = 2T(n/2) + O(n) which is divide and conquer.", "Q-FORM-OK");
        assert(resFormOK.numericalOrFormulaMatched);
        bool foundFormNonProof = false;
        for (const auto& fp : resFormOK.feedbackPoints) {
            if (fp.find("not a proof of symbolic mathematical equivalence") != std::string::npos) {
                foundFormNonProof = true;
            }
        }
        assert(foundFormNonProof);

        auto resFormFail = SolutionVerifier::verifyAnswer(refForm, "The recurrence is T(n) = 3T(n/3) + O(n).", "Q-FORM-FAIL");
        assert(!resFormFail.numericalOrFormulaMatched);
        bool foundFormFailNote = false;
        for (const auto& fp : resFormFail.feedbackPoints) {
            if (fp.find("Expected formula or complexity target") != std::string::npos &&
                fp.find("was not detected") != std::string::npos) {
                foundFormFailNote = true;
            }
        }
        assert(foundFormFailNote);
        std::cout << "  -> Passed: Formula match feedback emphasizes structural checking over symbolic equivalence." << std::endl;
    }

    // Test 47: Preservation of original reference formulas
    {
        std::cout << "[RUN] Test 47: Preservation of original reference formulas..." << std::endl;
        ReferenceSolution originalRef("Model answer", {"term"}, {"struct"}, "3.14 * r^2");
        std::string origFormula = originalRef.getNumericalOrFormulaAnswer();
        auto resOrig = SolutionVerifier::verifyAnswer(originalRef, "3.14 * r^2");
        assert(originalRef.getNumericalOrFormulaAnswer() == origFormula);
        assert(originalRef.getNumericalOrFormulaAnswer() == "3.14 * r^2");
        assert(resOrig.numericalOrFormulaMatched);
        std::cout << "  -> Passed: ReferenceSolution object and formula string remain immutable." << std::endl;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 47 SOLUTION VERIFIER TESTS PASSED!                   " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
