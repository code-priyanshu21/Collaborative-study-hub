#ifndef STUDYSYNC_PYQ_ANALYTICS_HPP
#define STUDYSYNC_PYQ_ANALYTICS_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

#include "models/Question.hpp"
#include "ds/SyllabusTree.hpp"

namespace studysync {

/**
 * @brief Categorization of observed historical recurrence patterns.
 * 
 * NOTE: These classifications represent observed historical facts and pattern heuristics,
 * NOT predictive probabilities of future exam content.
 */
enum class TrendClassification {
    INSUFFICIENT_DATA,      // < 2 distinct academic years recorded; cannot infer multi-year trend
    CONSECUTIVE_REPEAT,     // Appeared in consecutive recorded academic years (e.g. 2022, 2023)
    PERIODIC_GAP_REPEAT,    // Recurred after a gap of 1 or more intervening academic years (e.g. 2020, 2023)
    EMERGING_RECENT,        // Active exclusively in recent years (within 2 years of reference)
    DORMANT,                // Historically active (>= 2 appearances) but absent in recent cycles (>= 4 year gap)
    STABLE_HIGH_FREQUENCY   // Consistently present across 3 or more distinct academic years
};

inline std::string trendClassificationToString(TrendClassification c) {
    switch (c) {
        case TrendClassification::INSUFFICIENT_DATA:    return "INSUFFICIENT_DATA";
        case TrendClassification::CONSECUTIVE_REPEAT:   return "CONSECUTIVE_REPEAT";
        case TrendClassification::PERIODIC_GAP_REPEAT:  return "PERIODIC_GAP_REPEAT";
        case TrendClassification::EMERGING_RECENT:      return "EMERGING_RECENT";
        case TrendClassification::DORMANT:              return "DORMANT";
        case TrendClassification::STABLE_HIGH_FREQUENCY:return "STABLE_HIGH_FREQUENCY";
        default:                                        return "UNKNOWN";
    }
}

/**
 * @brief Detailed breakdown of question or topic historical recurrence patterns.
 * Explicitly separates objective historical facts from inferred trend classifications.
 */
struct TrendAnalysisResult {
    TrendClassification classification;
    std::string classificationName;
    bool isSufficientData;          // True if >= 2 distinct academic years available
    bool hasConsecutiveRepeat;      // Observed fact: true if appeared in year y and y+1
    bool hasGapRecurrence;          // Observed fact: true if gap between any consecutive appearances >= 2
    int maxYearGap;                 // Observed fact: maximum gap between recorded appearance years
    int consecutiveYearStreak;      // Observed fact: longest consecutive sequence of years
    int recentAppearances;          // Observed fact: appearances within recent window (<= 3 years from reference)
    int historicalAppearances;      // Observed fact: appearances older than recent window
    std::string explanation;        // Human-readable diagnostic breakdown
};

/**
 * @brief Reusable frequency and recency analytics for a single question.
 */
struct QuestionFrequencyStats {
    std::string questionId;
    std::string questionText;
    std::string subject;
    int unitNumber;
    std::string topic;
    size_t totalAppearances;
    size_t distinctYearsCount;
    size_t distinctSessionsCount;
    int latestYear;
    int earliestYear;
    int totalMarks;
    double averageMarks;
    double recencyScore;            // Deterministic score in [0.0, 1.0]
    std::vector<int> distinctYears; // Ascending sorted distinct academic years
    std::vector<std::string> distinctSessions; // Distinct examination sessions (Year Sem Exam)
    TrendAnalysisResult trend;
    std::string explanation;
};

/**
 * @brief Topic-level aggregated frequency, recency, and recurrence analytics.
 */
struct TopicFrequencyStats {
    std::string topicId;
    std::string topicName;
    std::string subjectId;
    int unitNumber;
    size_t questionCount;
    size_t totalAppearances;
    size_t distinctYearsCount;
    size_t distinctSessionsCount;
    int latestYear;
    int earliestYear;
    int totalMarks;
    double averageMarks;
    double averageRecencyScore;
    std::vector<std::string> questionIds;
    std::vector<int> distinctYears;
    TrendAnalysisResult trend;
    std::string explanation;
};

/**
 * @brief Unit-level aggregated frequency analytics.
 */
struct UnitFrequencyStats {
    int unitNumber;
    std::string unitName;
    std::string subjectId;
    size_t topicCount;
    size_t questionCount;
    size_t totalAppearances;
    size_t distinctYearsCount;
    int latestYear;
    int totalMarks;
    double averageMarks;
    std::vector<std::string> topicNames;
    std::string explanation;
};

/**
 * @brief Subject-level aggregated frequency analytics.
 */
struct SubjectFrequencyStats {
    std::string subjectId;
    std::string subjectName;
    size_t unitCount;
    size_t topicCount;
    size_t questionCount;
    size_t totalAppearances;
    size_t distinctYearsCount;
    int latestYear;
    int totalMarks;
    double averageMarks;
    std::string explanation;
};

/**
 * @brief Statistical engine for examination question frequency analysis,
 * half-life recency-weighted scoring, and deterministic repeat-trend detection.
 */
class PYQAnalytics {
public:
    static constexpr int DEFAULT_REFERENCE_YEAR = 2024;
    static constexpr double DEFAULT_HALF_LIFE_YEARS = 2.0;
    static constexpr int RECENT_WINDOW_YEARS = 3;
    static constexpr int DORMANT_GAP_YEARS = 4;

