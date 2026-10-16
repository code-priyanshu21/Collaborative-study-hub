#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>

#include "models/Question.hpp"

using namespace studysync;

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " StudySync - Task 3.1 Question Domain Model Test Suite    " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // ========================================================================
    // Test 1: Basic construction and metadata access
    // ========================================================================
    {
        std::cout << "[RUN] Test 1: Basic construction and metadata access..." << std::endl;
        Question q("PYQ-001", "Explain the working of AVL Tree with all 4 rotation cases.",
                   "Data Structures", 3, "Trees", "AVL Trees", {"#avl", "#trees", "#dsa"},
                   CognitiveLevel::ANALYZE);

        assert(q.getId() == "PYQ-001");
        assert(q.getText() == "Explain the working of AVL Tree with all 4 rotation cases.");
        assert(q.getSubject() == "Data Structures");
        assert(q.getUnitNumber() == 3);
        assert(q.getUnitName() == "Trees");
        assert(q.getTopic() == "AVL Trees");
        assert(q.getTags().size() == 3);
        assert(q.getCognitiveLevel() == CognitiveLevel::ANALYZE);
        assert(q.getAppearanceCount() == 0);
        assert(q.getLatestYear() == 0);
        assert(q.getTotalMarksCarried() == 0);
        assert(q.getClusterId().empty());
        std::cout << "  -> Passed: Basic construction and getters verified." << std::endl;
    }

    // ========================================================================
    // Test 2: Validation of required fields
    // ========================================================================
    {
        std::cout << "[RUN] Test 2: Validation of required fields..." << std::endl;

        // Empty Question ID
        bool threwEmptyId = false;
        try {
            Question q("", "Valid Question Text");
        } catch (const std::invalid_argument&) {
            threwEmptyId = true;
        }
        assert(threwEmptyId);

        // Whitespace Question ID
        bool threwWhitespaceId = false;
        try {
            Question q("   \t  ", "Valid Question Text");
        } catch (const std::invalid_argument&) {
            threwWhitespaceId = true;
        }
        assert(threwWhitespaceId);

        // Empty Question Text
        bool threwEmptyText = false;
        try {
            Question q("PYQ-002", "");
        } catch (const std::invalid_argument&) {
            threwEmptyText = true;
        }
        assert(threwEmptyText);

        // Negative Unit Number
        bool threwNegativeUnit = false;
        try {
            Question q("PYQ-003", "Text", "DSA", -1);
        } catch (const std::invalid_argument&) {
            threwNegativeUnit = true;
        }
        assert(threwNegativeUnit);

        std::cout << "  -> Passed: Required ID, text, and unit number constraints enforced." << std::endl;
    }

    // ========================================================================
    // Test 3: ExamAppearance validation
    // ========================================================================
    {
        std::cout << "[RUN] Test 3: ExamAppearance validation..." << std::endl;

        // Valid ExamAppearance
        ExamAppearance app1("End-Term University Exam", 2024, "Even", "Q4(a)", 10, true);
        assert(app1.getExamName() == "End-Term University Exam");
        assert(app1.getAcademicYear() == 2024);
        assert(app1.getSemester() == "Even");
        assert(app1.getQuestionNumber() == "Q4(a)");
        assert(app1.getMarks() == 10);
        assert(app1.getIsCompulsory() == true);

        // Empty Exam Name
        bool threwEmptyExam = false;
        try {
            ExamAppearance badApp("", 2024);
        } catch (const std::invalid_argument&) {
            threwEmptyExam = true;
        }
        assert(threwEmptyExam);

        // Invalid Year (< 1900 or > 2100)
        bool threwYearLow = false;
        try {
            ExamAppearance badApp("Mid-Term", 1850);
        } catch (const std::invalid_argument&) {
            threwYearLow = true;
        }
        assert(threwYearLow);

        bool threwYearHigh = false;
        try {
            ExamAppearance badApp("Mid-Term", 2150);
        } catch (const std::invalid_argument&) {
            threwYearHigh = true;
        }
        assert(threwYearHigh);

        // Negative Marks
        bool threwNegativeMarks = false;
        try {
            ExamAppearance badApp("Mid-Term", 2023, "Odd", "Q1", -5);
        } catch (const std::invalid_argument&) {
            threwNegativeMarks = true;
        }
        assert(threwNegativeMarks);

        std::cout << "  -> Passed: ExamAppearance input parameters thoroughly validated." << std::endl;
    }

    // ========================================================================
    // Test 4: Appearance addition and duplicate prevention
    // ========================================================================
    {
        std::cout << "[RUN] Test 4: Appearance addition and duplicate prevention..." << std::endl;
        Question q("PYQ-DUP-01", "What is QuickSort?", "DSA", 5);

        ExamAppearance app1("End-Term Exam", 2023, "Odd", "Q2(b)", 5, false);
        ExamAppearance app1Duplicate("End-Term Exam", 2023, "Odd", "Q2(b)", 5, false);

        assert(q.addAppearance(app1) == true);
        assert(q.getAppearanceCount() == 1);

        // Adding exact duplicate appearance must return false and not inflate count
        assert(q.addAppearance(app1Duplicate) == false);
        assert(q.getAppearanceCount() == 1);

        // Removing appearance
        assert(q.removeAppearance(app1) == true);
        assert(q.getAppearanceCount() == 0);
        assert(q.removeAppearance(app1) == false);

        std::cout << "  -> Passed: Duplicate examination appearances strictly prevented." << std::endl;
    }

    // ========================================================================
    // Test 5: Multiple appearances across different years and sessions
    // ========================================================================
    {
        std::cout << "[RUN] Test 5: Multiple appearances across different years and sessions..." << std::endl;
        Question q("PYQ-DIJKSTRA", "Describe Dijkstra's algorithm for single-source shortest paths.",
                   "Algorithms", 4, "Graphs", "Shortest Path");

        ExamAppearance app2021("End-Term University Exam", 2021, "Even", "Q5", 10, true);
        ExamAppearance app2023("End-Term University Exam", 2023, "Even", "Q3(a)", 10, false);
        ExamAppearance app2024_mid("Mid-Term Test", 2024, "Odd", "Q2", 5, false);
        ExamAppearance app2024_end("End-Term University Exam", 2024, "Even", "Q6", 15, true);

        assert(q.addAppearance(app2021));
        assert(q.addAppearance(app2023));
        assert(q.addAppearance(app2024_mid));
        assert(q.addAppearance(app2024_end));

        assert(q.getAppearanceCount() == 4);
        assert(q.hasAppearedInYear(2021));
        assert(q.hasAppearedInYear(2023));
        assert(q.hasAppearedInYear(2024));
        assert(!q.hasAppearedInYear(2022));

        std::cout << "  -> Passed: Multiple historical appearances across years and sessions recorded." << std::endl;
    }

    // ========================================================================
    // Test 6: Appearance count, latest year, and total marks calculation
    // ========================================================================
    {
        std::cout << "[RUN] Test 6: Appearance count, latest year, and total marks calculation..." << std::endl;
        Question q("PYQ-METRICS", "Define Heap Sort and analyze its time complexity.", "DSA", 2);

        assert(q.getAppearanceCount() == 0);
        assert(q.getLatestYear() == 0);
        assert(q.getTotalMarksCarried() == 0);
        assert(q.getAverageMarks() == 0.0);
        assert(!q.isFrequentlyAsked(2));

        q.addAppearance(ExamAppearance("Exam A", 2020, "Even", "Q1", 5));
        q.addAppearance(ExamAppearance("Exam B", 2023, "Odd", "Q4", 10));
        q.addAppearance(ExamAppearance("Exam C", 2022, "Even", "Q2", 15));

        assert(q.getAppearanceCount() == 3);
        assert(q.getLatestYear() == 2023); // Max year across appearances
        assert(q.getTotalMarksCarried() == 30); // 5 + 10 + 15
        assert(std::abs(q.getAverageMarks() - 10.0) < 0.001); // 30 / 3
        assert(q.isFrequentlyAsked(2) == true);
        assert(q.isFrequentlyAsked(4) == false);

        std::cout << "  -> Passed: Historical metrics derived accurately from appearance records." << std::endl;
    }

    // ========================================================================
    // Test 7: ReferenceSolution model and rubric fields
    // ========================================================================
    {
        std::cout << "[RUN] Test 7: ReferenceSolution model and rubric fields..." << std::endl;
        ReferenceSolution sol("AVL tree is a self-balancing BST with balance factor in {-1, 0, 1}.",
                              {"balance factor", "rotations", "height-balanced"},
                              {"O(log n)", "O(1) rotation"},
                              "h <= 1.44 log_2(n)");

        assert(!sol.empty());
        assert(sol.getModelAnswer().find("self-balancing BST") != std::string::npos);
        assert(sol.getMandatoryKeyTerms().size() == 3);
        assert(sol.getStructuralKeywords().size() == 2);
        assert(sol.getNumericalOrFormulaAnswer() == "h <= 1.44 log_2(n)");

        // Deduplication in key terms
        sol.addKeyTerm("rotations"); // duplicate
        assert(sol.getMandatoryKeyTerms().size() == 3);
        sol.addKeyTerm("AVL invariant");
        assert(sol.getMandatoryKeyTerms().size() == 4);

        Question q("PYQ-SOL", "AVL Question", "DSA", 3);
        q.setReferenceSolution(sol);
        assert(q.getReferenceSolution() == sol);

        std::cout << "  -> Passed: ReferenceSolution rubric and keyword structures verified." << std::endl;
    }

    // ========================================================================
    // Test 8: Value semantics, Rule of 5 copy and move isolation
    // ========================================================================
    {
        std::cout << "[RUN] Test 8: Value semantics, copy and move isolation..." << std::endl;
        Question qOriginal("PYQ-ORIG", "Original Question", "DSA", 1);
        qOriginal.addAppearance(ExamAppearance("Exam 2023", 2023, "Even", "Q1", 10));

        // Copy Construction
        Question qCopy(qOriginal);
        assert(qCopy.getId() == qOriginal.getId());
        assert(qCopy.getAppearanceCount() == 1);

        // Mutate copy
        qCopy.addAppearance(ExamAppearance("Exam 2024", 2024, "Odd", "Q2", 5));
        assert(qCopy.getAppearanceCount() == 2);
        assert(qOriginal.getAppearanceCount() == 1); // Isolation preserved

        // Move Construction
        Question qMoved(std::move(qCopy));
        assert(qMoved.getId() == "PYQ-ORIG");
        assert(qMoved.getAppearanceCount() == 2);

        // Move Assignment
        Question qTarget("PYQ-TEMP", "Temp", "Temp", 1);
        qTarget = std::move(qMoved);
        assert(qTarget.getId() == "PYQ-ORIG");
        assert(qTarget.getAppearanceCount() == 2);

        std::cout << "  -> Passed: Rule of 5 value semantics and memory independence verified." << std::endl;
    }

    // ========================================================================
    // Test 9: Preservation of original question text verbatim
    // ========================================================================
    {
        std::cout << "[RUN] Test 9: Preservation of original question text verbatim..." << std::endl;
        std::string rawVerbatimText =
            "Solve the recurrence relation:\n"
            "\tT(n) = 2T(n/2) + n \\log n\n"
            "Using Master Theorem or Akra-Bazzi method.\n"
            "State whether Case 1, 2, or 3 applies and justify \"Big-O\" bounds.";

        Question q("PYQ-VERBATIM", rawVerbatimText, "Algorithms", 1);
        assert(q.getText() == rawVerbatimText); // Must match character-for-character

        std::cout << "  -> Passed: Multiline, tabs, math notation, and quotes preserved verbatim." << std::endl;
    }

    // ========================================================================
    // Test 10: Operator overloading (operator==, operator!=, operator<, operator<<)
    // ========================================================================
    {
        std::cout << "[RUN] Test 10: Operator overloading..." << std::endl;
        Question q1("PYQ-A", "Question A", "Subject", 1);
        Question q2("PYQ-A", "Different text same ID", "Subject", 1);
        Question q3("PYQ-B", "Question B", "Subject", 2);

        // Identity based strictly on ID
        assert(q1 == q2);
        assert(q1 != q3);

        // operator<: Unit number takes priority, then frequency descending, then ID
        assert(q1 < q3); // Unit 1 < Unit 2

        std::cout << "  -> Passed: Operator overloading semantics verified." << std::endl;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL QUESTION MODEL TESTS PASSED! (10/10 Test Suites)     " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
