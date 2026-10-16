#ifndef STUDYSYNC_PYQ_PREDICTOR_HPP
#define STUDYSYNC_PYQ_PREDICTOR_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "models/Question.hpp"
#include "ds/SyllabusTree.hpp"
#include "algorithms/QuestionSimilarity.hpp"
#include "analytics/PYQAnalytics.hpp"

namespace studysync {

/**
 * @brief Configurable weights for multi-dimensional historical priority scoring.
 * 
 * Weights are automatically normalized so that their sum equals 1.0.
 */
struct ScoringWeights {
    double frequencyWeight;   // Weight for appearance frequency [default: 0.25]
    double recencyWeight;     // Weight for recency-decayed score [default: 0.30]
    double consistencyWeight; // Weight for distinct academic year breadth [default: 0.20]
    double marksWeight;       // Weight for average/total exam marks carried [default: 0.10]
    double trendWeight;       // Weight for repeat-trend recurrence dynamics [default: 0.15]

    ScoringWeights(double freq = 0.25,
                   double rec = 0.30,
                   double cons = 0.20,
                   double marks = 0.10,
                   double trend = 0.15)
        : frequencyWeight(freq),
          recencyWeight(rec),
          consistencyWeight(cons),
          marksWeight(marks),
          trendWeight(trend) {}

    bool isValid() const noexcept {
        if (frequencyWeight < 0.0 || recencyWeight < 0.0 || consistencyWeight < 0.0 ||
            marksWeight < 0.0 || trendWeight < 0.0) {
            return false;
        }
        double sum = frequencyWeight + recencyWeight + consistencyWeight + marksWeight + trendWeight;
        return sum > 0.0;
    }

    ScoringWeights normalized() const {
        double sum = frequencyWeight + recencyWeight + consistencyWeight + marksWeight + trendWeight;
        if (sum <= 0.0) {
            return ScoringWeights(0.25, 0.30, 0.20, 0.10, 0.15);
        }
        return ScoringWeights(
            frequencyWeight / sum,
            recencyWeight / sum,
            consistencyWeight / sum,
            marksWeight / sum,
            trendWeight / sum
        );
    }
};

/**
 * @brief Configuration parameters governing historical examination priority analysis.
 */
struct PredictorConfig {
    static constexpr int DEFAULT_REFERENCE_YEAR = 2024;
    static constexpr double DEFAULT_RECENCY_HALF_LIFE = 2.0;

    int referenceYear;               // Explicit reference academic year
    double recencyHalfLife;          // Half-life in academic years for temporal decay
    size_t minAppearancesForScoring; // Minimum recorded appearances required for scoring
    size_t resultLimit;              // Maximum results to return (0 = unlimited)
    std::string subjectFilter;       // Filter by subject identifier ("" = all)
    int unitFilter;                  // Filter by unit number (0 = all units)
    std::string topicFilter;         // Filter by topic name ("" = all topics)
    bool enableClusterAggregation;   // Whether to factor in cluster variant evidence
    ScoringWeights weights;          // Component scoring weights

    PredictorConfig(int refYear = DEFAULT_REFERENCE_YEAR,
                    double halfLife = DEFAULT_RECENCY_HALF_LIFE,
                    size_t minApps = 1,
                    size_t limit = 0,
                    std::string subject = "",
                    int unit = 0,
                    std::string topic = "",
                    bool clusterAgg = true,
                    ScoringWeights w = ScoringWeights())
        : referenceYear(refYear),
          recencyHalfLife(halfLife),
          minAppearancesForScoring(minApps),
          resultLimit(limit),
          subjectFilter(std::move(subject)),
          unitFilter(unit),
          topicFilter(std::move(topic)),
          enableClusterAggregation(clusterAgg),
          weights(w) {}