    // ------------------------------------------------------------------------
    // Appearance & Session Deduplication Helpers
    // ------------------------------------------------------------------------

    /**
     * @brief Deduplicates appearance records and filters invalid records.
     * Guaranteed safe against duplicate records in historical data.
     */
    static std::vector<ExamAppearance> deduplicateAppearances(
        const std::vector<ExamAppearance>& rawHistory) {

        std::vector<ExamAppearance> clean;
        for (const auto& app : rawHistory) {
            // Validate year sanity
            if (app.getAcademicYear() < 1900 || app.getAcademicYear() > 2100) {
                continue;
            }
            if (std::find(clean.begin(), clean.end(), app) == clean.end()) {
                clean.push_back(app);
            }
        }
        return clean;
    }

    /**
     * @brief Generates an examination session identifier: e.g., "2023 Spring End-Term Exam".
     * Multiple question sub-parts within the same paper belong to one examination session.
     */
    static std::string formatSessionKey(const ExamAppearance& a) {
        std::ostringstream oss;
        oss << a.getAcademicYear();
        if (!a.getSemester().empty()) {
            oss << " " << a.getSemester();
        }
        oss << " " << a.getExamName();
        return oss.str();
    }

    /**
     * @brief Extracts sorted, unique academic years from appearance history.
     */
    static std::vector<int> extractDistinctYears(const std::vector<ExamAppearance>& history) {
        std::vector<int> years;
        for (const auto& a : history) {
            if (a.getAcademicYear() >= 1900 && a.getAcademicYear() <= 2100) {
                if (std::find(years.begin(), years.end(), a.getAcademicYear()) == years.end()) {
                    years.push_back(a.getAcademicYear());
                }
            }
        }
        std::sort(years.begin(), years.end());
        return years;
    }

    /**
     * @brief Extracts distinct examination sessions from appearance history.
     */
    static std::vector<std::string> extractDistinctSessions(const std::vector<ExamAppearance>& history) {
        std::vector<std::string> sessions;
        for (const auto& a : history) {
            std::string key = formatSessionKey(a);
            if (std::find(sessions.begin(), sessions.end(), key) == sessions.end()) {
                sessions.push_back(std::move(key));
            }
        }
        return sessions;
    }

    // ------------------------------------------------------------------------
    // Recency-Weighted Scoring Formula
    // ------------------------------------------------------------------------

