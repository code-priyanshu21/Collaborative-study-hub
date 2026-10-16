#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

#include "models/Question.hpp"
#include "ds/SyllabusTree.hpp"
#include "algorithms/QuestionSimilarity.hpp"
#include "analytics/PYQAnalytics.hpp"
#include "analytics/PYQPredictor.hpp"

using namespace studysync;

bool isClose(double a, double b, double eps = 0.05) {
    return std::abs(a - b) <= eps;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " StudySync - Task 3.5 PYQPredictor Test Suite             " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // ========================================================================
    // Test 1: Empty question collection
    // ========================================================================
    {
        std::cout << "[RUN] Test 1: Empty question collection..." << std::endl;
        std::vector<Question> emptyQuestions;
        PredictorConfig config;

        auto qRank = PYQPredictor::rankQuestions(emptyQuestions, config);
        assert(qRank.empty());

        auto tRank = PYQPredictor::rankTopics(emptyQuestions, config);
        assert(tRank.empty());

        auto cRank = PYQPredictor::rankClusters(emptyQuestions, config);
        assert(cRank.empty());

        std::cout << "  -> Passed: Empty collections return safe empty outputs." << std::endl;
    }

    // ========================================================================
    // Test 2: Questions with no appearance history
    // ========================================================================
    {
        std::cout << "[RUN] Test 2: Questions with no appearance history..." << std::endl;
        Question q("PYQ-NONE", "Define an AVL tree and its balance factor.", "SUB-DSA", 2, "Trees", "AVL Trees");
        PredictorConfig config(2024);

        auto res = PYQPredictor::evaluateQuestion(q, config);
        assert(res.questionId == "PYQ-NONE");
        assert(res.totalAppearances == 0);
        assert(isClose(res.priorityScore, 0.0));
        assert(res.isInsufficientEvidence == true);
        assert(!res.insufficientEvidenceReason.empty());
        assert(res.explanation.find("Insufficient historical evidence") != std::string::npos);

        std::cout << "  -> Passed: Unappeared question receives 0.00 score and explicit insufficient evidence flag." << std::endl;
    }

    // ========================================================================
    // Test 3: A single recorded appearance
    // ========================================================================
    {
        std::cout << "[RUN] Test 3: A single recorded appearance..." << std::endl;
        Question q("PYQ-SINGLE", "Explain linear probing in hash tables.", "SUB-DSA", 1, "Hashing", "Hash Tables");
        q.addAppearance(ExamAppearance("End-Term Exam", 2024, "Spring", "Q3", 10, false));

        PredictorConfig config(2024);
        auto res = PYQPredictor::evaluateQuestion(q, config);

        assert(res.totalAppearances == 1);
        assert(res.distinctYearsCount == 1);
        assert(res.priorityScore > 0.0 && res.priorityScore <= 100.0);
        assert(res.isInsufficientEvidence == false);
        assert(res.components.frequencyScore > 0.0);
        assert(res.components.recencyScore > 0.0);

        std::cout << "  -> Passed: Single-appearance question scores appropriately (" << res.priorityScore << "/100.0)." << std::endl;
    }

    // ========================================================================
    // Test 4: Frequently recurring questions
    // ========================================================================
    {
        std::cout << "[RUN] Test 4: Frequently recurring questions..." << std::endl;
        Question q("PYQ-FREQ", "Explain Dijkstra algorithm with an example graph.", "SUB-DSA", 3, "Graphs", "Shortest Paths");
        q.addAppearance(ExamAppearance("End-Term", 2020, "Spring", "Q4", 10, false));
        q.addAppearance(ExamAppearance("End-Term", 2021, "Spring", "Q4", 10, false));
        q.addAppearance(ExamAppearance("End-Term", 2022, "Spring", "Q4", 10, false));
        q.addAppearance(ExamAppearance("End-Term", 2023, "Spring", "Q4", 10, false));
        q.addAppearance(ExamAppearance("End-Term", 2024, "Spring", "Q4", 10, false));

        PredictorConfig config(2024);
        auto res = PYQPredictor::evaluateQuestion(q, config);

        assert(res.totalAppearances == 5);
        assert(res.distinctYearsCount == 5);
        // Frequency and consistency should be saturated to 1.0
        assert(isClose(res.components.frequencyScore, 1.0));
        assert(isClose(res.components.consistencyScore, 1.0));
        assert(res.priorityScore >= 80.0); // Very high revision priority

        std::cout << "  -> Passed: Frequently recurring question achieves high priority score ("
                  << res.priorityScore << "/100.0)." << std::endl;
    }

    // ========================================================================
    // Test 5: Recent versus older appearances
    // ========================================================================
    {
        std::cout << "[RUN] Test 5: Recent versus older appearances..." << std::endl;
        // Question A: Appeared recently (2023, 2024)
        Question qRecent("PYQ-REC", "Explain QuickSort algorithm partition.", "SUB-DSA", 1, "Sorting", "QuickSort");
        qRecent.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));
        qRecent.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        // Question B: Appeared in older years (2016, 2017)
        Question qOld("PYQ-OLD", "Explain BubbleSort algorithm steps.", "SUB-DSA", 1, "Sorting", "BubbleSort");
        qOld.addAppearance(ExamAppearance("Exam", 2016, "Spring", "Q1", 10, false));
        qOld.addAppearance(ExamAppearance("Exam", 2017, "Spring", "Q1", 10, false));

        PredictorConfig config(2024);
        auto resRecent = PYQPredictor::evaluateQuestion(qRecent, config);
        auto resOld = PYQPredictor::evaluateQuestion(qOld, config);

        assert(resRecent.totalAppearances == resOld.totalAppearances); // Both 2 appearances
        assert(resRecent.components.recencyScore > resOld.components.recencyScore);
        assert(resRecent.priorityScore > resOld.priorityScore);

        std::cout << "  -> Passed: Recent question (" << resRecent.priorityScore
                  << ") ranks higher than older question (" << resOld.priorityScore << ")." << std::endl;
    }

    // ========================================================================
    // Test 6: Consistency across distinct years
    // ========================================================================
    {
        std::cout << "[RUN] Test 6: Consistency across distinct years..." << std::endl;
        // Question C: Appeared 3 times in ONE single academic year (2023)
        Question qConcentrated("PYQ-CONC", "Explain Prim's algorithm.", "SUB-DSA");
        qConcentrated.addAppearance(ExamAppearance("Mid-Term 1", 2023, "Spring", "Q1", 10, false));
        qConcentrated.addAppearance(ExamAppearance("Mid-Term 2", 2023, "Spring", "Q2", 10, false));
        qConcentrated.addAppearance(ExamAppearance("End-Term", 2023, "Autumn", "Q4", 10, false));

        // Question D: Appeared 3 times across 3 DISTINCT academic years (2021, 2022, 2023)
        Question qSpread("PYQ-SPREAD", "Explain Kruskal's algorithm.", "SUB-DSA");
        qSpread.addAppearance(ExamAppearance("End-Term", 2021, "Spring", "Q4", 10, false));
        qSpread.addAppearance(ExamAppearance("End-Term", 2022, "Spring", "Q4", 10, false));
        qSpread.addAppearance(ExamAppearance("End-Term", 2023, "Spring", "Q4", 10, false));

        PredictorConfig config(2024);
        auto resConc = PYQPredictor::evaluateQuestion(qConcentrated, config);
        auto resSpread = PYQPredictor::evaluateQuestion(qSpread, config);

        assert(resConc.totalAppearances == 3);
        assert(resSpread.totalAppearances == 3);
        assert(resConc.distinctYearsCount == 1);
        assert(resSpread.distinctYearsCount == 3);
        assert(resSpread.components.consistencyScore > resConc.components.consistencyScore);
        assert(resSpread.priorityScore > resConc.priorityScore);

        std::cout << "  -> Passed: Year consistency rewards question spread across distinct academic years ("
                  << resSpread.priorityScore << " vs " << resConc.priorityScore << ")." << std::endl;
    }

    // ========================================================================
    // Test 7: Equal scores and deterministic tie-breaking
    // ========================================================================
    {
        std::cout << "[RUN] Test 7: Equal scores and deterministic tie-breaking..." << std::endl;
        Question q1("PYQ-001", "Identical question A", "SUB-DSA");
        q1.addAppearance(ExamAppearance("End-Term", 2023, "Spring", "Q1", 10, false));

        Question q2("PYQ-002", "Identical question B", "SUB-DSA");
        q2.addAppearance(ExamAppearance("End-Term", 2023, "Spring", "Q1", 10, false));

        std::vector<Question> listA = {q2, q1};
        std::vector<Question> listB = {q1, q2};

        auto rankA = PYQPredictor::rankQuestions(listA, PredictorConfig(2024));
        auto rankB = PYQPredictor::rankQuestions(listB, PredictorConfig(2024));

        assert(rankA.size() == 2 && rankB.size() == 2);
        assert(rankA[0].questionId == "PYQ-001");
        assert(rankA[1].questionId == "PYQ-002");
        assert(rankB[0].questionId == "PYQ-001");
        assert(rankB[1].questionId == "PYQ-002");

        std::cout << "  -> Passed: Equal scores tie-broken deterministically by Question ID." << std::endl;
    }

    // ========================================================================
    // Test 8: Score bounds [0.0, 100.0]
    // ========================================================================
    {
        std::cout << "[RUN] Test 8: Score bounds [0.0, 100.0]..." << std::endl;
        std::vector<Question> testQuestions;

        // No appearances
        testQuestions.emplace_back("Q-EMPTY", "Text", "SUB");

        // 1 appearance in distant past
        Question qPast("Q-PAST", "Text", "SUB");
        qPast.addAppearance(ExamAppearance("Exam", 1995, "Spring", "Q1", 2, false));
        testQuestions.push_back(qPast);

        // 10 appearances with high marks
        Question qHuge("Q-HUGE", "Text", "SUB");
        for (int y = 2015; y <= 2024; ++y) {
            qHuge.addAppearance(ExamAppearance("Exam", y, "Spring", "Q1", 20, false));
        }
        testQuestions.push_back(qHuge);

        auto ranked = PYQPredictor::rankQuestions(testQuestions, PredictorConfig(2024));
        for (const auto& r : ranked) {
            assert(r.priorityScore >= 0.0);
            assert(r.priorityScore <= 100.0);
            assert(r.components.frequencyScore >= 0.0 && r.components.frequencyScore <= 1.0);
            assert(r.components.recencyScore >= 0.0 && r.components.recencyScore <= 1.0);
            assert(r.components.consistencyScore >= 0.0 && r.components.consistencyScore <= 1.0);
            assert(r.components.marksScore >= 0.0 && r.components.marksScore <= 1.0);
            assert(r.components.trendScore >= 0.0 && r.components.trendScore <= 1.0);
        }

        std::cout << "  -> Passed: Priority scores and components strictly bounded within valid ranges." << std::endl;
    }

    // ========================================================================
    // Test 9: Configurable weights
    // ========================================================================
    {
        std::cout << "[RUN] Test 9: Configurable weights..." << std::endl;
        // Question A: High frequency, low recency (many old appearances)
        Question qA("Q-A", "Topic A", "SUB");
        for (int y = 2010; y <= 2015; ++y) {
            qA.addAppearance(ExamAppearance("Exam", y, "Spring", "Q1", 10, false));
        }

        // Question B: Low frequency, high recency (one appearance in 2024)
        Question qB("Q-B", "Topic B", "SUB");
        qB.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        std::vector<Question> questions = {qA, qB};

        // Configuration 1: Heavy frequency weight
        PredictorConfig freqConfig(2024);
        freqConfig.weights = ScoringWeights(0.80, 0.05, 0.05, 0.05, 0.05);
        auto rankFreq = PYQPredictor::rankQuestions(questions, freqConfig);
        assert(rankFreq[0].questionId == "Q-A");

        // Configuration 2: Heavy recency weight
        PredictorConfig recConfig(2024);
        recConfig.weights = ScoringWeights(0.05, 0.80, 0.05, 0.05, 0.05);
        auto rankRec = PYQPredictor::rankQuestions(questions, recConfig);
        assert(rankRec[0].questionId == "Q-B");

        std::cout << "  -> Passed: Weight adjustments shift priorities according to user configuration." << std::endl;
    }

    // ========================================================================
    // Test 10: Invalid configuration handling
    // ========================================================================
    {
        std::cout << "[RUN] Test 10: Invalid configuration handling..." << std::endl;
        std::vector<Question> qList = {Question("Q-1", "Text", "SUB")};

        // Invalid year (< 1900)
        PredictorConfig badYear(1850);
        try {
            PYQPredictor::rankQuestions(qList, badYear);
            assert(false && "Should have thrown invalid_argument for year 1850");
        } catch (const std::invalid_argument& e) {
            assert(std::string(e.what()).find("Invalid reference") != std::string::npos);
        }

        // Invalid half-life (<= 0.0)
        PredictorConfig badHalfLife(2024, -2.0);
        try {
            PYQPredictor::rankQuestions(qList, badHalfLife);
            assert(false && "Should have thrown invalid_argument for negative half-life");
        } catch (const std::invalid_argument& e) {
            assert(std::string(e.what()).find("half-life") != std::string::npos);
        }

        // Invalid weights (all zeros)
        PredictorConfig badWeights(2024);
        badWeights.weights = ScoringWeights(0.0, 0.0, 0.0, 0.0, 0.0);
        try {
            PYQPredictor::rankQuestions(qList, badWeights);
            assert(false && "Should have thrown invalid_argument for all zero weights");
        } catch (const std::invalid_argument& e) {
            assert(std::string(e.what()).find("weights") != std::string::npos);
        }

        std::cout << "  -> Passed: Invalid configurations correctly rejected with std::invalid_argument." << std::endl;
    }

    // ========================================================================
    // Test 11: Explicit reference academic year
    // ========================================================================
    {
        std::cout << "[RUN] Test 11: Explicit reference academic year..." << std::endl;
        Question q("Q-REF", "Text", "SUB");
        q.addAppearance(ExamAppearance("Exam", 2020, "Spring", "Q1", 10, false));

        // When reference year is 2020: Delta = 0 -> recency = 0.50
        auto res2020 = PYQPredictor::evaluateQuestion(q, PredictorConfig(2020));
        assert(isClose(res2020.components.recencyScore, 0.50));

        // When reference year is 2024: Delta = 4 -> recency = 0.159
        auto res2024 = PYQPredictor::evaluateQuestion(q, PredictorConfig(2024));
        assert(res2024.components.recencyScore < res2020.components.recencyScore);

        std::cout << "  -> Passed: Explicit reference year parameter controls temporal decay deterministically." << std::endl;
    }

    // ========================================================================
    // Test 12: Insufficient evidence threshold
    // ========================================================================
    {
        std::cout << "[RUN] Test 12: Insufficient evidence threshold..." << std::endl;
        Question q("Q-MIN", "Text", "SUB");
        q.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        PredictorConfig strictConfig(2024);
        strictConfig.minAppearancesForScoring = 2; // Requires at least 2 appearances

        auto res = PYQPredictor::evaluateQuestion(q, strictConfig);
        assert(res.isInsufficientEvidence == true);
        assert(isClose(res.priorityScore, 0.0));

        std::cout << "  -> Passed: Questions below minAppearancesForScoring flagged as insufficient evidence." << std::endl;
    }

    // ========================================================================
    // Test 13: Topic aggregation
    // ========================================================================
    {
        std::cout << "[RUN] Test 13: Topic aggregation..." << std::endl;
        Question q1("Q-TOP-1", "Explain AVL Rotations.", "SUB-DSA", 2, "Trees", "AVL Trees");
        q1.addAppearance(ExamAppearance("Exam", 2022, "Spring", "Q1", 10, false));

        Question q2("Q-TOP-2", "Explain AVL Balance Factors.", "SUB-DSA", 2, "Trees", "AVL Trees");
        q2.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        std::vector<Question> questions = {q1, q2};
        auto topicRes = PYQPredictor::evaluateTopic("AVL Trees", questions, PredictorConfig(2024));

        assert(topicRes.topicName == "AVL Trees");
        assert(topicRes.totalQuestionsStored == 2);
        assert(topicRes.questionsWithEvidence == 2);
        assert(isClose(topicRes.evidenceCoverageRatio, 1.0));
        assert(topicRes.topicPriorityScore > 0.0);

        std::cout << "  -> Passed: Topic aggregation computes coverage, average score, and aggregate score." << std::endl;
    }

    // ========================================================================
    // Test 14: Topics with different numbers of stored questions (avoiding bias)
    // ========================================================================
    {
        std::cout << "[RUN] Test 14: Topics with different question counts (avoiding question-count bias)..." << std::endl;
        // Topic BigDormant: Has 6 questions stored, but each appeared once 8 years ago (dormant)
        std::vector<Question> questions;
        for (int i = 1; i <= 6; ++i) {
            Question q("Q-DORM-" + std::to_string(i), "Old text " + std::to_string(i), "SUB-DSA", 1, "Unit 1", "Dormant Topic");
            q.addAppearance(ExamAppearance("Exam", 2016, "Spring", "Q1", 5, false));
            questions.push_back(q);
        }

        // Topic SmallActive: Has only 2 questions stored, but both appear every year recently (2022, 2023, 2024)
        for (int i = 1; i <= 2; ++i) {
            Question q("Q-ACT-" + std::to_string(i), "Active text " + std::to_string(i), "SUB-DSA", 2, "Unit 2", "Active Topic");
            q.addAppearance(ExamAppearance("Exam", 2022, "Spring", "Q1", 10, false));
            q.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));
            q.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));
            questions.push_back(q);
        }

        auto rankedTopics = PYQPredictor::rankTopics(questions, PredictorConfig(2024));
        assert(rankedTopics.size() == 2);
        // Active Topic must rank FIRST despite having only 2 stored questions vs 6 stored questions!
        assert(rankedTopics[0].topicName == "Active Topic");
        assert(rankedTopics[1].topicName == "Dormant Topic");
        assert(rankedTopics[0].topicPriorityScore > rankedTopics[1].topicPriorityScore);

        std::cout << "  -> Passed: Question-count bias avoided: Active Topic ("
                  << rankedTopics[0].topicPriorityScore << ") ranks above Dormant Topic ("
                  << rankedTopics[1].topicPriorityScore << ")." << std::endl;
    }

    // ========================================================================
    // Test 15: Duplicate appearance records
    // ========================================================================
    {
        std::cout << "[RUN] Test 15: Duplicate appearance records..." << std::endl;
        Question q("Q-DUP", "Text", "SUB");
        q.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));
        // Add duplicate appearance
        std::vector<ExamAppearance> rawWithDup = {
            ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false),
            ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false)
        };
        q.setHistory(rawWithDup);

        auto res = PYQPredictor::evaluateQuestion(q, PredictorConfig(2024));
        assert(res.totalAppearances == 1); // Deduplicated to 1

        std::cout << "  -> Passed: Duplicate appearance records safely deduplicated." << std::endl;
    }

    // ========================================================================
    // Test 16: Multiple question subparts from one exam session
    // ========================================================================
    {
        std::cout << "[RUN] Test 16: Multiple question subparts from one exam session..." << std::endl;
        Question q("Q-SUBPARTS", "Text", "SUB");
        std::vector<ExamAppearance> subparts = {
            ExamAppearance("Exam", 2023, "Spring", "Q1(a)", 5, false),
            ExamAppearance("Exam", 2023, "Spring", "Q1(b)", 5, false)
        };
        q.setHistory(subparts);

        auto res = PYQPredictor::evaluateQuestion(q, PredictorConfig(2024));
        assert(res.totalAppearances == 2);
        assert(res.distinctSessionsCount == 1); // Exactly 1 session!

        std::cout << "  -> Passed: Subparts in same exam counted as 2 appearances in 1 distinct session." << std::endl;
    }

    // ========================================================================
    // Test 17: Similarity clusters integration
    // ========================================================================
    {
        std::cout << "[RUN] Test 17: Similarity clusters integration..." << std::endl;
        Question q1("PYQ-C1", "Explain Dijkstra's algorithm with single source shortest path.", "SUB-DSA");
        q1.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));

        Question q2("PYQ-C2", "Explain Dijkstra algorithm for single source shortest path graph.", "SUB-DSA");
        q2.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        std::vector<Question> questions = {q1, q2};
        auto clusterRanks = PYQPredictor::rankClusters(questions, PredictorConfig(2024));

        assert(!clusterRanks.empty());
        assert(clusterRanks[0].memberCount == 2);
        assert(clusterRanks[0].combinedAppearances == 2);

        // Also check question evaluation incorporates clusterId
        auto qRes = PYQPredictor::rankQuestions(questions, PredictorConfig(2024));
        assert(!qRes[0].clusterId.empty());
        assert(qRes[0].clusterSize == 2);

        std::cout << "  -> Passed: Paraphrase clusters identified and aggregated (Cluster with "
                  << clusterRanks[0].memberCount << " members, " << clusterRanks[0].combinedAppearances << " appearances)." << std::endl;
    }

    // ========================================================================
    // Test 18: Explanation and score consistency
    // ========================================================================
    {
        std::cout << "[RUN] Test 18: Explanation and score consistency..." << std::endl;
        Question q("Q-EXP", "Explain MergeSort.", "SUB-DSA");
        q.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));
        q.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        auto res = PYQPredictor::evaluateQuestion(q, PredictorConfig(2024));
        assert(!res.explanation.empty());
        assert(res.explanation.find("Revision Priority Score") != std::string::npos);
        assert(res.explanation.find("Repeat Frequency") != std::string::npos);
        assert(res.explanation.find("Recency Index") != std::string::npos);
        assert(res.explanation.find("NOT a calibrated probability") != std::string::npos);

        std::cout << "  -> Passed: Explanation matches numerical scores and includes non-probability disclaimer." << std::endl;
    }

    // ========================================================================
    // Test 19: Ranking reproducibility across input permutations
    // ========================================================================
    {
        std::cout << "[RUN] Test 19: Ranking reproducibility across input permutations..." << std::endl;
        Question q1("Q-1", "Sort 1", "SUB");
        q1.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        Question q2("Q-2", "Sort 2", "SUB");
        q2.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));
        q2.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        Question q3("Q-3", "Sort 3", "SUB");
        q3.addAppearance(ExamAppearance("Exam", 2021, "Spring", "Q1", 10, false));

        std::vector<Question> p1 = {q1, q2, q3};
        std::vector<Question> p2 = {q3, q1, q2};
        std::vector<Question> p3 = {q2, q3, q1};

        auto r1 = PYQPredictor::rankQuestions(p1, PredictorConfig(2024));
        auto r2 = PYQPredictor::rankQuestions(p2, PredictorConfig(2024));
        auto r3 = PYQPredictor::rankQuestions(p3, PredictorConfig(2024));

        for (size_t i = 0; i < 3; ++i) {
            assert(r1[i].questionId == r2[i].questionId);
            assert(r1[i].questionId == r3[i].questionId);
        }

        std::cout << "  -> Passed: Ranking sequence strictly identical across permutations." << std::endl;
    }

    // ========================================================================
    // Test 20: Filtering and result limits
    // ========================================================================
    {
        std::cout << "[RUN] Test 20: Filtering and result limits..." << std::endl;
        Question qDSA("Q-DSA", "Text", "SUB-DSA", 1, "Unit 1", "Arrays");
        qDSA.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        Question qOS("Q-OS", "Text", "SUB-OS", 1, "Unit 1", "Processes");
        qOS.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        std::vector<Question> questions = {qDSA, qOS};

        // Filter by subject
        PredictorConfig subConfig(2024);
        subConfig.subjectFilter = "SUB-DSA";
        auto subRank = PYQPredictor::rankQuestions(questions, subConfig);
        assert(subRank.size() == 1);
        assert(subRank[0].questionId == "Q-DSA");

        // Limit results
        PredictorConfig limitConfig(2024);
        limitConfig.resultLimit = 1;
        auto limitRank = PYQPredictor::rankQuestions(questions, limitConfig);
        assert(limitRank.size() == 1);

        std::cout << "  -> Passed: Filtering by subject and result limits function correctly." << std::endl;
    }

    // ========================================================================
    // Test 21: Missing data handling
    // ========================================================================
    {
        std::cout << "[RUN] Test 21: Missing data handling..." << std::endl;
        // Question with empty unitName, empty topic, 0 marks
        Question qBare("Q-BARE", "Minimal question text.", "SUB");
        qBare.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 0, false));

        auto res = PYQPredictor::evaluateQuestion(qBare, PredictorConfig(2024));
        assert(res.totalAppearances == 1);
        assert(res.totalMarks == 0);
        assert(isClose(res.averageMarks, 0.0));
        assert(res.priorityScore >= 0.0);

        std::cout << "  -> Passed: Bare question without marks or topic metadata handled safely without errors." << std::endl;
    }

    // ========================================================================
    // Test 22: Explainable comparative reports
    // ========================================================================
    {
        std::cout << "[RUN] Test 22: Explainable comparative reports..." << std::endl;
        Question qA("Q-A", "Question A", "SUB");
        qA.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));
        qA.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        Question qB("Q-B", "Question B", "SUB");
        qB.addAppearance(ExamAppearance("Exam", 2021, "Spring", "Q1", 5, false));

        auto resA = PYQPredictor::evaluateQuestion(qA, PredictorConfig(2024));
        auto resB = PYQPredictor::evaluateQuestion(qB, PredictorConfig(2024));

        std::string report = PYQPredictor::explainRankingComparison(resA, resB);
        assert(!report.empty());
        assert(report.find("Ranking Comparison Report") != std::string::npos);
        assert(report.find("HIGHER") != std::string::npos);
        assert(report.find("Q-A") != std::string::npos);

        std::cout << "  -> Passed: Ranking comparison generates transparent diagnostic report." << std::endl;
    }

    // ========================================================================
    // Test 23: Deterministic reference year derivation
    // ========================================================================
    {
        std::cout << "[RUN] Test 23: Deterministic reference year derivation..." << std::endl;
        std::vector<Question> emptySet;
        assert(PYQPredictor::deriveReferenceYear(emptySet) == PredictorConfig::DEFAULT_REFERENCE_YEAR);

        Question q1("Q-DERIV-1", "Text", "SUB");
        q1.addAppearance(ExamAppearance("Exam", 2021, "Spring", "Q1", 10, false));
        q1.addAppearance(ExamAppearance("Exam", 2026, "Spring", "Q1", 10, false));

        Question q2("Q-DERIV-2", "Text", "SUB");
        q2.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));

        std::vector<Question> dataset = {q1, q2};
        int derived = PYQPredictor::deriveReferenceYear(dataset);
        assert(derived == 2026); // Accurately extracted latest exam year

        std::cout << "  -> Passed: deriveReferenceYear extracts latest academic year (2026) deterministically." << std::endl;
    }

    // ========================================================================
    // Test 24: Mathematical precision audit of explanation component values
    // ========================================================================
    {
        std::cout << "[RUN] Test 24: Mathematical precision audit of explanation component values..." << std::endl;
        Question q("Q-MATH-AUDIT", "Explain Dijkstra algorithm with an example graph.", "SUB-DSA", 3, "Graphs", "Shortest Paths");
        q.addAppearance(ExamAppearance("End-Term", 2020, "Spring", "Q4", 10, false));
        q.addAppearance(ExamAppearance("End-Term", 2021, "Spring", "Q4", 10, false));
        q.addAppearance(ExamAppearance("End-Term", 2022, "Spring", "Q4", 10, false));
        q.addAppearance(ExamAppearance("End-Term", 2023, "Spring", "Q4", 10, false));
        q.addAppearance(ExamAppearance("End-Term", 2024, "Spring", "Q4", 10, false));

        PredictorConfig config(2024);
        auto res = PYQPredictor::evaluateQuestion(q, config);

        // Verify that recalculating from components matches priorityScore within 0.01
        const auto& c = res.components;
        const auto& w = res.effectiveWeights;
        double manualSum = (w.frequencyWeight * c.frequencyScore) +
                           (w.recencyWeight * c.recencyScore) +
                           (w.consistencyWeight * c.consistencyScore) +
                           (w.marksWeight * c.marksScore) +
                           (w.trendWeight * c.trendScore);
        double manualScore = std::round(manualSum * 10000.0) / 100.0;
        assert(isClose(res.priorityScore, manualScore, 0.001));
        assert(isClose(res.priorityScore, 92.39, 0.01));

        std::cout << "  -> Passed: Component calculation rigorously matches priority score (92.39 == "
                  << manualScore << ")." << std::endl;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 24 PYQ PREDICTOR TESTS PASSED!                       " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
