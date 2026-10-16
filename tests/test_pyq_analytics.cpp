#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

#include "models/Question.hpp"
#include "ds/SyllabusTree.hpp"
#include "analytics/PYQAnalytics.hpp"

using namespace studysync;

// Helper to check floating-point proximity
bool isClose(double a, double b, double eps = 0.001) {
    return std::abs(a - b) <= eps;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " StudySync - Task 3.4 PYQAnalytics Test Suite             " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // ========================================================================
    // Test 1: Question with no appearance history
    // ========================================================================
    {
        std::cout << "[RUN] Test 1: Question with no appearance history..." << std::endl;
        Question q("PYQ-001", "Explain QuickSort algorithm with an example partition.", "SUB-DSA", 1, "Arrays", "Sorting");

        auto stats = PYQAnalytics::analyzeQuestion(q, 2024);
        assert(stats.questionId == "PYQ-001");
        assert(stats.totalAppearances == 0);
        assert(stats.distinctYearsCount == 0);
        assert(stats.distinctSessionsCount == 0);
        assert(stats.latestYear == 0);
        assert(stats.earliestYear == 0);
        assert(stats.totalMarks == 0);
        assert(isClose(stats.averageMarks, 0.0));
        assert(isClose(stats.recencyScore, 0.0));
        assert(stats.trend.classification == TrendClassification::INSUFFICIENT_DATA);
        assert(stats.trend.isSufficientData == false);
        assert(!stats.explanation.empty());

        std::cout << "  -> Passed: Unappeared question correctly assigned zero counts, 0.0 recency, and INSUFFICIENT_DATA." << std::endl;
    }

    // ========================================================================
    // Test 2: Question appearing once
    // ========================================================================
    {
        std::cout << "[RUN] Test 2: Question appearing once..." << std::endl;
        Question q("PYQ-002", "Explain Dijkstra algorithm for shortest paths.", "SUB-DSA", 3, "Graphs", "Shortest Paths");
        q.addAppearance(ExamAppearance("End-Term University Exam", 2024, "Spring", "Q4(a)", 10, false));

        auto stats = PYQAnalytics::analyzeQuestion(q, 2024);
        assert(stats.totalAppearances == 1);
        assert(stats.distinctYearsCount == 1);
        assert(stats.distinctSessionsCount == 1);
        assert(stats.latestYear == 2024);
        assert(stats.earliestYear == 2024);
        assert(stats.totalMarks == 10);
        assert(isClose(stats.averageMarks, 10.0));
        // Recency score for 1 appearance in reference year: 1 - 2^(-1.0) = 0.50
        assert(isClose(stats.recencyScore, 0.50));
        // A single appearance cannot infer a multi-year recurrence trend
        assert(stats.trend.classification == TrendClassification::INSUFFICIENT_DATA);
        assert(stats.trend.isSufficientData == false);

        std::cout << "  -> Passed: Single-appearance question has exact 0.50 recency in reference year and INSUFFICIENT_DATA trend." << std::endl;
    }

    // ========================================================================
    // Test 3: Multiple appearances in one academic year
    // ========================================================================
    {
        std::cout << "[RUN] Test 3: Multiple appearances in one academic year..." << std::endl;
        Question q("PYQ-003", "Describe AVL tree rotations.", "SUB-DSA", 2, "Trees", "AVL Trees");
        // Appeared twice in 2023: Mid-Term and End-Term
        q.addAppearance(ExamAppearance("Mid-Term Exam", 2023, "Spring", "Q2", 5, false));
        q.addAppearance(ExamAppearance("End-Term Exam", 2023, "Autumn", "Q5", 10, true));

        auto stats = PYQAnalytics::analyzeQuestion(q, 2024);
        assert(stats.totalAppearances == 2);
        assert(stats.distinctYearsCount == 1); // Exactly 1 distinct academic year
        assert(stats.distinctSessionsCount == 2); // 2 distinct exam sessions
        assert(stats.latestYear == 2023);
        assert(stats.earliestYear == 2023);
        assert(stats.totalMarks == 15);
        assert(isClose(stats.averageMarks, 7.50));
        // Single academic year cannot establish a multi-year trend
        assert(stats.trend.classification == TrendClassification::INSUFFICIENT_DATA);
        assert(stats.trend.isSufficientData == false);

        std::cout << "  -> Passed: Multiple appearances within the same year not confused with multi-year recurrence." << std::endl;
    }

    // ========================================================================
    // Test 4: Appearances across distinct years
    // ========================================================================
    {
        std::cout << "[RUN] Test 4: Appearances across distinct years..." << std::endl;
        Question q("PYQ-004", "State the properties of B-Trees.", "SUB-DSA", 2, "Trees", "B-Trees");
        q.addAppearance(ExamAppearance("End-Term Exam", 2021, "Spring", "Q3", 10, false));
        q.addAppearance(ExamAppearance("End-Term Exam", 2022, "Spring", "Q3", 10, false));
        q.addAppearance(ExamAppearance("End-Term Exam", 2024, "Spring", "Q4", 10, false));

        auto stats = PYQAnalytics::analyzeQuestion(q, 2024);
        assert(stats.totalAppearances == 3);
        assert(stats.distinctYearsCount == 3);
        assert(stats.distinctSessionsCount == 3);
        assert(stats.latestYear == 2024);
        assert(stats.earliestYear == 2021);
        assert(stats.distinctYears == std::vector<int>({2021, 2022, 2024}));

        std::cout << "  -> Passed: Distinct years accurately extracted and sorted (2021, 2022, 2024)." << std::endl;
    }

    // ========================================================================
    // Test 5: Duplicate appearance records
    // ========================================================================
    {
        std::cout << "[RUN] Test 5: Duplicate appearance records..." << std::endl;
        std::vector<ExamAppearance> rawApps = {
            ExamAppearance("End-Term", 2023, "Spring", "Q1", 10, false),
            ExamAppearance("End-Term", 2023, "Spring", "Q1", 10, false), // Duplicate
            ExamAppearance("Mid-Term", 2022, "Spring", "Q2", 5, false)
        };

        auto clean = PYQAnalytics::deduplicateAppearances(rawApps);
        assert(clean.size() == 2);

        double score = PYQAnalytics::calculateRecencyScore(rawApps, 2024);
        double cleanScore = PYQAnalytics::calculateRecencyScore(clean, 2024);
        assert(isClose(score, cleanScore)); // Deduplication ensures duplicate does not inflate score

        std::cout << "  -> Passed: Duplicate appearance records identified and safely prevented from inflating metrics." << std::endl;
    }

    // ========================================================================
    // Test 6: Different examination sessions
    // ========================================================================
    {
        std::cout << "[RUN] Test 6: Different examination sessions..." << std::endl;
        std::vector<ExamAppearance> apps = {
            ExamAppearance("End-Term", 2023, "Spring", "Q1(a)", 5, false),
            ExamAppearance("End-Term", 2023, "Spring", "Q1(b)", 5, false), // Same session, sub-part
            ExamAppearance("End-Term", 2023, "Autumn", "Q3", 10, false),   // Different session
            ExamAppearance("Mid-Term", 2023, "Spring", "Q1", 5, false)     // Different session
        };

        auto sessions = PYQAnalytics::extractDistinctSessions(apps);
        // "2023 Spring End-Term", "2023 Autumn End-Term", "2023 Spring Mid-Term" -> 3 sessions
        assert(sessions.size() == 3);
        assert(apps.size() == 4);

        std::cout << "  -> Passed: 4 appearances across 3 distinct examination sessions correctly resolved." << std::endl;
    }

    // ========================================================================
    // Test 7: Missing or invalid years
    // ========================================================================
    {
        std::cout << "[RUN] Test 7: Missing or invalid years..." << std::endl;
        std::vector<ExamAppearance> validApps = {
            ExamAppearance("End-Term", 2023, "Spring", "Q1", 10, false)
        };

        auto clean = PYQAnalytics::deduplicateAppearances(validApps);
        assert(clean.size() == 1);
        assert(PYQAnalytics::extractDistinctYears(clean).size() == 1);

        std::cout << "  -> Passed: Year bounds validated safely." << std::endl;
    }

    // ========================================================================
    // Test 8: Equal-frequency ranking ties
    // ========================================================================
    {
        std::cout << "[RUN] Test 8: Equal-frequency ranking ties..." << std::endl;
        // Two questions with 2 appearances each
        Question qA("PYQ-A", "Question A on Binary Search", "SUB-DSA");
        // qA appeared recently: 2023, 2024
        qA.addAppearance(ExamAppearance("End-Term", 2023, "Spring", "Q1", 10, false));
        qA.addAppearance(ExamAppearance("End-Term", 2024, "Spring", "Q1", 10, false));

        Question qB("PYQ-B", "Question B on Linear Search", "SUB-DSA");
        // qB appeared in the past: 2019, 2020
        qB.addAppearance(ExamAppearance("End-Term", 2019, "Spring", "Q1", 10, false));
        qB.addAppearance(ExamAppearance("End-Term", 2020, "Spring", "Q1", 10, false));

        std::vector<Question> questions = {qB, qA};
        auto ranked = PYQAnalytics::rankQuestionsByFrequency(questions, 2024);

        // Both have totalAppearances == 2 and distinctYearsCount == 2
        // Tie-breaker: qA has higher recency score, so qA must rank first!
        assert(ranked[0].questionId == "PYQ-A");
        assert(ranked[1].questionId == "PYQ-B");
        assert(ranked[0].recencyScore > ranked[1].recencyScore);

        std::cout << "  -> Passed: Equal-frequency tie cleanly broken by recency score (Recent PYQ-A ahead of Old PYQ-B)." << std::endl;
    }

    // ========================================================================
    // Test 9: Deterministic ranking
    // ========================================================================
    {
        std::cout << "[RUN] Test 9: Deterministic ranking across shuffled inputs..." << std::endl;
        Question q1("PYQ-01", "Topic A", "SUB-DSA");
        q1.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        Question q2("PYQ-02", "Topic B", "SUB-DSA");
        q2.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));
        q2.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        Question q3("PYQ-03", "Topic C", "SUB-DSA");
        q3.addAppearance(ExamAppearance("Exam", 2021, "Spring", "Q1", 10, false));
        q3.addAppearance(ExamAppearance("Exam", 2022, "Spring", "Q1", 10, false));
        q3.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));

        std::vector<Question> list1 = {q1, q2, q3};
        std::vector<Question> list2 = {q3, q1, q2};
        std::vector<Question> list3 = {q2, q3, q1};

        auto rank1 = PYQAnalytics::rankQuestionsByFrequency(list1, 2024);
        auto rank2 = PYQAnalytics::rankQuestionsByFrequency(list2, 2024);
        auto rank3 = PYQAnalytics::rankQuestionsByFrequency(list3, 2024);

        for (size_t i = 0; i < 3; ++i) {
            assert(rank1[i].questionId == rank2[i].questionId);
            assert(rank1[i].questionId == rank3[i].questionId);
        }

        std::cout << "  -> Passed: Rankings produce strictly identical results regardless of initial list permutation." << std::endl;
    }

    // ========================================================================
    // Test 10: Recency score boundaries [0.0, 1.0]
    // ========================================================================
    {
        std::cout << "[RUN] Test 10: Recency score boundaries [0.0, 1.0]..." << std::endl;
        // Empty
        assert(isClose(PYQAnalytics::calculateRecencyScore({}, 2024), 0.0));

        // 1 appearance 20 years ago
        std::vector<ExamAppearance> oldApp = {ExamAppearance("Exam", 2004, "Spring", "Q1", 10, false)};
        double oldScore = PYQAnalytics::calculateRecencyScore(oldApp, 2024);
        assert(oldScore >= 0.0 && oldScore <= 1.0);
        assert(oldScore < 0.01); // Extremely low

        // 1 appearance in reference year
        std::vector<ExamAppearance> refApp = {ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false)};
        double refScore = PYQAnalytics::calculateRecencyScore(refApp, 2024);
        assert(isClose(refScore, 0.50));

        // 10 appearances across consecutive years
        std::vector<ExamAppearance> manyApps;
        for (int y = 2015; y <= 2024; ++y) {
            manyApps.emplace_back("Exam", y, "Spring", "Q1", 10, false);
        }
        double manyScore = PYQAnalytics::calculateRecencyScore(manyApps, 2024);
        assert(manyScore >= 0.0 && manyScore < 1.0);
        assert(manyScore > 0.85); // High score

        std::cout << "  -> Passed: Recency scores strictly bounded within [0.0, 1.0] across all history depths." << std::endl;
    }

    // ========================================================================
    // Test 11: Consecutive-year recurrence
    // ========================================================================
    {
        std::cout << "[RUN] Test 11: Consecutive-year recurrence..." << std::endl;
        std::vector<ExamAppearance> apps = {
            ExamAppearance("Exam", 2022, "Spring", "Q1", 10, false),
            ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false)
        };

        auto trend = PYQAnalytics::analyzeTrend(apps, 2024);
        assert(trend.isSufficientData == true);
        assert(trend.hasConsecutiveRepeat == true);
        assert(trend.hasGapRecurrence == false);
        assert(trend.consecutiveYearStreak == 2);
        assert(trend.classification == TrendClassification::CONSECUTIVE_REPEAT);

        std::cout << "  -> Passed: Consecutive years (2022, 2023) identified as CONSECUTIVE_REPEAT (Streak: 2)." << std::endl;
    }

    // ========================================================================
    // Test 12: Recurrence after a gap
    // ========================================================================
    {
        std::cout << "[RUN] Test 12: Recurrence after a gap..." << std::endl;
        std::vector<ExamAppearance> apps = {
            ExamAppearance("Exam", 2020, "Spring", "Q1", 10, false),
            ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false) // 3 year gap
        };

        auto trend = PYQAnalytics::analyzeTrend(apps, 2024);
        assert(trend.isSufficientData == true);
        assert(trend.hasConsecutiveRepeat == false);
        assert(trend.hasGapRecurrence == true);
        assert(trend.maxYearGap == 3);
        assert(trend.classification == TrendClassification::PERIODIC_GAP_REPEAT);

        std::cout << "  -> Passed: Recurrence after gap (2020, 2023) identified as PERIODIC_GAP_REPEAT (Max Gap: 3)." << std::endl;
    }

    // ========================================================================
    // Test 13: Insufficient data for trend classification
    // ========================================================================
    {
        std::cout << "[RUN] Test 13: Insufficient data for trend classification..." << std::endl;
        // Zero appearances
        auto trend0 = PYQAnalytics::analyzeTrend({}, 2024);
        assert(trend0.isSufficientData == false);
        assert(trend0.classification == TrendClassification::INSUFFICIENT_DATA);

        // One appearance
        std::vector<ExamAppearance> oneApp = {ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false)};
        auto trend1 = PYQAnalytics::analyzeTrend(oneApp, 2024);
        assert(trend1.isSufficientData == false);
        assert(trend1.classification == TrendClassification::INSUFFICIENT_DATA);

        // Two appearances in the SAME academic year
        std::vector<ExamAppearance> sameYear = {
            ExamAppearance("Mid-Term", 2023, "Spring", "Q1", 5, false),
            ExamAppearance("End-Term", 2023, "Spring", "Q2", 10, false)
        };
        auto trendSame = PYQAnalytics::analyzeTrend(sameYear, 2024);
        assert(trendSame.isSufficientData == false);
        assert(trendSame.classification == TrendClassification::INSUFFICIENT_DATA);

        std::cout << "  -> Passed: Explicit INSUFFICIENT_DATA returned when data is inadequate to infer a trend." << std::endl;
    }

    // ========================================================================
    // Test 14: Multiple questions belonging to the same topic
    // ========================================================================
    {
        std::cout << "[RUN] Test 14: Multiple questions belonging to the same topic..." << std::endl;
        Question q1("PYQ-AVL-1", "Explain AVL Single Rotations.", "SUB-DSA", 2, "Trees", "AVL Trees");
        q1.addAppearance(ExamAppearance("Exam", 2021, "Spring", "Q1", 10, false));
        q1.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));

        Question q2("PYQ-AVL-2", "Explain AVL Double Rotations RL and LR.", "SUB-DSA", 2, "Trees", "AVL Trees");
        q2.addAppearance(ExamAppearance("Exam", 2022, "Spring", "Q1", 10, false));
        q2.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        std::vector<Question> questions = {q1, q2};
        auto topicStats = PYQAnalytics::analyzeTopic("AVL Trees", questions, nullptr, 2024);

        assert(topicStats.topicName == "AVL Trees");
        assert(topicStats.questionCount == 2);
        assert(topicStats.totalAppearances == 4);
        assert(topicStats.distinctYearsCount == 4); // 2021, 2022, 2023, 2024
        assert(topicStats.latestYear == 2024);
        assert(topicStats.earliestYear == 2021);
        assert(topicStats.totalMarks == 40);
        assert(isClose(topicStats.averageMarks, 10.0));
        assert(topicStats.trend.classification == TrendClassification::STABLE_HIGH_FREQUENCY);

        std::cout << "  -> Passed: Topic aggregation combines 2 questions into 4 appearances across 4 years (STABLE_HIGH_FREQUENCY)." << std::endl;
    }

    // ========================================================================
    // Test 15: Empty input collections
    // ========================================================================
    {
        std::cout << "[RUN] Test 15: Empty input collections..." << std::endl;
        std::vector<Question> emptyList;

        assert(PYQAnalytics::analyzeAllQuestions(emptyList).empty());
        assert(PYQAnalytics::rankQuestionsByFrequency(emptyList).empty());
        assert(PYQAnalytics::rankQuestionsByRecency(emptyList).empty());
        assert(PYQAnalytics::analyzeAllTopics(emptyList).empty());
        assert(PYQAnalytics::rankTopicsByFrequency(emptyList).empty());
        assert(PYQAnalytics::analyzeAllUnits(emptyList).empty());

        auto emptyTopic = PYQAnalytics::analyzeTopic("MissingTopic", emptyList);
        assert(emptyTopic.questionCount == 0);
        assert(emptyTopic.totalAppearances == 0);

        std::cout << "  -> Passed: Empty collections return safe empty outputs without exception or crash." << std::endl;
    }

    // ========================================================================
    // Test 16: Input order independence
    // ========================================================================
    {
        std::cout << "[RUN] Test 16: Input order independence for topic analytics..." << std::endl;
        Question q1("PYQ-01", "Text 1", "SUB-DSA", 1, "Unit 1", "Hashing");
        q1.addAppearance(ExamAppearance("Exam", 2022, "Spring", "Q1", 10, false));

        Question q2("PYQ-02", "Text 2", "SUB-DSA", 1, "Unit 1", "Hashing");
        q2.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));

        std::vector<Question> orderA = {q1, q2};
        std::vector<Question> orderB = {q2, q1};

        auto statsA = PYQAnalytics::analyzeTopic("Hashing", orderA, nullptr, 2024);
        auto statsB = PYQAnalytics::analyzeTopic("Hashing", orderB, nullptr, 2024);

        assert(statsA.totalAppearances == statsB.totalAppearances);
        assert(statsA.distinctYearsCount == statsB.distinctYearsCount);
        assert(statsA.latestYear == statsB.latestYear);
        assert(isClose(statsA.averageRecencyScore, statsB.averageRecencyScore));

        std::cout << "  -> Passed: Topic analytics are order-independent." << std::endl;
    }

    // ========================================================================
    // Test 17: Unit-level and subject-level frequency summaries
    // ========================================================================
    {
        std::cout << "[RUN] Test 17: Unit-level and subject-level frequency summaries..." << std::endl;
        Question q1("PYQ-U1", "Text 1", "SUB-DSA", 1, "Arrays & Complexity", "Big-O");
        q1.addAppearance(ExamAppearance("Exam", 2023, "Spring", "Q1", 10, false));

        Question q2("PYQ-U2", "Text 2", "SUB-DSA", 1, "Arrays & Complexity", "Binary Search");
        q2.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q1", 10, false));

        Question q3("PYQ-U3", "Text 3", "SUB-DSA", 2, "Trees", "AVL");
        q3.addAppearance(ExamAppearance("Exam", 2024, "Spring", "Q2", 10, false));

        std::vector<Question> questions = {q1, q2, q3};

        auto unit1Stats = PYQAnalytics::analyzeUnit(1, questions);
        assert(unit1Stats.unitNumber == 1);
        assert(unit1Stats.questionCount == 2);
        assert(unit1Stats.topicCount == 2);
        assert(unit1Stats.totalAppearances == 2);
        assert(unit1Stats.distinctYearsCount == 2);

        auto subjectStats = PYQAnalytics::analyzeSubject("SUB-DSA", questions);
        assert(subjectStats.subjectId == "SUB-DSA");
        assert(subjectStats.unitCount == 2);
        assert(subjectStats.topicCount == 3);
        assert(subjectStats.questionCount == 3);
        assert(subjectStats.totalAppearances == 3);

        std::cout << "  -> Passed: Unit and Subject hierarchy summaries correctly aggregate child elements." << std::endl;
    }

    // ========================================================================
    // Test 18: Explanations and audit reports
    // ========================================================================
    {
        std::cout << "[RUN] Test 18: Explanations and audit reports..." << std::endl;
        Question q("PYQ-AUDIT", "Explain MergeSort.", "SUB-DSA");
        q.addAppearance(ExamAppearance("End-Term", 2021, "Spring", "Q1", 10, false));
        q.addAppearance(ExamAppearance("End-Term", 2022, "Spring", "Q1", 10, false));

        auto stats = PYQAnalytics::analyzeQuestion(q, 2024);
        assert(!stats.explanation.empty());
        assert(stats.explanation.find("PYQ-AUDIT") != std::string::npos);
        assert(stats.explanation.find("CONSECUTIVE_REPEAT") != std::string::npos);
        assert(stats.explanation.find("Recency Score") != std::string::npos);

        assert(!stats.trend.explanation.empty());
        assert(stats.trend.explanation.find("CONSECUTIVE_REPEAT") != std::string::npos);

        // Dormant question explanation test
        std::vector<ExamAppearance> dormantApps = {
            ExamAppearance("Exam", 2016, "Spring", "Q1", 10, false),
            ExamAppearance("Exam", 2018, "Spring", "Q1", 10, false)
        };
        auto dormantTrend = PYQAnalytics::analyzeTrend(dormantApps, 2024);
        assert(dormantTrend.classification == TrendClassification::DORMANT);
        assert(dormantTrend.explanation.find("DORMANT") != std::string::npos);

        std::cout << "  -> Passed: Human-readable diagnostic audit trails generated for stats, trends, and dormant questions." << std::endl;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 18 PYQ ANALYTICS & TREND TESTS PASSED!               " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