    /**
     * @brief Calculates a deterministic, bounded recency score in [0.0, 1.0].
     * 
     * MATHEMATICAL FORMULA & DESIGN:
     * 1. Data Granularity: The system records academic examination years (e.g. 2021, 2023).
     *    Calculations operate explicitly on academic year deltas (Delta_y = max(0, referenceYear - year)).
     * 2. Exponential Half-Life Decay:
     *    Each unique appearance 'a' receives an age-decay weight:
     *        w(a) = (0.5) ^ (Delta_y / halfLifeYears)
     *    With default halfLifeYears = 2.0:
     *        - Current year (Delta_y = 0): w = 1.000
     *        - 1 year ago   (Delta_y = 1): w = 0.707
     *        - 2 years ago  (Delta_y = 2): w = 0.500
     *        - 4 years ago  (Delta_y = 4): w = 0.250
     *        - 6 years ago  (Delta_y = 6): w = 0.125
     * 3. Cumulative Decayed Sum: S = sum(w(a)) across all deduplicated appearances.
     * 4. Monotonic Saturation Bounding into [0.0, 1.0]:
     *        recencyScore = 1.0 - (0.5) ^ S
     *    - S = 0.00 (no history)            -> recencyScore = 0.000
     *    - S = 1.00 (1 appearance this year)-> recencyScore = 0.500
     *    - S = 1.70 (recent repeats)        -> recencyScore = 0.693
     *    - S >= 3.00 (consistent appearances)-> recencyScore >= 0.875 -> 1.0
     * 
     * IMPORTANT: This score reflects historical recency and recurrence density.
     * It is NOT an empirical probability that the question will appear in a future exam.
     */
    static double calculateRecencyScore(
        const std::vector<ExamAppearance>& appearances,
        int referenceYear = DEFAULT_REFERENCE_YEAR,
        double halfLifeYears = DEFAULT_HALF_LIFE_YEARS) {

        auto clean = deduplicateAppearances(appearances);
        if (clean.empty() || halfLifeYears <= 0.0) {
            return 0.0;
        }

        double accumulatedSum = 0.0;
        for (const auto& a : clean) {
            int year = a.getAcademicYear();
            int deltaYear = std::max(0, referenceYear - year);
            double weight = std::pow(0.5, static_cast<double>(deltaYear) / halfLifeYears);
            accumulatedSum += weight;
        }

        if (accumulatedSum <= 0.0) {
            return 0.0;
        }

        // Bounded in [0.0, 1.0)
        double score = 1.0 - std::pow(0.5, accumulatedSum);
        if (score < 0.0) score = 0.0;
        if (score > 1.0) score = 1.0;
        return score;
    }

    static double calculateRecencyScore(
        const Question& q,
        int referenceYear = DEFAULT_REFERENCE_YEAR,
        double halfLifeYears = DEFAULT_HALF_LIFE_YEARS) {
        return calculateRecencyScore(q.getHistory(), referenceYear, halfLifeYears);
    }

    // ------------------------------------------------------------------------
    // Repeat-Trend Detection
    // ------------------------------------------------------------------------