    bool isValid(std::string& errorMsg) const {
        if (referenceYear < 1900 || referenceYear > 2100) {
            errorMsg = "Invalid reference academic year: must be between 1900 and 2100.";
            return false;
        }
        if (recencyHalfLife <= 0.0) {
            errorMsg = "Invalid recency half-life: must be strictly positive.";
            return false;
        }
        if (!weights.isValid()) {
            errorMsg = "Invalid scoring weights: weights must be non-negative and sum to > 0.";
            return false;
        }
        return true;
    }
};

/**
 * @brief Breakdown of individual normalized component scores in [0.0, 1.0].
 */
struct ComponentScores {
    double frequencyScore;    // [0.0, 1.0] Normalized total appearance volume
    double recencyScore;      // [0.0, 1.0] Half-life decayed recency index
    double consistencyScore;  // [0.0, 1.0] Distribution across distinct academic years
    double marksScore;        // [0.0, 1.0] Examination marks weightage
    double trendScore;        // [0.0, 1.0] Recurrence dynamics bonus
    double combinedRawScore;  // [0.0, 1.0] Weighted composite score
};

/**
 * @brief Detailed historical-priority analysis result for an individual examination question.
 */
struct QuestionPriorityResult {
    std::string questionId;
    std::string questionText;
    std::string subject;
    int unitNumber;
    std::string topic;
    double priorityScore;     // [0.0, 100.0] Historical revision priority score
    ComponentScores components;
    ScoringWeights effectiveWeights;
    size_t totalAppearances;
    size_t distinctYearsCount;
    size_t distinctSessionsCount;
    int latestYear;
    int earliestYear;
    int totalMarks;
    double averageMarks;
    TrendAnalysisResult trend;
    bool isInsufficientEvidence;
    std::string insufficientEvidenceReason;
    std::string clusterId;
    size_t clusterSize;
    size_t clusterCombinedAppearances;
    std::string explanation;
};

/**
 * @brief Aggregated historical-priority result for a syllabus topic.
 */
struct TopicPriorityResult {
    std::string topicId;
    std::string topicName;
    std::string subject;
    int unitNumber;
    double topicPriorityScore;      // [0.0, 100.0] Composite topic revision priority
    double averageQuestionScore;    // Mean priority score of questions with evidence
    double topicRecurrenceScore;    // Evaluates topic aggregate appearance history
    size_t totalQuestionsStored;
    size_t questionsWithEvidence;
    double evidenceCoverageRatio;   // questionsWithEvidence / totalQuestionsStored
    size_t totalTopicAppearances;
    size_t distinctYearsCount;
    int latestYear;
    int earliestYear;
    int totalMarks;
    double averageMarks;
    TrendAnalysisResult trend;
    bool isInsufficientEvidence;
    std::vector<std::string> questionIds;
    std::string explanation;
};

/**
 * @brief Aggregated priority result for a question similarity cluster.
 */
struct ClusterPriorityResult {
    std::string clusterId;
    std::string representativeQuestionId;
    std::string representativeText;
    size_t memberCount;
    std::vector<std::string> memberQuestionIds;
    size_t combinedAppearances;
    size_t distinctYearsCount;
    int latestYear;
    double clusterPriorityScore;    // [0.0, 100.0]
    std::string explanation;
};

/**
 * @brief Explainable Historical Examination Priority Engine.
 * 
 * Evaluates past-year examination records across multi-dimensional criteria:
 * - Repeat appearance volume
 * - Exponential half-life recency decay
 * - Temporal consistency across distinct academic years
 * - Examination marks weightage
 * - Structured recurrence dynamics (consecutive vs periodic gap repeats)
 * - Paraphrased question cluster integration
 * 
 * IMPORTANT DISCLAIMER:
 * Priority scores reflect historical revision importance and curriculum emphasis.
 * They are NOT guaranteed predictions or calibrated probabilities of future exam content.
 */
class PYQPredictor {
public:
    // Saturation scaling constants for normalization
    static constexpr double FREQUENCY_SATURATION = 5.0;     // 5+ appearances saturates frequency score to 1.0
    static constexpr double CONSISTENCY_SATURATION = 4.0;    // 4+ distinct years saturates consistency score to 1.0
    static constexpr double MARKS_SATURATION = 15.0;         // 15+ marks saturates marks score to 1.0

    // ------------------------------------------------------------------------
    // Evaluation Logic for Individual Questions
    // ------------------------------------------------------------------------

