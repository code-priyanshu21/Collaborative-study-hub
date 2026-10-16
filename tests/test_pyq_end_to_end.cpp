#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "models/Question.hpp"
#include "storage/PYQStorage.hpp"
#include "ds/SyllabusTree.hpp"
#include "algorithms/QuestionSimilarity.hpp"
#include "analytics/PYQAnalytics.hpp"
#include "analytics/PYQPredictor.hpp"
#include "analytics/SolutionVerifier.hpp"

using namespace studysync;

// Helper to check floating-point proximity
static bool isClose(double a, double b, double eps = 0.01) {
    return std::abs(a - b) <= eps;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " StudySync - Task 3.7 Module 3 End-to-End Integration Test" << std::endl;
    std::cout << "==========================================================" << std::endl;

    const std::string tempStoragePath = "test_e2e_pyq_storage.txt";

    // Clean up any stale test file
    std::error_code ec;
    std::filesystem::remove(tempStoragePath, ec);
    std::filesystem::remove(tempStoragePath + ".tmp", ec);
    std::filesystem::remove(tempStoragePath + ".bak", ec);

    // ========================================================================
    // Scenario 1: Hierarchical Syllabus Structure Setup
    // ========================================================================
    std::cout << "[RUN] Scenario 1: Hierarchical Syllabus Structure Setup..." << std::endl;
    SyllabusTree syllabus;
    bool s1 = syllabus.addSubject("CS301", "Data Structures & Algorithms");
    assert(s1);

    bool u1 = syllabus.addUnit("CS301", 1, "Divide and Conquer", "U1");
    bool u2 = syllabus.addUnit("CS301", 2, "Trees & Balance", "U2");
    bool u3 = syllabus.addUnit("CS301", 3, "Graph Theory", "U3");
    assert(u1 && u2 && u3);

    bool t1 = syllabus.addTopic("U1", "T1", "Recurrence Relations");
    bool t2 = syllabus.addTopic("U1", "T2", "Master Theorem");
    bool t3 = syllabus.addTopic("U2", "T3", "AVL Rotations");
    bool t4 = syllabus.addTopic("U2", "T4", "Red-Black Trees");
    bool t5 = syllabus.addTopic("U3", "T5", "Shortest Paths");
    assert(t1 && t2 && t3 && t4 && t5);

    assert(syllabus.getSubjectCount() == 1);
    assert(syllabus.getUnitCount() == 3);
    assert(syllabus.getTopicCount() == 5);
    std::cout << "  -> Passed: Syllabus tree established with Subjects, Units, and Topics." << std::endl;

    // ========================================================================
    // Scenario 2: Question Creation with Appearances, Formulas, & Solutions
    // ========================================================================
    std::cout << "[RUN] Scenario 2: Question Creation & Model Validation..." << std::endl;

    // Q1: MergeSort recurrence with formula reference
    ReferenceSolution refQ1(
        "MergeSort is a divide and conquer sorting algorithm that recursively halves the array and merges them in O(n) time.",
        {"mergesort", "divide", "conquer", "merge"},
        {"divide array", "recursive calls", "O(n log n)"},
        "T(n) = 2T(n/2) + O(n)"
    );
    Question q1(
        "Q-101",
        "Explain MergeSort and derive its recurrence relation T(n) = 2T(n/2) + O(n).",
        "CS301", 1, "Divide and Conquer", "Recurrence Relations",
        {"#sorting", "#mergesort", "#dnc"},
        CognitiveLevel::ANALYZE,
        {},
        refQ1
    );
    // Add distinct historical appearances including subparts in the same academic year
    bool a1_1 = q1.addAppearance(ExamAppearance("End-Term University Exam", 2021, "EVEN", "Q1a", 5, false));
    bool a1_2 = q1.addAppearance(ExamAppearance("End-Term University Exam", 2021, "EVEN", "Q1b", 5, false)); // Subpart in same exam
    bool a1_3 = q1.addAppearance(ExamAppearance("End-Term University Exam", 2023, "EVEN", "Q2", 10, true));
    bool a1_4 = q1.addAppearance(ExamAppearance("Mid-Term Exam", 2024, "ODD", "Q3a", 8, false));
    assert(a1_1 && a1_2 && a1_3 && a1_4);
    assert(q1.getAppearanceCount() == 4);

    // Duplicate appearance prevention test
    bool dupAdded = q1.addAppearance(ExamAppearance("End-Term University Exam", 2021, "EVEN", "Q1a", 5, false));
    assert(!dupAdded);
    assert(q1.getAppearanceCount() == 4);

    // Q2: Near-variant of Q1 for similarity and conservative clustering
    Question q2(
        "Q-102",
        "Describe the MergeSort algorithm and analyze its recurrence relation T(n) = 2T(n/2) + O(n).",
        "CS301", 1, "Divide and Conquer", "Recurrence Relations",
        {"#sorting", "#mergesort"},
        CognitiveLevel::UNDERSTAND,
        { ExamAppearance("Supplementary Exam", 2022, "ODD", "Q2", 8, false) }
    );

    // Q3: AVL Tree balance factor and rotations (Dormant candidate: 2018, 2020)
    ReferenceSolution refQ3(
        "An AVL tree maintains height balance such that balance factor for every node is within -1, 0, or +1.",
        {"avl", "balance factor", "rotation"},
        {"height balance", "single rotation", "double rotation"},
        ""
    );
    Question q3(
        "Q-103",
        "What is an AVL tree? Explain LL, RR, LR, and RL rotations with balance factor criteria.",
        "CS301", 2, "Trees & Balance", "AVL Rotations",
        {"#trees", "#avl", "#balanced"},
        CognitiveLevel::UNDERSTAND,
        {
            ExamAppearance("End-Term University Exam", 2018, "EVEN", "Q4", 12, false),
            ExamAppearance("End-Term University Exam", 2020, "EVEN", "Q5", 12, false)
        },
        refQ3
    );

    // Q4: Direct paraphrase variant of Q3 for clustering
    Question q4(
        "Q-104",
        "Explain an AVL tree and describe LL, RR, LR, and RL rotations with balance factor criteria.",
        "CS301", 2, "Trees & Balance", "AVL Rotations",
        {"#trees", "#avl"},
        CognitiveLevel::UNDERSTAND,
        { ExamAppearance("Mid-Term Exam", 2019, "ODD", "Q3", 6, false) }
    );

    // Q5: Numerical Dijkstra question
    ReferenceSolution refQ5(
        "Dijkstra algorithm computes single source shortest paths in weighted graphs with non-negative edge weights.",
        {"dijkstra", "shortest path", "priority queue"},
        {"greedy choice", "relaxation"},
        "14.0" // Numerical target answer
    );
    Question q5(
        "Q-105",
        "Calculate the shortest path distance from vertex A to vertex F using Dijkstra algorithm.",
        "CS301", 3, "Graph Theory", "Shortest Paths",
        {"#graphs", "#dijkstra", "#shortestpath"},
        CognitiveLevel::APPLY,
        { ExamAppearance("End-Term University Exam", 2024, "EVEN", "Q6", 14, false) },
        refQ5
    );

    // Q6: Red-Black Tree question
    ReferenceSolution refQ6(
        "Red-Black tree properties ensure balanced search by node color invariants and rotations.",
        {"red-black", "coloring", "black height"},
        {"recoloring", "rotations"},
        ""
    );
    Question q6(
        "Q-106",
        "Explain Red-Black tree insertion cases and recoloring rules.",
        "CS301", 2, "Trees & Balance", "Red-Black Trees",
        {"#trees", "#redblack"},
        CognitiveLevel::ANALYZE,
        { ExamAppearance("End-Term University Exam", 2023, "EVEN", "Q5", 10, false) },
        refQ6
    );

    // Q7: Master Theorem question without reference solution (tests missing reference rubric)
    Question q7(
        "Q-107",
        "Apply Master Theorem to solve recurrence relations of the form T(n) = aT(n/b) + f(n).",
        "CS301", 1, "Divide and Conquer", "Master Theorem",
        {"#recurrence", "#mastertheorem"},
        CognitiveLevel::APPLY,
        {
            ExamAppearance("Mid-Term Exam", 2022, "ODD", "Q1", 6, false),
            ExamAppearance("End-Term University Exam", 2023, "EVEN", "Q1", 8, false)
        }
        // No ReferenceSolution provided
    );

    // Q8: Advanced theoretical question without examination appearances (tests zero historical evidence)
    Question q8(
        "Q-108",
        "Discuss quantum lower bounds on comparison-based sorting models.",
        "CS301", 1, "Divide and Conquer", "Recurrence Relations",
        {"#advanced", "#theory"},
        CognitiveLevel::REMEMBER
        // No appearances
    );

    std::vector<Question> originalQuestions = { q1, q2, q3, q4, q5, q6, q7, q8 };
    assert(originalQuestions.size() == 8);
    std::cout << "  -> Passed: 8 representative questions created with diverse cognitive levels and appearances." << std::endl;

    // ========================================================================
    // Scenario 3: File Persistence and Safe Reload Verification
    // ========================================================================
    std::cout << "[RUN] Scenario 3: File Persistence & Reload Fidelity..." << std::endl;

    PYQStorage::saveQuestions(tempStoragePath, originalQuestions);
    assert(std::filesystem::exists(tempStoragePath));

    std::vector<Question> loadedQuestions = PYQStorage::loadQuestions(tempStoragePath);
    assert(loadedQuestions.size() == 8);

    // Detailed field-by-field fidelity checks
    for (size_t i = 0; i < originalQuestions.size(); ++i) {
        const auto& orig = originalQuestions[i];
        const auto& loaded = loadedQuestions[i];

        assert(loaded.getId() == orig.getId());
        assert(loaded.getText() == orig.getText());
        assert(loaded.getSubject() == orig.getSubject());
        assert(loaded.getUnitNumber() == orig.getUnitNumber());
        assert(loaded.getUnitName() == orig.getUnitName());
        assert(loaded.getTopic() == orig.getTopic());
        assert(loaded.getCognitiveLevel() == orig.getCognitiveLevel());
        assert(loaded.getTags() == orig.getTags());
        assert(loaded.getAppearanceCount() == orig.getAppearanceCount());

        // Verify appearance records
        const auto& origHist = orig.getHistory();
        const auto& loadHist = loaded.getHistory();
        assert(origHist.size() == loadHist.size());
        for (size_t j = 0; j < origHist.size(); ++j) {
            assert(origHist[j].getExamName() == loadHist[j].getExamName());
            assert(origHist[j].getAcademicYear() == loadHist[j].getAcademicYear());
            assert(origHist[j].getSemester() == loadHist[j].getSemester());
            assert(origHist[j].getQuestionNumber() == loadHist[j].getQuestionNumber());
            assert(origHist[j].getMarks() == loadHist[j].getMarks());
            assert(origHist[j].getIsCompulsory() == loadHist[j].getIsCompulsory());
        }

        // Verify ReferenceSolution persistence
        const auto& origRef = orig.getReferenceSolution();
        const auto& loadRef = loaded.getReferenceSolution();
        assert(origRef.getModelAnswer() == loadRef.getModelAnswer());
        assert(origRef.getMandatoryKeyTerms() == loadRef.getMandatoryKeyTerms());
        assert(origRef.getStructuralKeywords() == loadRef.getStructuralKeywords());
        assert(origRef.getNumericalOrFormulaAnswer() == loadRef.getNumericalOrFormulaAnswer());
    }

    // Corrupted file / non-existent file resilience test
    bool threwNotFound = false;
    try {
        PYQStorage::loadQuestions("non_existent_pyq_file_12345.txt", true);
    } catch (const FileAccessStorageException&) {
        threwNotFound = true;
    }
    assert(threwNotFound);

    // Default errorIfMissing = false returns empty collection safely
    auto emptyLoaded = PYQStorage::loadQuestions("non_existent_pyq_file_12345.txt", false);
    assert(emptyLoaded.empty());

    std::cout << "  -> Passed: Stored questions reloaded with 100% field fidelity." << std::endl;

    // ========================================================================
    // Scenario 4: Syllabus Association and Inverted Indexing
    // ========================================================================
    std::cout << "[RUN] Scenario 4: Syllabus Association & Inverted Indexing..." << std::endl;

    // Associate loaded questions to syllabus topics
    syllabus.addQuestionToTopic("T1", "Q-101");
    syllabus.addQuestionToTopic("T1", "Q-102");
    syllabus.addQuestionToTopic("T1", "Q-108");
    syllabus.addQuestionToTopic("T2", "Q-107");
    syllabus.addQuestionToTopic("T3", "Q-103");
    syllabus.addQuestionToTopic("T3", "Q-104");
    syllabus.addQuestionToTopic("T4", "Q-106");
    syllabus.addQuestionToTopic("T5", "Q-105");

    // Verify aggregate retrieval through syllabus tree
    auto unit1Questions = syllabus.getQuestionsForUnit("U1");
    assert(unit1Questions.size() == 4); // Q-101, Q-102, Q-108, Q-107

    auto topic3Questions = syllabus.getQuestionsForTopic("T3");
    assert(topic3Questions.size() == 2); // Q-103, Q-104

    auto subjectQuestions = syllabus.getQuestionsForSubject("CS301");
    assert(subjectQuestions.size() == 8);

    // Build Inverted Index from reloaded questions
    PYQIndex index;
    for (const auto& q : loadedQuestions) {
        index.indexQuestion(q);
    }

    // Multi-criteria search verification
    auto subjMatches = index.searchBySubject("CS301");
    assert(subjMatches.size() == 8);

    auto unitMatches = index.searchByUnit(1);
    assert(unitMatches.size() == 4);

    auto tagMatches = index.searchByTag("#sorting");
    assert(tagMatches.size() == 2); // Q-101, Q-102

    auto yearMatches = index.searchByYear(2024);
    assert(yearMatches.size() == 2); // Q-101, Q-105

    auto examMatches = index.searchByExam("end-term university exam");
    assert(!examMatches.empty());

    std::cout << "  -> Passed: Inverted index and syllabus tree retrieve questions accurately across all criteria." << std::endl;

    // ========================================================================
    // Scenario 5: Conservative Question Clustering
    // ========================================================================
    std::cout << "[RUN] Scenario 5: Conservative Question Clustering..." << std::endl;

    auto clusters = QuestionClusterer::clusterQuestions(loadedQuestions);
    assert(!clusters.empty());

    // Q1 and Q2 should share a cluster (MergeSort variants)
    std::string clusterQ1 = "";
    std::string clusterQ2 = "";
    std::string clusterQ3 = "";
    std::string clusterQ4 = "";
    std::string clusterQ5 = "";

    for (const auto& c : clusters) {
        for (const auto& qId : c.memberQuestionIds) {
            if (qId == "Q-101") clusterQ1 = c.clusterId;
            if (qId == "Q-102") clusterQ2 = c.clusterId;
            if (qId == "Q-103") clusterQ3 = c.clusterId;
            if (qId == "Q-104") clusterQ4 = c.clusterId;
            if (qId == "Q-105") clusterQ5 = c.clusterId;
        }
    }

    assert(!clusterQ1.empty());
    assert(clusterQ1 == clusterQ2); // Q1 and Q2 in same cluster

    assert(!clusterQ3.empty());
    assert(clusterQ3 == clusterQ4); // Q3 and Q4 in same cluster

    assert(clusterQ1 != clusterQ3); // Sorting and AVL trees are separate
    assert(clusterQ1 != clusterQ5); // Sorting and Dijkstra are separate

    // Verify questions remained unmutated
    assert(loadedQuestions[0].getText() == originalQuestions[0].getText());

    std::cout << "  -> Passed: Conservative clustering correctly isolated similar vs conflicting concepts." << std::endl;

    // ========================================================================
    // Scenario 6: Frequency, Recency, and Recurrence Analytics
    // ========================================================================
    std::cout << "[RUN] Scenario 6: Frequency, Recency, & Recurrence Analytics..." << std::endl;

    const int refYear = 2024;
    auto statsQ1 = PYQAnalytics::analyzeQuestion(loadedQuestions[0], refYear);
    assert(statsQ1.questionId == "Q-101");
    assert(statsQ1.totalAppearances == 4);
    assert(statsQ1.distinctYearsCount == 3);
    assert(statsQ1.recencyScore > 0.5);

    auto statsQ3 = PYQAnalytics::analyzeQuestion(loadedQuestions[2], refYear);
    assert(statsQ3.questionId == "Q-103");
    assert(statsQ3.totalAppearances == 2);
    assert(statsQ3.latestYear == 2020);
    assert(statsQ3.trend.classification == TrendClassification::DORMANT); // Appeared 2018, 2020; absent for 4 years

    auto statsQ8 = PYQAnalytics::analyzeQuestion(loadedQuestions[7], refYear);
    assert(statsQ8.questionId == "Q-108");
    assert(statsQ8.totalAppearances == 0);
    assert(isClose(statsQ8.recencyScore, 0.0));
    assert(statsQ8.trend.classification == TrendClassification::INSUFFICIENT_DATA);

    std::cout << "  -> Passed: Analytics correctly identified recency scores, session counts, and trend dynamics." << std::endl;

    // ========================================================================
    // Scenario 7: Explainable PYQ Predictor Evaluation
    // ========================================================================
    std::cout << "[RUN] Scenario 7: Explainable PYQ Predictor Evaluation..." << std::endl;

    PredictorConfig config;
    config.referenceYear = 2024;
    config.recencyHalfLife = 2.0;
    config.weights = ScoringWeights(0.25, 0.30, 0.20, 0.10, 0.15);
    config.enableClusterAggregation = true;

    auto ranked = PYQPredictor::rankQuestions(loadedQuestions, config);
    assert(ranked.size() == 8);

    // Verify strictly descending order of priority score
    for (size_t i = 0; i + 1 < ranked.size(); ++i) {
        assert(ranked[i].priorityScore >= ranked[i + 1].priorityScore);
    }

    // Verify all priority scores and component scores are within [0.0, 100.0] or [0.0, 1.0]
    for (const auto& pred : ranked) {
        assert(pred.priorityScore >= 0.0 && pred.priorityScore <= 100.0);
        assert(pred.components.frequencyScore >= 0.0 && pred.components.frequencyScore <= 1.0);
        assert(pred.components.recencyScore >= 0.0 && pred.components.recencyScore <= 1.0);
        assert(pred.components.consistencyScore >= 0.0 && pred.components.consistencyScore <= 1.0);
        assert(pred.components.marksScore >= 0.0 && pred.components.marksScore <= 1.0);
        assert(pred.components.trendScore >= 0.0 && pred.components.trendScore <= 1.0);
        assert(!pred.explanation.empty());
    }

    // Verify deterministic repeat execution
    auto ranked2 = PYQPredictor::rankQuestions(loadedQuestions, config);
    assert(ranked.size() == ranked2.size());
    for (size_t i = 0; i < ranked.size(); ++i) {
        assert(ranked[i].questionId == ranked2[i].questionId);
        assert(isClose(ranked[i].priorityScore, ranked2[i].priorityScore, 0.001));
    }

    std::cout << "  -> Passed: PYQ Predictor generated deterministic, explainable examination priorities." << std::endl;

    // ========================================================================
    // Scenario 8: SolutionVerifier with Task 3.6 & 3.6.1 Improvements
    // ========================================================================
    std::cout << "[RUN] Scenario 8: SolutionVerifier with Numerical & Formula Improvements..." << std::endl;

    // 8A. Formula question verification with isolated boundary and recurrence formula
    const auto& qMerge = loadedQuestions[0]; // Q-101
    std::string studentFormulaAnswer =
        "MergeSort is a divide and conquer algorithm that will divide array into halves using recursive calls. "
        "The recurrence relation is T(n) = 2T(n/2) + O(n) which solves to O(n log n). "
        "We merge the subproblems to produce the sorted array.";

    auto resFormula = SolutionVerifier::verifyAnswer(qMerge, studentFormulaAnswer);
    assert(resFormula.hasReferenceSolution);
    assert(!resFormula.isInsufficientEvidence);
    assert(resFormula.numericalOrFormulaMatched);
    assert(resFormula.numericalFormulaScore == 100.0);
    assert(resFormula.keyTermScore == 100.0);
    assert(resFormula.structuralScore == 100.0);
    assert(resFormula.overallScore >= 85.0);
    assert(!resFormula.disclaimer.empty());

    // 8B. Numerical question verification using Task 3.6.1 combined absolute/relative tolerance
    const auto& qDijkstra = loadedQuestions[4]; // Q-105, target: "14.0"
    std::string studentNumericalCorrect =
        "Applying Dijkstra relaxation step-by-step from source A, the shortest path distance to F is 14.0005.";
    auto resNumOK = SolutionVerifier::verifyAnswer(qDijkstra, studentNumericalCorrect);
    assert(resNumOK.numericalOrFormulaMatched); // 14.0005 matches 14.0 under 0.001 tolerance!
    assert(resNumOK.numericalFormulaScore == 100.0);

    // Mismatched numerical answer
    std::string studentNumericalWrong =
        "Applying Dijkstra relaxation step-by-step from source A, the shortest path distance to F is 28.5.";
    auto resNumFail = SolutionVerifier::verifyAnswer(qDijkstra, studentNumericalWrong);
    assert(!resNumFail.numericalOrFormulaMatched);
    assert(resNumFail.numericalFormulaScore == 0.0);

    // 8C. Question with Missing Reference Solution
    const auto& qNoRef = loadedQuestions[6]; // Q-107
    auto resNoRef = SolutionVerifier::verifyAnswer(qNoRef, "T(n) = aT(n/b) + f(n)");
    assert(!resNoRef.hasReferenceSolution);
    assert(resNoRef.isInsufficientEvidence);
    assert(resNoRef.overallScore == 0.0);
    assert(!resNoRef.insufficientEvidenceReason.empty());

    // 8D. Empty / Whitespace submitted answer
    auto resEmpty = SolutionVerifier::verifyAnswer(qMerge, "   \t\n  ");
    assert(resEmpty.isInsufficientEvidence);
    assert(resEmpty.overallScore == 0.0);
    assert(!resEmpty.missingKeyTerms.empty());

    std::cout << "  -> Passed: SolutionVerifier verified formulas, numerical tolerances, missing rubrics, and feedback." << std::endl;

    // ========================================================================
    // Scenario 9: Cross-Component Consistency & Safety Cleanup
    // ========================================================================
    std::cout << "[RUN] Scenario 9: Cross-Component Consistency & Clean-up..." << std::endl;

    // Verify IDs remain completely consistent across all pipeline stages
    for (const auto& q : loadedQuestions) {
        std::string qId = q.getId();
        assert(!qId.empty());
        // Verify index contains it
        auto found = index.searchBySubject(q.getSubject());
        assert(std::find(found.begin(), found.end(), qId) != found.end());
    }

    // Clean up temporary test file
    std::filesystem::remove(tempStoragePath, ec);
    std::filesystem::remove(tempStoragePath + ".tmp", ec);
    std::filesystem::remove(tempStoragePath + ".bak", ec);
    assert(!std::filesystem::exists(tempStoragePath));

    std::cout << "  -> Passed: Temporary storage cleanly disposed without residual artifacts." << std::endl;

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL MODULE 3 END-TO-END INTEGRATION SCENARIOS PASSED!    " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