    /**
     * @brief Detects historical recurrence patterns and distinguishes facts from classifications.
     */
    static TrendAnalysisResult analyzeTrend(
        const std::vector<ExamAppearance>& appearances,
        int referenceYear = DEFAULT_REFERENCE_YEAR) {

        TrendAnalysisResult result;
        result.classification = TrendClassification::INSUFFICIENT_DATA;
        result.classificationName = trendClassificationToString(result.classification);
        result.isSufficientData = false;
        result.hasConsecutiveRepeat = false;
        result.hasGapRecurrence = false;
        result.maxYearGap = 0;
        result.consecutiveYearStreak = 0;
        result.recentAppearances = 0;
        result.historicalAppearances = 0;

        auto clean = deduplicateAppearances(appearances);
        auto distinctYears = extractDistinctYears(clean);

        // Count recent vs older appearances
        for (const auto& a : clean) {
            if ((referenceYear - a.getAcademicYear()) <= RECENT_WINDOW_YEARS) {
                result.recentAppearances++;
            } else {
                result.historicalAppearances++;
            }
        }

        // Rule 1: Insufficient Data check
        if (distinctYears.size() < 2) {
            result.isSufficientData = false;
            result.classification = TrendClassification::INSUFFICIENT_DATA;
            result.classificationName = trendClassificationToString(result.classification);

            std::ostringstream oss;
            oss << "Insufficient historical data: Question appeared in "
                << distinctYears.size() << " distinct academic year(s) ("
                << clean.size() << " total appearance(s)). "
                << "At least 2 distinct academic years with recorded history are required to establish an observational recurrence trend.";
            result.explanation = oss.str();
            return result;
        }

        result.isSufficientData = true;

        // Compute consecutive streaks and gap metrics
        int currentStreak = 1;
        int maxStreak = 1;

        for (size_t i = 1; i < distinctYears.size(); ++i) {
            int gap = distinctYears[i] - distinctYears[i - 1];
            if (gap > result.maxYearGap) {
                result.maxYearGap = gap;
            }

            if (gap == 1) {
                result.hasConsecutiveRepeat = true;
                currentStreak++;
                if (currentStreak > maxStreak) {
                    maxStreak = currentStreak;
                }
            } else {
                result.hasGapRecurrence = true;
                currentStreak = 1;
            }
        }
        result.consecutiveYearStreak = maxStreak;

        int latestYear = distinctYears.back();
        int yearsSinceLatest = referenceYear - latestYear;

        // Rule 2: Dormant Classification
        if (yearsSinceLatest >= DORMANT_GAP_YEARS && clean.size() >= 2) {
            result.classification = TrendClassification::DORMANT;
            result.classificationName = trendClassificationToString(result.classification);

            std::ostringstream oss;
            oss << "DORMANT: Question appeared " << clean.size() << " times across years ";
            for (size_t i = 0; i < distinctYears.size(); ++i) {
                oss << distinctYears[i] << (i + 1 < distinctYears.size() ? ", " : "");
            }
            oss << ", but has not been recorded in the past " << yearsSinceLatest
                << " years relative to reference year " << referenceYear << ".";
            result.explanation = oss.str();
            return result;
        }

        // Rule 3: Stable High Frequency (3+ distinct years with continuous streak)
        if (distinctYears.size() >= 3 && result.hasConsecutiveRepeat && result.consecutiveYearStreak >= 3) {
            result.classification = TrendClassification::STABLE_HIGH_FREQUENCY;
            result.classificationName = trendClassificationToString(result.classification);

            std::ostringstream oss;
            oss << "STABLE_HIGH_FREQUENCY: Frequently recurring across " << distinctYears.size()
                << " distinct years with a maximum consecutive streak of "
                << result.consecutiveYearStreak << " year(s).";
            result.explanation = oss.str();
            return result;
        }

        // Rule 4: Consecutive Repeat (Appeared across consecutive academic years without historical gaps)
        if (result.hasConsecutiveRepeat && !result.hasGapRecurrence) {
            result.classification = TrendClassification::CONSECUTIVE_REPEAT;
            result.classificationName = trendClassificationToString(result.classification);

            std::ostringstream oss;
            oss << "CONSECUTIVE_REPEAT: Question appeared in consecutive recorded academic years "
                << "(Streak: " << result.consecutiveYearStreak << " years).";
            result.explanation = oss.str();
            return result;
        }

        // Rule 5: Periodic Gap Repeat (Appeared with intervening year gaps without consecutive repeats)
        if (result.hasGapRecurrence && !result.hasConsecutiveRepeat) {
            result.classification = TrendClassification::PERIODIC_GAP_REPEAT;
            result.classificationName = trendClassificationToString(result.classification);

            std::ostringstream oss;
            oss << "PERIODIC_GAP_REPEAT: Question recurred after historical gaps "
                << "(Max gap between appearances: " << result.maxYearGap << " years).";
            result.explanation = oss.str();
            return result;
        }

        // Rule 6: Emerging Recent (Recent recurrence surge combined with older historical records)
        if (result.recentAppearances >= 2) {
            result.classification = TrendClassification::EMERGING_RECENT;
            result.classificationName = trendClassificationToString(result.classification);

            std::ostringstream oss;
            oss << "EMERGING_RECENT: Recent recurrence surge observed with " << result.recentAppearances
                << " appearances in the recent 3-year window relative to reference year " << referenceYear << ".";
            result.explanation = oss.str();
            return result;
        }

        // Fallback for mixed multi-year recurrences
        if (result.hasConsecutiveRepeat) {
            result.classification = TrendClassification::CONSECUTIVE_REPEAT;
            result.classificationName = trendClassificationToString(result.classification);
            result.explanation = "CONSECUTIVE_REPEAT: Question exhibited consecutive-year recurrence.";
            return result;
        }

        result.classification = TrendClassification::PERIODIC_GAP_REPEAT;
        result.classificationName = trendClassificationToString(result.classification);
        result.explanation = "PERIODIC_GAP_REPEAT: Question exhibited periodic recurrence across historical years.";
        return result;

        result.explanation = "General multi-year recurrence observed across " +
                             std::to_string(distinctYears.size()) + " distinct years.";
        return result;
    }