    /**
     * @brief Computes explainable historical revision priority for a single question.
     */
    static QuestionPriorityResult evaluateQuestion(
        const Question& q,
        const PredictorConfig& config = PredictorConfig(),
        const std::vector<QuestionCluster>* clusters = nullptr) {

        std::string err;
        if (!config.isValid(err)) {
            throw std::invalid_argument("PredictorConfig validation error: " + err);
        }

        QuestionPriorityResult result;
        result.questionId = q.getId();
        result.questionText = q.getText();
        result.subject = q.getSubject();
        result.unitNumber = q.getUnitNumber();
        result.topic = q.getTopic();
        result.effectiveWeights = config.weights.normalized();
        result.priorityScore = 0.0;
        result.isInsufficientEvidence = false;
        result.clusterSize = 1;
        result.clusterCombinedAppearances = 0;

        auto clean = PYQAnalytics::deduplicateAppearances(q.getHistory());
        result.totalAppearances = clean.size();
        auto distinctYears = PYQAnalytics::extractDistinctYears(clean);
        result.distinctYearsCount = distinctYears.size();
        result.distinctSessionsCount = PYQAnalytics::extractDistinctSessions(clean).size();

        if (distinctYears.empty()) {
            result.latestYear = 0;
            result.earliestYear = 0;
        } else {
            result.latestYear = distinctYears.back();
            result.earliestYear = distinctYears.front();
        }

        result.totalMarks = 0;
        for (const auto& a : clean) {
            result.totalMarks += a.getMarks();
        }
        result.averageMarks = result.totalAppearances > 0
            ? static_cast<double>(result.totalMarks) / static_cast<double>(result.totalAppearances)
            : 0.0;

        result.trend = PYQAnalytics::analyzeTrend(clean, config.referenceYear);

        // 1. Check for Insufficient Evidence
        if (result.totalAppearances < config.minAppearancesForScoring) {
            result.isInsufficientEvidence = true;
            result.priorityScore = 0.0;
            result.components = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

            std::ostringstream oss;
            oss << "Insufficient historical evidence: Question [" << q.getId() << "] has "
                << result.totalAppearances << " recorded appearance(s) (minimum required: "
                << config.minAppearancesForScoring << "). Historical priority score assigned: 0.00.";
            result.insufficientEvidenceReason = oss.str();
            result.explanation = result.insufficientEvidenceReason;
            return result;
        }

        // 2. Compute Normalized Dimension Scores [0.0, 1.0]

        // Dimension A: Repeat Frequency (Saturated linearly up to FREQUENCY_SATURATION)
        double freqScore = std::min(1.0, static_cast<double>(result.totalAppearances) / FREQUENCY_SATURATION);

        // Dimension B: Recency-Weighted Score (Uses Task 3.4 half-life formula)
        double recScore = PYQAnalytics::calculateRecencyScore(clean, config.referenceYear, config.recencyHalfLife);

        // Dimension C: Distinct Year Consistency (Distribution across distinct academic years)
        double consScore = std::min(1.0, static_cast<double>(result.distinctYearsCount) / CONSISTENCY_SATURATION);

        // Dimension D: Exam Marks Weightage (Average marks relative to standard high-weight questions)
        double marksScore = std::min(1.0, result.averageMarks / MARKS_SATURATION);

        // Dimension E: Recurrence Trend Dynamics
        double trendScore = 0.0;
        switch (result.trend.classification) {
            case TrendClassification::STABLE_HIGH_FREQUENCY: trendScore = 1.00; break;
            case TrendClassification::CONSECUTIVE_REPEAT:    trendScore = 0.85; break;
            case TrendClassification::EMERGING_RECENT:       trendScore = 0.80; break;
            case TrendClassification::PERIODIC_GAP_REPEAT:   trendScore = 0.60; break;
            case TrendClassification::DORMANT:               trendScore = 0.20; break;
            case TrendClassification::INSUFFICIENT_DATA:     trendScore = 0.00; break;
            default:                                         trendScore = 0.00; break;
        }

        // 3. Cluster Paraphrase Integration
        if (config.enableClusterAggregation && clusters != nullptr) {
            for (const auto& cl : *clusters) {
                if (std::find(cl.memberQuestionIds.begin(), cl.memberQuestionIds.end(), q.getId()) != cl.memberQuestionIds.end()) {
                    result.clusterId = cl.clusterId;
                    result.clusterSize = cl.memberQuestionIds.size();
                    break;
                }
            }
        }

        // 4. Weighted Composite Score
        const auto& w = result.effectiveWeights;
        double rawScore = (w.frequencyWeight * freqScore) +
                          (w.recencyWeight * recScore) +
                          (w.consistencyWeight * consScore) +
                          (w.marksWeight * marksScore) +
                          (w.trendWeight * trendScore);

        if (rawScore < 0.0) rawScore = 0.0;
        if (rawScore > 1.0) rawScore = 1.0;

        result.components.frequencyScore = freqScore;
        result.components.recencyScore = recScore;
        result.components.consistencyScore = consScore;
        result.components.marksScore = marksScore;
        result.components.trendScore = trendScore;
        result.components.combinedRawScore = rawScore;

        result.priorityScore = std::round(rawScore * 10000.0) / 100.0; // Bounded in [0.0, 100.0]

        // 5. Generate Audit Explanation
        std::ostringstream oss;
        oss << std::fixed;
        oss << "Question [" << result.questionId << "] Revision Priority Score: "
            << std::setprecision(2) << result.priorityScore << "/100.0\n"
            << "  - Repeat Frequency    : " << result.totalAppearances << " appearance(s) (Score: "
            << std::setprecision(4) << freqScore << ", Weight: " << std::setprecision(2) << w.frequencyWeight << ")\n"
            << "  - Recency Index       : " << std::setprecision(4) << recScore << " relative to " << config.referenceYear
            << " (Weight: " << std::setprecision(2) << w.recencyWeight << ")\n"
            << "  - Year Consistency    : " << result.distinctYearsCount << " distinct year(s) (Score: "
            << std::setprecision(4) << consScore << ", Weight: " << std::setprecision(2) << w.consistencyWeight << ")\n"
            << "  - Marks Weightage     : " << std::setprecision(2) << result.averageMarks << " avg marks (Score: "
            << std::setprecision(4) << marksScore << ", Weight: " << std::setprecision(2) << w.marksWeight << ")\n"
            << "  - Recurrence Trend    : " << result.trend.classificationName << " (Score: "
            << std::setprecision(4) << trendScore << ", Weight: " << std::setprecision(2) << w.trendWeight << ")\n";

        if (!result.clusterId.empty() && result.clusterSize > 1) {
            oss << "  - Similarity Cluster  : " << result.clusterId << " with " << result.clusterSize
                << " paraphrased variants";
            if (result.clusterCombinedAppearances > 0) {
                oss << " (" << result.clusterCombinedAppearances << " combined appearances)";
            }
            oss << ".\n";
        }

        oss << "  - Interpretation      : Higher relative historical priority; NOT a calibrated probability.";
        result.explanation = oss.str();

        return result;
    }

