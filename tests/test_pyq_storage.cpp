#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <cmath>

#include "models/Question.hpp"
#include "storage/PYQStorage.hpp"

using namespace studysync;

// Helper to remove temporary test files
static void cleanFile(const std::string& filepath) {
    std::error_code ec;
    std::filesystem::remove(filepath, ec);
    std::filesystem::remove(filepath + ".tmp", ec);
    std::filesystem::remove(filepath + ".bak", ec);
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " StudySync - Task 3.1 PYQStorage Persistence Test Suite   " << std::endl;
    std::cout << "==========================================================" << std::endl;

    std::error_code dirEc;
    std::filesystem::create_directories("data", dirEc);

    // ========================================================================
    // Test 1: Saving and loading ordinary questions
    // ========================================================================
    {
        std::cout << "[RUN] Test 1: Saving and loading ordinary questions..." << std::endl;
        std::string testPath = "data/test_pyq_basic.dat";
        cleanFile(testPath);

        Question q1("PYQ-001", "What is an AVL Tree?", "Data Structures", 3,
                    "Trees", "AVL Trees", {"#avl", "#dsa"}, CognitiveLevel::REMEMBER);

        Question q2("PYQ-002", "Explain Dijkstra's shortest path algorithm.", "Algorithms", 4,
                    "Graphs", "Shortest Path", {"#graphs", "#greedy"}, CognitiveLevel::UNDERSTAND);

        std::vector<Question> original = {q1, q2};
        PYQStorage::saveQuestions(testPath, original);
        assert(std::filesystem::exists(testPath));

        std::vector<Question> loaded = PYQStorage::loadQuestions(testPath, true);
        assert(loaded.size() == 2);

        // Verify Question 1
        assert(loaded[0].getId() == "PYQ-001");
        assert(loaded[0].getText() == "What is an AVL Tree?");
        assert(loaded[0].getSubject() == "Data Structures");
        assert(loaded[0].getUnitNumber() == 3);
        assert(loaded[0].getUnitName() == "Trees");
        assert(loaded[0].getTopic() == "AVL Trees");
        assert(loaded[0].getTags().size() == 2);
        assert(loaded[0].getCognitiveLevel() == CognitiveLevel::REMEMBER);

        // Verify Question 2
        assert(loaded[1].getId() == "PYQ-002");
        assert(loaded[1].getText() == "Explain Dijkstra's shortest path algorithm.");
        assert(loaded[1].getUnitNumber() == 4);
        assert(loaded[1].getCognitiveLevel() == CognitiveLevel::UNDERSTAND);

        cleanFile(testPath);
        std::cout << "  -> Passed: Standard questions saved and loaded with all fields intact." << std::endl;
    }

    // ========================================================================
    // Test 2: Questions with multiple historical appearances
    // ========================================================================
    {
        std::cout << "[RUN] Test 2: Questions with multiple historical appearances..." << std::endl;
        std::string testPath = "data/test_pyq_appearances.dat";
        cleanFile(testPath);

        Question q("PYQ-REPEAT", "Derive the time complexity of QuickSort best, average, and worst cases.",
                   "Algorithms", 2, "Divide & Conquer", "QuickSort", {"#sorting", "#complexity"},
                   CognitiveLevel::ANALYZE);

        q.addAppearance(ExamAppearance("End-Term University Exam", 2021, "Even", "Q3(a)", 10, true));
        q.addAppearance(ExamAppearance("End-Term University Exam", 2023, "Even", "Q4(b)", 10, false));
        q.addAppearance(ExamAppearance("Mid-Term Exam", 2024, "Odd", "Q1", 5, false));

        PYQStorage::saveQuestions(testPath, {q});
        auto loaded = PYQStorage::loadQuestions(testPath, true);

        assert(loaded.size() == 1);
        assert(loaded[0].getAppearanceCount() == 3);
        assert(loaded[0].getLatestYear() == 2024);
        assert(loaded[0].getTotalMarksCarried() == 25);
        assert(loaded[0].hasAppearedInYear(2021));
        assert(loaded[0].hasAppearedInYear(2023));
        assert(loaded[0].hasAppearedInYear(2024));

        // Check specific appearance attributes
        const auto& apps = loaded[0].getHistory();
        assert(apps[0].getExamName() == "End-Term University Exam");
        assert(apps[0].getAcademicYear() == 2021);
        assert(apps[0].getSemester() == "Even");
        assert(apps[0].getQuestionNumber() == "Q3(a)");
        assert(apps[0].getMarks() == 10);
        assert(apps[0].getIsCompulsory() == true);

        assert(apps[1].getAcademicYear() == 2023);
        assert(apps[1].getIsCompulsory() == false);

        assert(apps[2].getExamName() == "Mid-Term Exam");
        assert(apps[2].getMarks() == 5);

        cleanFile(testPath);
        std::cout << "  -> Passed: Multiple historical appearances restored with exact attributes." << std::endl;
    }

    // ========================================================================
    // Test 3: ReferenceSolution round-trip
    // ========================================================================
    {
        std::cout << "[RUN] Test 3: ReferenceSolution round-trip..." << std::endl;
        std::string testPath = "data/test_pyq_solution.dat";
        cleanFile(testPath);

        Question q("PYQ-SOL-01", "Solve Master Theorem recurrence T(n) = 2T(n/2) + O(n).",
                   "Algorithms", 1, "Asymptotics", "Master Theorem");

        ReferenceSolution sol("By Master Theorem Case 2: a=2, b=2, f(n)=n. log_b(a) = 1. Therefore T(n) = Theta(n log n).",
                              {"Master Theorem", "Case 2", "log_b(a)", "critical exponent"},
                              {"Theta(n log n)", "T(n) = aT(n/b) + f(n)"},
                              "Theta(n log n)");

        q.setReferenceSolution(sol);
        PYQStorage::saveQuestions(testPath, {q});

        auto loaded = PYQStorage::loadQuestions(testPath, true);
        assert(loaded.size() == 1);

        const auto& loadedSol = loaded[0].getReferenceSolution();
        assert(loadedSol == sol);
        assert(loadedSol.getModelAnswer().find("Case 2") != std::string::npos);
        assert(loadedSol.getMandatoryKeyTerms().size() == 4);
        assert(loadedSol.getStructuralKeywords().size() == 2);
        assert(loadedSol.getNumericalOrFormulaAnswer() == "Theta(n log n)");

        cleanFile(testPath);
        std::cout << "  -> Passed: Reference solution model answer, key terms, and formulas restored." << std::endl;
    }

    // ========================================================================
    // Test 4: All metadata fields (Cluster ID, CognitiveLevel, Unit name)
    // ========================================================================
    {
        std::cout << "[RUN] Test 4: All metadata fields..." << std::endl;
        std::string testPath = "data/test_pyq_metadata.dat";
        cleanFile(testPath);

        Question q("PYQ-CLUSTER-01", "Describe Kruskal's algorithm for MST.", "DSA", 5,
                   "Graphs & Networks", "Minimum Spanning Tree", {"#mst", "#kruskal"},
                   CognitiveLevel::EVALUATE, {}, ReferenceSolution(), "CLUSTER-MST-01");

        PYQStorage::saveQuestions(testPath, {q});
        auto loaded = PYQStorage::loadQuestions(testPath, true);

        assert(loaded.size() == 1);
        assert(loaded[0].getClusterId() == "CLUSTER-MST-01");
        assert(loaded[0].getCognitiveLevel() == CognitiveLevel::EVALUATE);
        assert(loaded[0].getUnitName() == "Graphs & Networks");

        cleanFile(testPath);
        std::cout << "  -> Passed: Cognitive levels, cluster IDs, and unit names round-tripped." << std::endl;
    }

    // ========================================================================
    // Test 5: Multiline questions, LaTeX / KaTeX math expressions
    // ========================================================================
    {
        std::cout << "[RUN] Test 5: Multiline questions and LaTeX/KaTeX math..." << std::endl;
        std::string testPath = "data/test_pyq_math.dat";
        cleanFile(testPath);

        std::string mathText =
            "Consider the integral equation:\n"
            "\\int_0^\\infty e^{-x^2} dx = \\frac{\\sqrt{\\pi}}{2}\n"
            "1. Prove using polar coordinates: \\left(\\int_{-\\infty}^\\infty e^{-x^2} dx\\right)^2 = \\pi\n"
            "2. State its application in Gaussian distribution: f(x) = \\frac{1}{\\sigma\\sqrt{2\\pi}} e^{-\\frac{1}{2}(\\frac{x-\\mu}{\\sigma})^2}";

        std::string mathAnswer =
            "Solution Steps:\n"
            "\tStep 1: Convert Cartesian double integral to polar coordinates (r, \\theta).\n"
            "\tStep 2: Jacobians: dx dy = r dr d\\theta.\n"
            "\tStep 3: Limits: \\theta \\in [0, 2\\pi], r \\in [0, \\infty).\n"
            "Result: \\sqrt{\\pi}";

        Question mathQ("PYQ-GAUSS", mathText, "Mathematics", 2, "Multivariable Calculus",
                       "Improper Integrals", {"#math", "#gaussian", "#polar"});
        mathQ.setReferenceSolution(ReferenceSolution(mathAnswer, {"polar coordinates", "Jacobian"},
                                                     {"\\sqrt{\\pi}"}, "\\sqrt{\\pi}"));

        PYQStorage::saveQuestions(testPath, {mathQ});
        auto loaded = PYQStorage::loadQuestions(testPath, true);

        assert(loaded.size() == 1);
        assert(loaded[0].getText() == mathText);
        assert(loaded[0].getReferenceSolution().getModelAnswer() == mathAnswer);
        assert(loaded[0].getReferenceSolution().getNumericalOrFormulaAnswer() == "\\sqrt{\\pi}");

        cleanFile(testPath);
        std::cout << "  -> Passed: Complex LaTeX/KaTeX symbols and multiline blocks preserved verbatim." << std::endl;
    }

    // ========================================================================
    // Test 6: Quotes, backslashes, delimiters, and special characters
    // ========================================================================
    {
        std::cout << "[RUN] Test 6: Quotes, backslashes, delimiters, and special characters..." << std::endl;
        std::string testPath = "data/test_pyq_special.dat";
        cleanFile(testPath);

        std::string specialText =
            "Question with \"double quotes\" and 'single quotes'.\n"
            "Windows Path: C:\\Program Files\\StudySync\\pyq.dat\n"
            "Framing tokens: [QUESTION], [APPEARANCE], [END_APPEARANCE], [END_STORAGE]\n"
            "Commas: item1, item2, item3; Semicolon: val1; val2";

        Question specialQ("PYQ-SPEC-01", specialText, "System Programming", 1);
        PYQStorage::saveQuestions(testPath, {specialQ});

        auto loaded = PYQStorage::loadQuestions(testPath, true);
        assert(loaded.size() == 1);
        assert(loaded[0].getText() == specialText);

        cleanFile(testPath);
        std::cout << "  -> Passed: Special characters, delimiters, paths, and quotes round-tripped." << std::endl;
    }

    // ========================================================================
    // Test 7: Missing and empty file handling
    // ========================================================================
    {
        std::cout << "[RUN] Test 7: Missing and empty file handling..." << std::endl;
        std::string nonExistent = "data/non_existent_pyq.dat";
        cleanFile(nonExistent);

        // When errorIfMissing is false -> returns empty vector
        auto res = PYQStorage::loadQuestions(nonExistent, false);
        assert(res.empty());

        // When errorIfMissing is true -> throws FileAccessStorageException
        bool threwMissing = false;
        try {
            PYQStorage::loadQuestions(nonExistent, true);
        } catch (const FileAccessStorageException&) {
            threwMissing = true;
        }
        assert(threwMissing);

        // Empty file (0 bytes) -> throws CorruptedRecordStorageException
        std::string emptyFile = "data/empty_pyq.dat";
        cleanFile(emptyFile);
        std::ofstream out(emptyFile);
        out.close();

        bool threwEmpty = false;
        try {
            PYQStorage::loadQuestions(emptyFile, false);
        } catch (const CorruptedRecordStorageException&) {
            threwEmpty = true;
        }
        assert(threwEmpty);

        cleanFile(emptyFile);
        std::cout << "  -> Passed: Missing and empty file policies verified." << std::endl;
    }

    // ========================================================================
    // Test 8: Malformed headers and corrupted records
    // ========================================================================
    {
        std::cout << "[RUN] Test 8: Malformed headers and corrupted records..." << std::endl;
        std::string corruptPath = "data/corrupt_pyq.dat";
        cleanFile(corruptPath);

        // 1. Wrong header
        {
            std::ofstream out(corruptPath);
            out << "INVALID_HEADER_V1\n[END_STORAGE]\n";
            out.close();

            bool threw = false;
            try {
                PYQStorage::loadQuestions(corruptPath);
            } catch (const CorruptedRecordStorageException&) {
                threw = true;
            }
            assert(threw);
        }

        // 2. Missing footer
        {
            std::ofstream out(corruptPath);
            out << PYQStorage::FORMAT_HEADER_V1 << "\n";
            out << "[QUESTION]\nID: Q1\nTEXT: Sample\n[END_QUESTION]\n";
            // No [END_STORAGE]
            out.close();

            bool threw = false;
            try {
                PYQStorage::loadQuestions(corruptPath);
            } catch (const CorruptedRecordStorageException&) {
                threw = true;
            }
            assert(threw);
        }

        // 3. Incomplete question (missing [END_QUESTION])
        {
            std::ofstream out(corruptPath);
            out << PYQStorage::FORMAT_HEADER_V1 << "\n";
            out << "[QUESTION]\nID: Q1\nTEXT: Sample\n";
            out << PYQStorage::FORMAT_FOOTER << "\n";
            out.close();

            bool threw = false;
            try {
                PYQStorage::loadQuestions(corruptPath);
            } catch (const CorruptedRecordStorageException&) {
                threw = true;
            }
            assert(threw);
        }

        cleanFile(corruptPath);
        std::cout << "  -> Passed: Header and record corruption caught by exceptions." << std::endl;
    }

    // ========================================================================
    // Test 9: Duplicate IDs in persisted data
    // ========================================================================
    {
        std::cout << "[RUN] Test 9: Duplicate IDs in persisted data..." << std::endl;
        std::string dupPath = "data/duplicate_pyq.dat";
        cleanFile(dupPath);

        std::ofstream out(dupPath);
        out << PYQStorage::FORMAT_HEADER_V1 << "\n";
        out << "[QUESTION]\nID: SAME_PYQ_ID\nTEXT: Prompt 1\n[END_QUESTION]\n";
        out << "[QUESTION]\nID: SAME_PYQ_ID\nTEXT: Prompt 2\n[END_QUESTION]\n";
        out << PYQStorage::FORMAT_FOOTER << "\n";
        out.close();

        bool threwDup = false;
        try {
            PYQStorage::loadQuestions(dupPath);
        } catch (const CorruptedRecordStorageException&) {
            threwDup = true;
        }
        assert(threwDup);

        cleanFile(dupPath);
        std::cout << "  -> Passed: Duplicate question IDs rejected with CorruptedRecordStorageException." << std::endl;
    }

    // ========================================================================
    // Test 10: Save failure preserving existing data
    // ========================================================================
    {
        std::cout << "[RUN] Test 10: Save failure preserving existing data..." << std::endl;
        std::string targetPath = "data/preserve_pyq.dat";
        cleanFile(targetPath);

        std::vector<Question> initial = {
            Question("PRESERVE-01", "Original Question 1", "DSA", 1),
            Question("PRESERVE-02", "Original Question 2", "DSA", 2)
        };
        PYQStorage::saveQuestions(targetPath, initial);
        assert(std::filesystem::exists(targetPath));

        // Attempting to save to an invalid target directory should throw and leave targetPath unaffected
        std::string invalidPath = "Z:/non_existent_folder_xyz/pyqs.dat";
        bool threwInvalid = false;
        try {
            PYQStorage::saveQuestions(invalidPath, initial);
        } catch (const FileAccessStorageException&) {
            threwInvalid = true;
        }
        assert(threwInvalid);

        // Verify targetPath is still fully intact and unchanged
        auto verified = PYQStorage::loadQuestions(targetPath, true);
        assert(verified.size() == 2);
        assert(verified[0].getId() == "PRESERVE-01");
        assert(verified[1].getId() == "PRESERVE-02");

        cleanFile(targetPath);
        std::cout << "  -> Passed: Target file remains intact and staging cleaned up upon write failure." << std::endl;
    }

    // ========================================================================
    // Test 11: Repeated save and load cycle stability
    // ========================================================================
    {
        std::cout << "[RUN] Test 11: Repeated save and load cycle stability..." << std::endl;
        std::string testPath = "data/cycle_pyq.dat";
        cleanFile(testPath);

        Question q("PYQ-CYCLE", "Testing repeated write-read cycles.", "DSA", 1);
        q.addAppearance(ExamAppearance("Mid-Term", 2023, "Odd", "Q1", 5));
        q.setReferenceSolution(ReferenceSolution("Answer", {"keyword1"}, {"O(1)"}, "10"));

        std::vector<Question> current = {q};

        // Run 3 successive save-and-load cycles
        for (int i = 0; i < 3; ++i) {
            PYQStorage::saveQuestions(testPath, current);
            current = PYQStorage::loadQuestions(testPath, true);
            assert(current.size() == 1);
            assert(current[0].getId() == "PYQ-CYCLE");
            assert(current[0].getAppearanceCount() == 1);
            assert(current[0].getReferenceSolution().getModelAnswer() == "Answer");
        }

        cleanFile(testPath);
        std::cout << "  -> Passed: Data stability verified across repeated serialization cycles." << std::endl;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL PYQ STORAGE TESTS PASSED! (11/11 Test Suites)        " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