    static TrendAnalysisResult analyzeTrend(
        const Question& q,
        int referenceYear = DEFAULT_REFERENCE_YEAR) {
        return analyzeTrend(q.getHistory(), referenceYear);
    }

    // ------------------------------------------------------------------------
    // Question Frequency Analytics
    // ------------------------------------------------------------------------

    /**
     * @brief Performs complete frequency, recency, and trend analytics for a question.
     */
    static QuestionFrequencyStats analyzeQuestion(
        const Question& q,
        int referenceYear = DEFAULT_REFERENCE_YEAR) {

        QuestionFrequencyStats stats;
        stats.questionId = q.getId();
        stats.questionText = q.getText();
        stats.subject = q.getSubject();
        stats.unitNumber = q.getUnitNumber();
        stats.topic = q.getTopic();

        auto clean = deduplicateAppearances(q.getHistory());
        stats.totalAppearances = clean.size();
        stats.distinctYears = extractDistinctYears(clean);
        stats.distinctYearsCount = stats.distinctYears.size();
        stats.distinctSessions = extractDistinctSessions(clean);
        stats.distinctSessionsCount = stats.distinctSessions.size();

        if (stats.distinctYears.empty()) {
            stats.latestYear = 0;
            stats.earliestYear = 0;
        } else {
            stats.latestYear = stats.distinctYears.back();
            stats.earliestYear = stats.distinctYears.front();
        }

        stats.totalMarks = 0;
        for (const auto& a : clean) {
            stats.totalMarks += a.getMarks();
        }
        stats.averageMarks = stats.totalAppearances > 0
            ? static_cast<double>(stats.totalMarks) / static_cast<double>(stats.totalAppearances)
            : 0.0;

        stats.recencyScore = calculateRecencyScore(clean, referenceYear);
        stats.trend = analyzeTrend(clean, referenceYear);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Question [" << stats.questionId << "]: "
            << stats.totalAppearances << " appearance(s) across "
            << stats.distinctYearsCount << " distinct year(s) and "
            << stats.distinctSessionsCount << " session(s). "
            << "Latest: " << (stats.latestYear > 0 ? std::to_string(stats.latestYear) : "N/A")
            << " | Total Marks: " << stats.totalMarks
            << " (Avg: " << stats.averageMarks << ")"
            << " | Recency Score: " << stats.recencyScore
            << " | Trend: " << stats.trend.classificationName;
        stats.explanation = oss.str();

        return stats;
    }

    static std::vector<QuestionFrequencyStats> analyzeAllQuestions(
        const std::vector<Question>& questions,
        int referenceYear = DEFAULT_REFERENCE_YEAR) {

        std::vector<QuestionFrequencyStats> results;
        results.reserve(questions.size());
        for (const auto& q : questions) {
            results.push_back(analyzeQuestion(q, referenceYear));
        }
        return results;
    }

    /**
     * @brief Ranks questions by appearance frequency with deterministic tie-breaking.
     * Tie-breaking hierarchy:
     * 1. Total appearances (descending)
     * 2. Distinct years count (descending)
     * 3. Recency score (descending)
     * 4. Latest year (descending)
     * 5. Total marks carried (descending)
     * 6. Question ID (ascending lexicographical)
     */
    static std::vector<QuestionFrequencyStats> rankQuestionsByFrequency(
        const std::vector<Question>& questions,
        int referenceYear = DEFAULT_REFERENCE_YEAR) {

        auto list = analyzeAllQuestions(questions, referenceYear);
        std::sort(list.begin(), list.end(), [](const QuestionFrequencyStats& a, const QuestionFrequencyStats& b) {
            if (a.totalAppearances != b.totalAppearances) {
                return a.totalAppearances > b.totalAppearances;
            }
            if (a.distinctYearsCount != b.distinctYearsCount) {
                return a.distinctYearsCount > b.distinctYearsCount;
            }
            if (std::abs(a.recencyScore - b.recencyScore) > 0.001) {
                return a.recencyScore > b.recencyScore;
            }
            if (a.latestYear != b.latestYear) {
                return a.latestYear > b.latestYear;
            }
            if (a.totalMarks != b.totalMarks) {
                return a.totalMarks > b.totalMarks;
            }
            return a.questionId < b.questionId;
        });
        return list;
    }