    /**
     * @brief Derives the reference academic year deterministically from a question collection.
     * Returns the maximum recorded academic year across all questions,
     * or DEFAULT_REFERENCE_YEAR if no appearances are recorded.
     */
    static int deriveReferenceYear(const std::vector<Question>& questions) {
        int maxYear = 0;
        for (const auto& q : questions) {
            for (const auto& a : q.getHistory()) {
                if (a.getAcademicYear() > maxYear && a.getAcademicYear() <= 2100) {
                    maxYear = a.getAcademicYear();
                }
            }
        }
        return (maxYear >= 1900) ? maxYear : PredictorConfig::DEFAULT_REFERENCE_YEAR;
    }

    // ------------------------------------------------------------------------
    // Question Ranking & Filtering
    // ------------------------------------------------------------------------

    /**
     * @brief Evaluates, ranks, and filters questions deterministically by revision priority.
     */
    static std::vector<QuestionPriorityResult> rankQuestions(
        const std::vector<Question>& questions,
        const PredictorConfig& config = PredictorConfig(),
        const SyllabusTree* tree = nullptr) {

        (void)tree;
        if (questions.empty()) return {};

        std::string err;
        if (!config.isValid(err)) {
            throw std::invalid_argument("PredictorConfig validation error: " + err);
        }

        // Build clusters if cluster aggregation enabled
        std::vector<QuestionCluster> clusters;
        if (config.enableClusterAggregation) {
            clusters = QuestionClusterer::clusterQuestions(questions);
        }

        std::vector<QuestionPriorityResult> results;
        results.reserve(questions.size());

        for (const auto& q : questions) {
            // Apply Scope Filtering
            if (!config.subjectFilter.empty() && q.getSubject() != config.subjectFilter) {
                continue;
            }
            if (config.unitFilter > 0 && q.getUnitNumber() != config.unitFilter) {
                continue;
            }
            if (!config.topicFilter.empty() && q.getTopic() != config.topicFilter) {
                continue;
            }

            auto res = evaluateQuestion(q, config, config.enableClusterAggregation ? &clusters : nullptr);

            // Populate combined cluster appearances if cluster aggregation is active
            if (config.enableClusterAggregation && !res.clusterId.empty() && res.clusterSize > 1) {
                for (const auto& cl : clusters) {
                    if (cl.clusterId == res.clusterId) {
                        std::vector<ExamAppearance> allClApps;
                        for (const auto& mId : cl.memberQuestionIds) {
                            auto it = std::find_if(questions.begin(), questions.end(),
                                [&mId](const Question& item) { return item.getId() == mId; });
                            if (it != questions.end()) {
                                auto cleanApps = PYQAnalytics::deduplicateAppearances(it->getHistory());
                                for (const auto& ca : cleanApps) allClApps.push_back(ca);
                            }
                        }
                        res.clusterCombinedAppearances = PYQAnalytics::deduplicateAppearances(allClApps).size();
                        break;
                    }
                }
            }

            results.push_back(std::move(res));
        }

        // Deterministic Ranking Sort
        // 1. priorityScore (descending)
        // 2. recencyScore (descending)
        // 3. distinctYearsCount (descending)
        // 4. totalAppearances (descending)
        // 5. latestYear (descending)
        // 6. questionId (ascending lexicographical)
        std::sort(results.begin(), results.end(), [](const QuestionPriorityResult& a, const QuestionPriorityResult& b) {
            if (std::abs(a.priorityScore - b.priorityScore) > 0.001) {
                return a.priorityScore > b.priorityScore;
            }
            if (std::abs(a.components.recencyScore - b.components.recencyScore) > 0.001) {
                return a.components.recencyScore > b.components.recencyScore;
            }
            if (a.distinctYearsCount != b.distinctYearsCount) {
                return a.distinctYearsCount > b.distinctYearsCount;
            }
            if (a.totalAppearances != b.totalAppearances) {
                return a.totalAppearances > b.totalAppearances;
            }
            if (a.latestYear != b.latestYear) {
                return a.latestYear > b.latestYear;
            }
            return a.questionId < b.questionId;
        });

        // Apply Result Limit
        if (config.resultLimit > 0 && results.size() > config.resultLimit) {
            results.resize(config.resultLimit);
        }

        return results;
    }