    /**
     * @brief Ranks questions by recency score with deterministic tie-breaking.
     */
    static std::vector<QuestionFrequencyStats> rankQuestionsByRecency(
        const std::vector<Question>& questions,
        int referenceYear = DEFAULT_REFERENCE_YEAR) {

        auto list = analyzeAllQuestions(questions, referenceYear);
        std::sort(list.begin(), list.end(), [](const QuestionFrequencyStats& a, const QuestionFrequencyStats& b) {
            if (std::abs(a.recencyScore - b.recencyScore) > 0.001) {
                return a.recencyScore > b.recencyScore;
            }
            if (a.latestYear != b.latestYear) {
                return a.latestYear > b.latestYear;
            }
            if (a.totalAppearances != b.totalAppearances) {
                return a.totalAppearances > b.totalAppearances;
            }
            if (a.distinctYearsCount != b.distinctYearsCount) {
                return a.distinctYearsCount > b.distinctYearsCount;
            }
            return a.questionId < b.questionId;
        });
        return list;
    }

    // ------------------------------------------------------------------------
    // Topic-Level Frequency Analytics
    // ------------------------------------------------------------------------

    /**
     * @brief Computes aggregated frequency and recurrence metrics for a topic.
     */
    static TopicFrequencyStats analyzeTopic(
        const std::string& topicNameOrId,
        const std::vector<Question>& questions,
        const SyllabusTree* tree = nullptr,
        int referenceYear = DEFAULT_REFERENCE_YEAR) {

        TopicFrequencyStats stats;
        stats.topicId = topicNameOrId;
        stats.topicName = topicNameOrId;
        stats.subjectId = "";
        stats.unitNumber = 0;
        stats.questionCount = 0;
        stats.totalAppearances = 0;
        stats.distinctYearsCount = 0;
        stats.distinctSessionsCount = 0;
        stats.latestYear = 0;
        stats.earliestYear = 0;
        stats.totalMarks = 0;
        stats.averageMarks = 0.0;
        stats.averageRecencyScore = 0.0;

        std::vector<std::string> matchingQIds;

        // If a SyllabusTree is provided, query question IDs mapped to this topic
        if (tree != nullptr) {
            matchingQIds = tree->getQuestionsForTopic(topicNameOrId);
        }

        std::vector<ExamAppearance> allTopicAppearances;
        double sumRecency = 0.0;

        for (const auto& q : questions) {
            bool matches = false;
            if (!matchingQIds.empty()) {
                matches = std::find(matchingQIds.begin(), matchingQIds.end(), q.getId()) != matchingQIds.end();
            } else {
                // Match by question's internal topic field
                matches = (q.getTopic() == topicNameOrId);
            }

            if (matches) {
                stats.questionCount++;
                stats.questionIds.push_back(q.getId());
                if (stats.subjectId.empty()) stats.subjectId = q.getSubject();
                if (stats.unitNumber == 0) stats.unitNumber = q.getUnitNumber();

                auto clean = deduplicateAppearances(q.getHistory());
                for (const auto& app : clean) {
                    allTopicAppearances.push_back(app);
                    stats.totalMarks += app.getMarks();
                }

                sumRecency += calculateRecencyScore(clean, referenceYear);
            }
        }

        // Deduplicate appearances at the topic level to prevent double-counting
        auto cleanTopicApps = deduplicateAppearances(allTopicAppearances);
        stats.totalAppearances = cleanTopicApps.size();
        stats.distinctYears = extractDistinctYears(cleanTopicApps);
        stats.distinctYearsCount = stats.distinctYears.size();
        stats.distinctSessionsCount = extractDistinctSessions(cleanTopicApps).size();

        if (!stats.distinctYears.empty()) {
            stats.latestYear = stats.distinctYears.back();
            stats.earliestYear = stats.distinctYears.front();
        }

        stats.averageMarks = stats.totalAppearances > 0
            ? static_cast<double>(stats.totalMarks) / static_cast<double>(stats.totalAppearances)
            : 0.0;

        stats.averageRecencyScore = stats.questionCount > 0
            ? sumRecency / static_cast<double>(stats.questionCount)
            : 0.0;

        stats.trend = analyzeTrend(cleanTopicApps, referenceYear);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Topic [" << stats.topicName << "]: "
            << stats.questionCount << " question(s), "
            << stats.totalAppearances << " total appearance(s) across "
            << stats.distinctYearsCount << " distinct year(s). "
            << "Latest Year: " << (stats.latestYear > 0 ? std::to_string(stats.latestYear) : "N/A")
            << " | Avg Marks: " << stats.averageMarks
            << " | Avg Recency: " << stats.averageRecencyScore
            << " | Trend: " << stats.trend.classificationName;
        stats.explanation = oss.str();

        return stats;
    }