    // ------------------------------------------------------------------------
    // Topic-Level Priority Analysis & Ranking
    // ------------------------------------------------------------------------

    /**
     * @brief Computes topic-level priority ranking accounting for question-count bias.
     */
    static TopicPriorityResult evaluateTopic(
        const std::string& topicNameOrId,
        const std::vector<Question>& questions,
        const PredictorConfig& config = PredictorConfig(),
        const SyllabusTree* tree = nullptr) {

        TopicPriorityResult result;
        result.topicId = topicNameOrId;
        result.topicName = topicNameOrId;
        result.subject = "";
        result.unitNumber = 0;
        result.topicPriorityScore = 0.0;
        result.averageQuestionScore = 0.0;
        result.topicRecurrenceScore = 0.0;
        result.totalQuestionsStored = 0;
        result.questionsWithEvidence = 0;
        result.evidenceCoverageRatio = 0.0;
        result.totalTopicAppearances = 0;
        result.distinctYearsCount = 0;
        result.latestYear = 0;
        result.earliestYear = 0;
        result.totalMarks = 0;
        result.averageMarks = 0.0;
        result.isInsufficientEvidence = false;

        std::vector<std::string> treeMatchingQIds;
        if (tree != nullptr) {
            treeMatchingQIds = tree->getQuestionsForTopic(topicNameOrId);
        }

        std::vector<ExamAppearance> allTopicApps;
        std::vector<QuestionPriorityResult> questionScores;

        for (const auto& q : questions) {
            bool matches = false;
            if (!treeMatchingQIds.empty()) {
                matches = std::find(treeMatchingQIds.begin(), treeMatchingQIds.end(), q.getId()) != treeMatchingQIds.end();
            } else {
                matches = (q.getTopic() == topicNameOrId);
            }

            if (matches) {
                result.totalQuestionsStored++;
                result.questionIds.push_back(q.getId());
                if (result.subject.empty()) result.subject = q.getSubject();
                if (result.unitNumber == 0) result.unitNumber = q.getUnitNumber();

                auto clean = PYQAnalytics::deduplicateAppearances(q.getHistory());
                if (!clean.empty()) {
                    result.questionsWithEvidence++;
                    for (const auto& app : clean) {
                        allTopicApps.push_back(app);
                        result.totalMarks += app.getMarks();
                    }
                    questionScores.push_back(evaluateQuestion(q, config));
                }
            }
        }

        auto cleanTopicApps = PYQAnalytics::deduplicateAppearances(allTopicApps);
        result.totalTopicAppearances = cleanTopicApps.size();
        auto distinctYears = PYQAnalytics::extractDistinctYears(cleanTopicApps);
        result.distinctYearsCount = distinctYears.size();

        if (!distinctYears.empty()) {
            result.latestYear = distinctYears.back();
            result.earliestYear = distinctYears.front();
        }

        result.averageMarks = result.totalTopicAppearances > 0
            ? static_cast<double>(result.totalMarks) / static_cast<double>(result.totalTopicAppearances)
            : 0.0;

        result.trend = PYQAnalytics::analyzeTrend(cleanTopicApps, config.referenceYear);

        if (result.totalQuestionsStored == 0 || result.totalTopicAppearances < config.minAppearancesForScoring) {
            result.isInsufficientEvidence = true;
            result.topicPriorityScore = 0.0;
            result.evidenceCoverageRatio = 0.0;

            std::ostringstream oss;
            oss << "Insufficient topic evidence: Topic [" << topicNameOrId << "] has "
                << result.totalTopicAppearances << " appearance(s) across "
                << result.totalQuestionsStored << " question(s). Priority score: 0.00.";
            result.explanation = oss.str();
            return result;
        }

        result.evidenceCoverageRatio = static_cast<double>(result.questionsWithEvidence) /
                                       static_cast<double>(result.totalQuestionsStored);

        // 1. Mean Question Priority Score
        double sumQScore = 0.0;
        for (const auto& qs : questionScores) {
            sumQScore += qs.priorityScore;
        }
        result.averageQuestionScore = !questionScores.empty()
            ? sumQScore / static_cast<double>(questionScores.size())
            : 0.0;

        // 2. Topic Aggregate Recurrence Score (Prevents question-count inflation)
        double topicFreq = std::min(1.0, static_cast<double>(result.totalTopicAppearances) / (FREQUENCY_SATURATION * 1.5));
        double topicRecency = PYQAnalytics::calculateRecencyScore(cleanTopicApps, config.referenceYear, config.recencyHalfLife);
        double topicCons = std::min(1.0, static_cast<double>(result.distinctYearsCount) / CONSISTENCY_SATURATION);
        double topicMarks = std::min(1.0, result.averageMarks / MARKS_SATURATION);
        double topicTrendScore = 0.0;
        switch (result.trend.classification) {
            case TrendClassification::STABLE_HIGH_FREQUENCY: topicTrendScore = 1.00; break;
            case TrendClassification::CONSECUTIVE_REPEAT:    topicTrendScore = 0.85; break;
            case TrendClassification::EMERGING_RECENT:       topicTrendScore = 0.80; break;
            case TrendClassification::PERIODIC_GAP_REPEAT:   topicTrendScore = 0.60; break;
            case TrendClassification::DORMANT:               topicTrendScore = 0.20; break;
            case TrendClassification::INSUFFICIENT_DATA:     topicTrendScore = 0.00; break;
            default:                                         topicTrendScore = 0.00; break;
        }

        const auto& w = config.weights.normalized();
        double topicRaw = (w.frequencyWeight * topicFreq) +
                          (w.recencyWeight * topicRecency) +
                          (w.consistencyWeight * topicCons) +
                          (w.marksWeight * topicMarks) +
                          (w.trendWeight * topicTrendScore);

        result.topicRecurrenceScore = std::round(topicRaw * 10000.0) / 100.0;

        // Balanced Topic Score: 50% Mean Question Score + 50% Topic Recurrence Score
        // multiplied by square-root of coverage ratio to gently temper sparse data
        double coverageFactor = 0.50 + (0.50 * result.evidenceCoverageRatio);
        double balanced = ((0.50 * result.averageQuestionScore) + (0.50 * result.topicRecurrenceScore)) * coverageFactor;
        result.topicPriorityScore = std::round(balanced * 100.0) / 100.0;
        if (result.topicPriorityScore > 100.0) result.topicPriorityScore = 100.0;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "Topic [" << result.topicName << "] Revision Priority Score: "
            << result.topicPriorityScore << "/100.0\n"
            << "  - Stored Questions     : " << result.totalQuestionsStored << " ("
            << result.questionsWithEvidence << " with recorded exam evidence, Coverage: "
            << (result.evidenceCoverageRatio * 100.0) << "%)\n"
            << "  - Avg Question Score   : " << result.averageQuestionScore << "/100.0\n"
            << "  - Topic Aggregate Score: " << result.topicRecurrenceScore << "/100.0 ("
            << result.totalTopicAppearances << " total appearances across "
            << result.distinctYearsCount << " distinct years)\n"
            << "  - Topic Trend          : " << result.trend.classificationName;
        result.explanation = oss.str();

        return result;
    }