    /**
     * @brief Computes topic-level frequency statistics for all distinct topics in the question set.
     */
    static std::vector<TopicFrequencyStats> analyzeAllTopics(
        const std::vector<Question>& questions,
        const SyllabusTree* tree = nullptr,
        int referenceYear = DEFAULT_REFERENCE_YEAR) {

        std::vector<std::string> topicNames;
        for (const auto& q : questions) {
            if (!q.getTopic().empty()) {
                if (std::find(topicNames.begin(), topicNames.end(), q.getTopic()) == topicNames.end()) {
                    topicNames.push_back(q.getTopic());
                }
            }
        }

        std::sort(topicNames.begin(), topicNames.end());

        std::vector<TopicFrequencyStats> results;
        results.reserve(topicNames.size());
        for (const auto& tName : topicNames) {
            results.push_back(analyzeTopic(tName, questions, tree, referenceYear));
        }
        return results;
    }

    /**
     * @brief Ranks topics by total appearance frequency.
     */
    static std::vector<TopicFrequencyStats> rankTopicsByFrequency(
        const std::vector<Question>& questions,
        const SyllabusTree* tree = nullptr,
        int referenceYear = DEFAULT_REFERENCE_YEAR) {

        auto list = analyzeAllTopics(questions, tree, referenceYear);
        std::sort(list.begin(), list.end(), [](const TopicFrequencyStats& a, const TopicFrequencyStats& b) {
            if (a.totalAppearances != b.totalAppearances) {
                return a.totalAppearances > b.totalAppearances;
            }
            if (a.distinctYearsCount != b.distinctYearsCount) {
                return a.distinctYearsCount > b.distinctYearsCount;
            }
            if (std::abs(a.averageRecencyScore - b.averageRecencyScore) > 0.001) {
                return a.averageRecencyScore > b.averageRecencyScore;
            }
            return a.topicName < b.topicName;
        });
        return list;
    }

    // ------------------------------------------------------------------------
    // Unit & Subject Level Summaries
    // ------------------------------------------------------------------------

    /**
     * @brief Computes unit-level frequency summary.
     */
    static UnitFrequencyStats analyzeUnit(
        int unitNumber,
        const std::vector<Question>& questions,
        const SyllabusTree* tree = nullptr) {

        (void)tree; // Can be used for tree-based verification if needed

        UnitFrequencyStats stats;
        stats.unitNumber = unitNumber;
        stats.unitName = "";
        stats.subjectId = "";
        stats.topicCount = 0;
        stats.questionCount = 0;
        stats.totalAppearances = 0;
        stats.distinctYearsCount = 0;
        stats.latestYear = 0;
        stats.totalMarks = 0;
        stats.averageMarks = 0.0;

        std::vector<ExamAppearance> allUnitApps;

        for (const auto& q : questions) {
            if (q.getUnitNumber() == unitNumber) {
                stats.questionCount++;
                if (stats.unitName.empty()) stats.unitName = q.getUnitName();
                if (stats.subjectId.empty()) stats.subjectId = q.getSubject();

                if (!q.getTopic().empty()) {
                    if (std::find(stats.topicNames.begin(), stats.topicNames.end(), q.getTopic()) == stats.topicNames.end()) {
                        stats.topicNames.push_back(q.getTopic());
                    }
                }

                auto clean = deduplicateAppearances(q.getHistory());
                for (const auto& a : clean) {
                    allUnitApps.push_back(a);
                    stats.totalMarks += a.getMarks();
                }
            }
        }

        stats.topicCount = stats.topicNames.size();
        auto cleanUnitApps = deduplicateAppearances(allUnitApps);
        stats.totalAppearances = cleanUnitApps.size();
        auto distinctYears = extractDistinctYears(cleanUnitApps);
        stats.distinctYearsCount = distinctYears.size();
        if (!distinctYears.empty()) {
            stats.latestYear = distinctYears.back();
        }

        stats.averageMarks = stats.totalAppearances > 0
            ? static_cast<double>(stats.totalMarks) / static_cast<double>(stats.totalAppearances)
            : 0.0;

        std::ostringstream oss;
        oss << "Unit " << stats.unitNumber << " [" << stats.unitName << "]: "
            << stats.topicCount << " topic(s), "
            << stats.questionCount << " question(s), "
            << stats.totalAppearances << " appearance(s) across "
            << stats.distinctYearsCount << " year(s).";
        stats.explanation = oss.str();

        return stats;
    }