    /**
     * @brief Computes, ranks, and filters topic-level priority across all topics.
     */
    static std::vector<TopicPriorityResult> rankTopics(
        const std::vector<Question>& questions,
        const PredictorConfig& config = PredictorConfig(),
        const SyllabusTree* tree = nullptr) {

        if (questions.empty()) return {};

        std::vector<std::string> topicNames;
        for (const auto& q : questions) {
            if (!config.subjectFilter.empty() && q.getSubject() != config.subjectFilter) {
                continue;
            }
            if (config.unitFilter > 0 && q.getUnitNumber() != config.unitFilter) {
                continue;
            }
            if (!q.getTopic().empty()) {
                if (std::find(topicNames.begin(), topicNames.end(), q.getTopic()) == topicNames.end()) {
                    topicNames.push_back(q.getTopic());
                }
            }
        }

        std::sort(topicNames.begin(), topicNames.end());

        std::vector<TopicPriorityResult> results;
        results.reserve(topicNames.size());
        for (const auto& tName : topicNames) {
            results.push_back(evaluateTopic(tName, questions, config, tree));
        }

        // Rank topics deterministically:
        // 1. topicPriorityScore (descending)
        // 2. averageQuestionScore (descending)
        // 3. distinctYearsCount (descending)
        // 4. totalTopicAppearances (descending)
        // 5. topicName (ascending lexicographically)
        std::sort(results.begin(), results.end(), [](const TopicPriorityResult& a, const TopicPriorityResult& b) {
            if (std::abs(a.topicPriorityScore - b.topicPriorityScore) > 0.001) {
                return a.topicPriorityScore > b.topicPriorityScore;
            }
            if (std::abs(a.averageQuestionScore - b.averageQuestionScore) > 0.001) {
                return a.averageQuestionScore > b.averageQuestionScore;
            }
            if (a.distinctYearsCount != b.distinctYearsCount) {
                return a.distinctYearsCount > b.distinctYearsCount;
            }
            if (a.totalTopicAppearances != b.totalTopicAppearances) {
                return a.totalTopicAppearances > b.totalTopicAppearances;
            }
            return a.topicName < b.topicName;
        });

        if (config.resultLimit > 0 && results.size() > config.resultLimit) {
            results.resize(config.resultLimit);
        }

        return results;
    }