    static std::vector<UnitFrequencyStats> analyzeAllUnits(
        const std::vector<Question>& questions,
        const SyllabusTree* tree = nullptr) {

        std::vector<int> unitNumbers;
        for (const auto& q : questions) {
            if (std::find(unitNumbers.begin(), unitNumbers.end(), q.getUnitNumber()) == unitNumbers.end()) {
                unitNumbers.push_back(q.getUnitNumber());
            }
        }
        std::sort(unitNumbers.begin(), unitNumbers.end());

        std::vector<UnitFrequencyStats> list;
        for (int u : unitNumbers) {
            list.push_back(analyzeUnit(u, questions, tree));
        }
        return list;
    }

    /**
     * @brief Computes subject-level frequency summary.
     */
    static SubjectFrequencyStats analyzeSubject(
        const std::string& subjectIdOrName,
        const std::vector<Question>& questions,
        const SyllabusTree* tree = nullptr) {

        (void)tree;

        SubjectFrequencyStats stats;
        stats.subjectId = subjectIdOrName;
        stats.subjectName = subjectIdOrName;
        stats.unitCount = 0;
        stats.topicCount = 0;
        stats.questionCount = 0;
        stats.totalAppearances = 0;
        stats.distinctYearsCount = 0;
        stats.latestYear = 0;
        stats.totalMarks = 0;
        stats.averageMarks = 0.0;

        std::vector<int> units;
        std::vector<std::string> topics;
        std::vector<ExamAppearance> allSubjectApps;

        for (const auto& q : questions) {
            if (q.getSubject() == subjectIdOrName || subjectIdOrName.empty()) {
                stats.questionCount++;
                if (std::find(units.begin(), units.end(), q.getUnitNumber()) == units.end()) {
                    units.push_back(q.getUnitNumber());
                }
                if (!q.getTopic().empty()) {
                    if (std::find(topics.begin(), topics.end(), q.getTopic()) == topics.end()) {
                        topics.push_back(q.getTopic());
                    }
                }

                auto clean = deduplicateAppearances(q.getHistory());
                for (const auto& a : clean) {
                    allSubjectApps.push_back(a);
                    stats.totalMarks += a.getMarks();
                }
            }
        }

        stats.unitCount = units.size();
        stats.topicCount = topics.size();
        auto cleanSubApps = deduplicateAppearances(allSubjectApps);
        stats.totalAppearances = cleanSubApps.size();
        auto distinctYears = extractDistinctYears(cleanSubApps);
        stats.distinctYearsCount = distinctYears.size();
        if (!distinctYears.empty()) {
            stats.latestYear = distinctYears.back();
        }

        stats.averageMarks = stats.totalAppearances > 0
            ? static_cast<double>(stats.totalMarks) / static_cast<double>(stats.totalAppearances)
            : 0.0;

        std::ostringstream oss;
        oss << "Subject [" << stats.subjectId << "]: "
            << stats.unitCount << " unit(s), "
            << stats.topicCount << " topic(s), "
            << stats.questionCount << " question(s), "
            << stats.totalAppearances << " appearance(s) across "
            << stats.distinctYearsCount << " distinct year(s).";
        stats.explanation = oss.str();

        return stats;
    }
};

} // namespace studysync

#endif // STUDYSYNC_PYQ_ANALYTICS_HPP