    // ------------------------------------------------------------------------
    // Cluster Priority Ranking
    // ------------------------------------------------------------------------

    /**
     * @brief Computes combined priority scores for paraphrase clusters.
     */
    static std::vector<ClusterPriorityResult> rankClusters(
        const std::vector<Question>& questions,
        const PredictorConfig& config = PredictorConfig()) {

        if (questions.empty()) return {};

        auto clusters = QuestionClusterer::clusterQuestions(questions);
        std::vector<ClusterPriorityResult> results;
        results.reserve(clusters.size());

        for (const auto& cl : clusters) {
            ClusterPriorityResult cRes;
            cRes.clusterId = cl.clusterId;
            cRes.representativeQuestionId = cl.representativeId;
            cRes.representativeText = cl.representativeText;
            cRes.memberCount = cl.memberQuestionIds.size();
            cRes.memberQuestionIds = cl.memberQuestionIds;

            // Combine appearances across all questions in the cluster
            std::vector<ExamAppearance> allClusterApps;
            for (const auto& mId : cl.memberQuestionIds) {
                auto it = std::find_if(questions.begin(), questions.end(),
                    [&mId](const Question& q) { return q.getId() == mId; });
                if (it != questions.end()) {
                    auto clean = PYQAnalytics::deduplicateAppearances(it->getHistory());
                    for (const auto& a : clean) {
                        allClusterApps.push_back(a);
                    }
                }
            }

            auto cleanClusterApps = PYQAnalytics::deduplicateAppearances(allClusterApps);
            cRes.combinedAppearances = cleanClusterApps.size();
            auto distinctYears = PYQAnalytics::extractDistinctYears(cleanClusterApps);
            cRes.distinctYearsCount = distinctYears.size();
            cRes.latestYear = distinctYears.empty() ? 0 : distinctYears.back();

            // Compute priority score for cluster
            double freqScore = std::min(1.0, static_cast<double>(cRes.combinedAppearances) / FREQUENCY_SATURATION);
            double recScore = PYQAnalytics::calculateRecencyScore(cleanClusterApps, config.referenceYear, config.recencyHalfLife);
            double consScore = std::min(1.0, static_cast<double>(cRes.distinctYearsCount) / CONSISTENCY_SATURATION);

            const auto& w = config.weights.normalized();
            double raw = (w.frequencyWeight * freqScore) +
                         (w.recencyWeight * recScore) +
                         (w.consistencyWeight * consScore);

            cRes.clusterPriorityScore = std::round(raw * 10000.0) / 100.0;

            std::ostringstream oss;
            oss << "Cluster [" << cRes.clusterId << "] Priority: " << cRes.clusterPriorityScore
                << " (" << cRes.memberCount << " paraphrased variants, "
                << cRes.combinedAppearances << " combined appearances across "
                << cRes.distinctYearsCount << " years).";
            cRes.explanation = oss.str();

            results.push_back(std::move(cRes));
        }

        std::sort(results.begin(), results.end(), [](const ClusterPriorityResult& a, const ClusterPriorityResult& b) {
            if (std::abs(a.clusterPriorityScore - b.clusterPriorityScore) > 0.001) {
                return a.clusterPriorityScore > b.clusterPriorityScore;
            }
            if (a.combinedAppearances != b.combinedAppearances) {
                return a.combinedAppearances > b.combinedAppearances;
            }
            return a.clusterId < b.clusterId;
        });

        return results;
    }

    // ------------------------------------------------------------------------
    // Explainable Ranking Comparisons
    // ------------------------------------------------------------------------

    /**
     * @brief Explains why Question A ranked higher than Question B with full evidence transparency.
     */
    static std::string explainRankingComparison(
        const QuestionPriorityResult& a,
        const QuestionPriorityResult& b) {

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "=== Ranking Comparison Report ===\n"
            << "Item A [" << a.questionId << "]: Score = " << a.priorityScore << "/100.0\n"
            << "Item B [" << b.questionId << "]: Score = " << b.priorityScore << "/100.0\n";

        double delta = a.priorityScore - b.priorityScore;
        if (std::abs(delta) < 0.001) {
            oss << "Comparison: Items have identical priority scores. Tie broken deterministically by:\n"
                << "  - Recency: " << a.components.recencyScore << " vs " << b.components.recencyScore << "\n"
                << "  - Distinct Years: " << a.distinctYearsCount << " vs " << b.distinctYearsCount << "\n"
                << "  - Total Appearances: " << a.totalAppearances << " vs " << b.totalAppearances << "\n"
                << "  - Question ID: " << a.questionId << " vs " << b.questionId << "\n";
        } else if (delta > 0) {
            oss << "Verdict: [" << a.questionId << "] ranks HIGHER (+ " << delta << " points) due to:\n";
            if (a.components.recencyScore > b.components.recencyScore) {
                oss << "  - Higher recency index: " << a.components.recencyScore
                    << " vs " << b.components.recencyScore << "\n";
            }
            if (a.totalAppearances > b.totalAppearances) {
                oss << "  - Greater appearance volume: " << a.totalAppearances
                    << " vs " << b.totalAppearances << "\n";
            }
            if (a.distinctYearsCount > b.distinctYearsCount) {
                oss << "  - Broader academic year distribution: " << a.distinctYearsCount
                    << " vs " << b.distinctYearsCount << "\n";
            }
            if (a.averageMarks > b.averageMarks) {
                oss << "  - Higher average examination marks: " << a.averageMarks
                    << " vs " << b.averageMarks << "\n";
            }
            if (a.components.trendScore > b.components.trendScore) {
                oss << "  - Stronger recurrence trend: " << a.trend.classificationName
                    << " vs " << b.trend.classificationName << "\n";
            }
        } else {
            oss << "Verdict: [" << b.questionId << "] ranks HIGHER (+ " << (-delta) << " points).\n";
        }

        return oss.str();
    }

    /**
     * @brief Explains why Topic A ranked higher than Topic B.
     */
    static std::string explainTopicComparison(
        const TopicPriorityResult& a,
        const TopicPriorityResult& b) {

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "=== Topic Ranking Comparison ===\n"
            << "Topic A [" << a.topicName << "]: Score = " << a.topicPriorityScore << "/100.0\n"
            << "Topic B [" << b.topicName << "]: Score = " << b.topicPriorityScore << "/100.0\n";

        double delta = a.topicPriorityScore - b.topicPriorityScore;
        if (delta > 0) {
            oss << "Topic A ranks HIGHER (+ " << delta << " points) due to:\n"
                << "  - Average Question Score: " << a.averageQuestionScore << " vs " << b.averageQuestionScore << "\n"
                << "  - Topic Aggregate Recurrence: " << a.topicRecurrenceScore << " vs " << b.topicRecurrenceScore << "\n"
                << "  - Total Appearances: " << a.totalTopicAppearances << " vs " << b.totalTopicAppearances << "\n"
                << "  - Evidence Coverage: " << (a.evidenceCoverageRatio * 100.0) << "% vs " << (b.evidenceCoverageRatio * 100.0) << "%\n";
        } else {
            oss << "Topic B ranks HIGHER (+ " << (-delta) << " points).\n";
        }

        return oss.str();
    }
};

} // namespace studysync

#endif // STUDYSYNC_PYQ_PREDICTOR_HPP
