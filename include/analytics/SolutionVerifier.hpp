#ifndef STUDYSYNC_SOLUTION_VERIFIER_HPP
#define STUDYSYNC_SOLUTION_VERIFIER_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <utility>

#include "models/Question.hpp"
#include "algorithms/QuestionSimilarity.hpp"

namespace studysync {

/**
 * @brief Structured outcome of reference-solution-based answer evaluation.
 */
struct VerificationResult {
    std::string questionId;
    bool hasReferenceSolution;          // True if ReferenceSolution contains usable criteria
    bool isInsufficientEvidence;        // True if reference solution or submission is empty
    std::string insufficientEvidenceReason;

    double overallScore;                // [0.0, 100.0] Weighted composite score
    double keyTermScore;                // [0.0, 100.0] Mandatory key terms coverage
    double structuralScore;             // [0.0, 100.0] Structural elements coverage
    double rubricCoverageScore;         // [0.0, 100.0] Reference model answer vocabulary coverage
    double numericalFormulaScore;       // [0.0, 100.0] Expected numerical/formula result score

    std::vector<std::string> matchedKeyTerms;
    std::vector<std::string> missingKeyTerms;

    std::vector<std::string> matchedStructuralKeywords;
    std::vector<std::string> missingStructuralKeywords;

    bool hasNumericalOrFormulaReference;
    bool numericalOrFormulaMatched;
    std::string expectedNumericalOrFormula;
    std::string detectedNumericalOrFormula;

    std::vector<std::string> feedbackPoints;
    std::string summaryFeedback;
    std::string disclaimer;
};

/**
 * @brief Deterministic, explainable reference-based answer verification engine.
 * 
 * Compares student-submitted answers against stored ReferenceSolution rubrics:
 * 1. Whole-token boundary matching for mandatory key terms (prevents partial-word false positives).
 * 2. Structural keyword verification (algorithm steps, asymptotic complexity, base cases).
 * 3. Model answer vocabulary coverage (content token recall).
 * 4. Numerical and algebraic formula validation with numeric tolerance.
 * 5. Dynamic weight redistribution across defined rubric criteria.
 * 
 * DISCLAIMER:
 * This is an educational feedback and revision tool. Deterministic matching detects
 * the presence of expected terminology, structural milestones, and numerical answers,
 * but does not constitute an authoritative qualitative or semantic evaluation.
 */
class SolutionVerifier {
public:
    // Base component weights when all criteria are defined
    static constexpr double BASE_WEIGHT_KEY_TERMS = 0.45;
    static constexpr double BASE_WEIGHT_STRUCTURAL = 0.25;
    static constexpr double BASE_WEIGHT_RUBRIC_TEXT = 0.15;
    static constexpr double BASE_WEIGHT_NUMERICAL = 0.15;

    // Default numerical tolerance constants
    static constexpr double DEFAULT_ABS_TOLERANCE = 0.001; // 1e-3 absolute tolerance for near-zero values
    static constexpr double DEFAULT_REL_TOLERANCE = 0.001; // 1e-3 (0.1%) relative tolerance for large magnitudes

    static inline const std::string VERIFIER_DISCLAIMER =
        "DISCLAIMER: This automated verification engine evaluates answers deterministically "
        "against stored reference rubrics, key terminology, and numerical targets. It is an "
        "educational self-study tool and does not provide an authoritative or semantic assessment.";

    // ------------------------------------------------------------------------
    // Numerical & Mathematical Comparison Helpers
    // ------------------------------------------------------------------------

    /**
     * @brief Evaluates whether two floating-point numbers match within combined
     * absolute and relative tolerances:
     *   |submitted - expected| <= max(absTol, relTol * max(|submitted|, |expected|))
     * 
     * Handles zero, negative values, small numbers, and large magnitudes safely.
     * Prevents NaN or infinity from accidentally passing.
     * 
     * @throws std::invalid_argument if absTol or relTol are negative or non-finite.
     */
    static bool isNumericallyClose(
        double submitted,
        double expected,
        double absTol = DEFAULT_ABS_TOLERANCE,
        double relTol = DEFAULT_REL_TOLERANCE) {

        if (!std::isfinite(absTol) || !std::isfinite(relTol) || absTol < 0.0 || relTol < 0.0) {
            throw std::invalid_argument("Tolerance values must be finite and non-negative.");
        }

        // Non-finite values (NaN, Inf) never match
        if (!std::isfinite(submitted) || !std::isfinite(expected)) {
            return false;
        }

        double diff = std::abs(submitted - expected);
        if (!std::isfinite(diff)) {
            return false;
        }

        double maxMag = std::max(std::abs(submitted), std::abs(expected));
        double allowed = std::max(absTol, relTol * maxMag);

        return diff <= allowed;
    }

    /**
     * @brief Attempts to parse a string as a pure, finite floating-point number.
     * Disallows NaN, infinity, empty strings, and trailing unparsed non-whitespace text.
     */
    static bool tryParseNumber(const std::string& str, double& val) {
        std::string trimmed = TextNormalizer::trim(str);
        if (trimmed.empty()) return false;

        // Disallow explicit NaN / Infinity literals
        std::string lower = TextNormalizer::toLower(trimmed);
        if (lower.find("nan") != std::string::npos ||
            lower.find("inf") != std::string::npos) {
            return false;
        }

        try {
            size_t idx = 0;
            val = std::stod(trimmed, &idx);
            // Verify remainder contains only whitespace
            while (idx < trimmed.size() && std::isspace(static_cast<unsigned char>(trimmed[idx]))) {
                idx++;
            }
            if (idx != trimmed.size()) return false;
            return std::isfinite(val);
        } catch (...) {
            return false;
        }
    }

    /**
     * @brief Checks if a formula expression is structurally well-formed:
     * - Non-empty
     * - Parentheses, brackets, and braces are balanced and non-empty
     * - Does not begin or end with dangling binary operators (*, /, ^, =)
     * - Contains at least one alphanumeric operand
     */
    static bool isWellFormedFormula(const std::string& str) {
        std::string trimmed = TextNormalizer::trim(str);
        if (trimmed.empty()) return false;

        std::vector<char> stack;
        char prevNonWs = '\0';
        bool hasOperand = false;

        for (size_t i = 0; i < trimmed.size(); ++i) {
            char c = trimmed[i];
            if (std::isspace(static_cast<unsigned char>(c))) continue;

            if (c == '(' || c == '[' || c == '{') {
                stack.push_back(c);
            } else if (c == ')' || c == ']' || c == '}') {
                if (stack.empty()) return false;
                char top = stack.back();
                stack.pop_back();
                if ((c == ')' && top != '(') ||
                    (c == ']' && top != '[') ||
                    (c == '}' && top != '{')) {
                    return false;
                }
                // Reject empty parentheses: e.g. "()"
                if (prevNonWs == '(' || prevNonWs == '[' || prevNonWs == '{') {
                    return false;
                }
            } else if (std::isalnum(static_cast<unsigned char>(c))) {
                hasOperand = true;
            }

            prevNonWs = c;
        }

        if (!stack.empty() || !hasOperand) return false;

        // Check dangling binary operators at beginning or end
        char firstNonWs = '\0';
        char lastNonWs = '\0';
        for (char c : trimmed) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                if (firstNonWs == '\0') firstNonWs = c;
                lastNonWs = c;
            }
        }

        if (firstNonWs == '*' || firstNonWs == '/' || firstNonWs == '^' || firstNonWs == '=') {
            return false;
        }
        if (lastNonWs == '+' || lastNonWs == '-' || lastNonWs == '*' ||
            lastNonWs == '/' || lastNonWs == '^' || lastNonWs == '=') {
            return false;
        }

        return true;
    }

    /**
     * @brief Normalizes a formula or algebraic expression:
     * - Strips all irrelevant whitespace
     * - Preserves meaningful operators (+, -, *, /, ^, =, <, >)
     * - Preserves meaningful parentheses, exponents, decimal points, and numeric literals
     * - Canonicalizes standard function names (log, sqrt, sin, cos, tan, exp, ln) and asymptotic O(...)
     * - Preserves variable casing (e.g. A vs a, X vs x)
     */
    static std::string normalizeFormula(const std::string& str) {
        std::string trimmed = TextNormalizer::trim(str);
        if (trimmed.empty()) return "";

        // Remove whitespace
        std::string noWs;
        noWs.reserve(trimmed.size());
        for (char c : trimmed) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                noWs.push_back(c);
            }
        }

        // Canonicalize standard mathematical functions and asymptotic notation
        // while preserving variable casing.
        static const std::vector<std::string> stdFuncs = {
            "sqrt", "log", "sin", "cos", "tan", "exp", "ln"
        };

        std::string result;
        result.reserve(noWs.size());

        for (size_t i = 0; i < noWs.size(); ) {
            // Check for Big-O / asymptotic notation: 'O(' or 'o('
            if ((noWs[i] == 'O' || noWs[i] == 'o') && i + 1 < noWs.size() && noWs[i + 1] == '(') {
                result.push_back('O');
                result.push_back('(');
                i += 2;
                continue;
            }

            // Check standard function names
            bool matchedFunc = false;
            for (const auto& fn : stdFuncs) {
                if (i + fn.size() <= noWs.size()) {
                    std::string sub = noWs.substr(i, fn.size());
                    if (TextNormalizer::toLower(sub) == fn) {
                        // Ensure followed by '(' or non-alpha delimiter
                        if (i + fn.size() == noWs.size() ||
                            !std::isalpha(static_cast<unsigned char>(noWs[i + fn.size()]))) {
                            result += fn;
                            i += fn.size();
                            matchedFunc = true;
                            break;
                        }
                    }
                }
            }
            if (matchedFunc) continue;

            result.push_back(noWs[i]);
            i++;
        }

        return result;
    }

    /**
     * @brief Searches for an isolated formula in submitted text matching normExpected.
     * Prevents partial-expression false matches (e.g. 'x/2' will NOT match inside 'x/20' or 'x/2 + 5').
     */
    static bool findFormulaInText(const std::string& text, const std::string& normExpected) {
        if (text.empty() || normExpected.empty()) return false;

        size_t expectedNonWsCount = normExpected.size();

        for (size_t i = 0; i < text.size(); ++i) {
            // Left boundary check: start of string, whitespace, open bracket/quote/colon/equals
            bool leftBoundary = (i == 0) ||
                                std::isspace(static_cast<unsigned char>(text[i - 1])) ||
                                text[i - 1] == '(' || text[i - 1] == '[' || text[i - 1] == '{' ||
                                text[i - 1] == ':' || text[i - 1] == '=' || text[i - 1] == '"' || text[i - 1] == '\'';

            if (!leftBoundary) continue;

            // Collect expectedNonWsCount non-whitespace characters from position i
            size_t nonWsCount = 0;
            size_t j = i;
            while (j < text.size() && nonWsCount < expectedNonWsCount) {
                if (!std::isspace(static_cast<unsigned char>(text[j]))) {
                    nonWsCount++;
                }
                j++;
            }

            if (nonWsCount != expectedNonWsCount) break;

            // Check if slice normalizes to normExpected
            std::string candidateSlice = text.substr(i, j - i);
            if (normalizeFormula(candidateSlice) == normExpected) {
                // Right boundary check: end of string, whitespace, or closing-delimiter
                bool rightBoundary = (j == text.size());
                if (!rightBoundary) {
                    char nextChar = text[j];
                    if (std::isspace(static_cast<unsigned char>(nextChar))) {
                        // Scan past whitespace to verify the expression does not continue
                        size_t k = j;
                        while (k < text.size() && std::isspace(static_cast<unsigned char>(text[k]))) {
                            k++;
                        }
                        if (k == text.size()) {
                            rightBoundary = true;
                        } else {
                            char kChar = text[k];
                            bool isMathOp = (kChar == '+' || kChar == '-' || kChar == '*' ||
                                             kChar == '/' || kChar == '^' || kChar == '=');
                            rightBoundary = !isMathOp;
                        }
                    } else if (nextChar == '.' || nextChar == ',' || nextChar == ';' ||
                               nextChar == ')' || nextChar == ']' || nextChar == '}' ||
                               nextChar == '?' || nextChar == '!' || nextChar == '"' || nextChar == '\'') {
                        rightBoundary = true;
                    }
                }

                if (rightBoundary) {
                    return true;
                }
            }
        }

        return false;
    }

    // ------------------------------------------------------------------------
    // Text Matching & Token Helpers
    // ------------------------------------------------------------------------

    /**
     * @brief Checks if a keyword or multi-word phrase appears as distinct whole tokens in submitted text.
     * Prevents partial-word false matches (e.g. 'sort' will NOT match inside 'resort' or 'assortment').
     */
    static bool isKeywordPresent(const std::string& keyword, const std::vector<std::string>& submittedTokens) {
        if (keyword.empty() || submittedTokens.empty()) return false;

        // Tokenize keyword without stopword filtering to preserve exact phrase tokens
        auto kwTokens = TextNormalizer::tokenize(keyword, false);
        if (kwTokens.empty()) return false;

        // Search for the contiguous sub-sequence of tokens in submittedTokens
        auto it = std::search(submittedTokens.begin(), submittedTokens.end(),
                              kwTokens.begin(), kwTokens.end());
        return it != submittedTokens.end();
    }

    /**
     * @brief Verifies expected numerical answer or formula against submitted text.
     */
    static bool checkNumericalOrFormula(
        const std::string& expectedRef,
        const std::string& submittedAnswer,
        std::string& detectedValue,
        double absTol = DEFAULT_ABS_TOLERANCE,
        double relTol = DEFAULT_REL_TOLERANCE) {

        detectedValue = "";
        std::string cleanRef = TextNormalizer::trim(expectedRef);
        std::string cleanSub = TextNormalizer::trim(submittedAnswer);
        if (cleanRef.empty() || cleanSub.empty()) return false;

        double expectedNum = 0.0;
        if (tryParseNumber(cleanRef, expectedNum)) {
            // Case A: Pure Numerical Comparison
            // 1. Direct check of whole submitted answer
            double subNum = 0.0;
            if (tryParseNumber(cleanSub, subNum)) {
                if (isNumericallyClose(subNum, expectedNum, absTol, relTol)) {
                    detectedValue = cleanSub;
                    return true;
                }
            }

            // 2. Scan submitted text for number candidates with boundary awareness
            std::string current;
            for (size_t i = 0; i <= submittedAnswer.size(); ++i) {
                char c = (i < submittedAnswer.size()) ? submittedAnswer[i] : ' ';
                bool isNumChar = std::isdigit(static_cast<unsigned char>(c)) ||
                                 c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E';

                if (isNumChar) {
                    current += c;
                } else {
                    if (!current.empty()) {
                        // Strip trailing punctuation like '.' or ',' from sentence boundaries
                        while (!current.empty() && (current.back() == '.' || current.back() == ',' || current.back() == ';')) {
                            current.pop_back();
                        }
                        double candidate = 0.0;
                        if (tryParseNumber(current, candidate)) {
                            if (isNumericallyClose(candidate, expectedNum, absTol, relTol)) {
                                detectedValue = current;
                                return true;
                            }
                        }
                        current.clear();
                    }
                }
            }
            return false;
        }

        // Case B: Formula / Complexity / Algebraic Expression
        // Reject malformed or empty reference formula
        if (!isWellFormedFormula(cleanRef)) {
            return false;
        }

        std::string normExpected = normalizeFormula(cleanRef);
        if (normExpected.empty()) return false;

        // 1. Direct match: entire submitted answer is the formula
        if (isWellFormedFormula(cleanSub)) {
            std::string normDirect = normalizeFormula(cleanSub);
            if (normDirect == normExpected) {
                detectedValue = cleanSub;
                return true;
            }
        }

        // 2. Substring match within sentences with boundary verification
        if (findFormulaInText(submittedAnswer, normExpected)) {
            detectedValue = cleanRef;
            return true;
        }

        // 3. Fallback: Standalone Big-O token equivalence (e.g. "O(n log n)" vs "o(nlogn)")
        auto refTokens = TextNormalizer::tokenize(cleanRef, false);
        if (refTokens.size() == 1 && (refTokens[0].find("o(") == 0 || refTokens[0].find("O(") == 0)) {
            auto subTokens = TextNormalizer::tokenize(submittedAnswer, false);
            if (std::find(subTokens.begin(), subTokens.end(), refTokens[0]) != subTokens.end()) {
                detectedValue = refTokens[0];
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Computes model answer vocabulary recall (fraction of reference content words covered).
     */
    static double computeRubricCoverage(const std::string& modelAnswer, const std::string& submittedAnswer) {
        auto refTokens = TextNormalizer::tokenize(modelAnswer, true); // Filter generic academic stopwords
        auto subTokens = TextNormalizer::tokenize(submittedAnswer, true);

        if (refTokens.empty()) return 0.0;
        if (subTokens.empty()) return 0.0;

        std::sort(refTokens.begin(), refTokens.end());
        refTokens.erase(std::unique(refTokens.begin(), refTokens.end()), refTokens.end());

        std::sort(subTokens.begin(), subTokens.end());
        subTokens.erase(std::unique(subTokens.begin(), subTokens.end()), subTokens.end());

        std::vector<std::string> common;
        std::set_intersection(refTokens.begin(), refTokens.end(),
                              subTokens.begin(), subTokens.end(),
                              std::back_inserter(common));

        return static_cast<double>(common.size()) / static_cast<double>(refTokens.size());
    }

    // ------------------------------------------------------------------------
    // Main Verification Workflow
    // ------------------------------------------------------------------------

    /**
     * @brief Evaluates a submitted answer against a Question's ReferenceSolution.
     */
    static VerificationResult verifyAnswer(
        const Question& question,
        const std::string& submittedAnswer,
        double absTol = DEFAULT_ABS_TOLERANCE,
        double relTol = DEFAULT_REL_TOLERANCE) {
        return verifyAnswer(question.getReferenceSolution(), submittedAnswer, question.getId(), absTol, relTol);
    }

    /**
     * @brief Evaluates a submitted answer against a ReferenceSolution directly.
     */
    static VerificationResult verifyAnswer(
        const ReferenceSolution& refSolution,
        const std::string& submittedAnswer,
        const std::string& questionId = "",
        double absTol = DEFAULT_ABS_TOLERANCE,
        double relTol = DEFAULT_REL_TOLERANCE) {

        VerificationResult res;
        res.questionId = questionId;
        res.hasReferenceSolution = !refSolution.empty();
        res.isInsufficientEvidence = false;
        res.overallScore = 0.0;
        res.keyTermScore = 0.0;
        res.structuralScore = 0.0;
        res.rubricCoverageScore = 0.0;
        res.numericalFormulaScore = 0.0;
        res.hasNumericalOrFormulaReference = !refSolution.getNumericalOrFormulaAnswer().empty();
        res.numericalOrFormulaMatched = false;
        res.expectedNumericalOrFormula = refSolution.getNumericalOrFormulaAnswer();
        res.disclaimer = VERIFIER_DISCLAIMER;

        // 1. Missing Reference Solution Check
        if (refSolution.empty()) {
            res.hasReferenceSolution = false;
            res.isInsufficientEvidence = true;
            res.overallScore = 0.0;
            res.insufficientEvidenceReason = "No reference solution or evaluation rubric is stored for Question [" +
                                             (questionId.empty() ? "N/A" : questionId) +
                                             "]. Automated verification is unavailable.";
            res.summaryFeedback = res.insufficientEvidenceReason;
            res.feedbackPoints.push_back(res.insufficientEvidenceReason);
            return res;
        }

        // 2. Empty or Whitespace-Only Submitted Answer Check
        std::string trimmedAnswer = TextNormalizer::trim(submittedAnswer);
        if (trimmedAnswer.empty()) {
            res.isInsufficientEvidence = true;
            res.overallScore = 0.0;
            res.insufficientEvidenceReason = "Submitted answer is empty or contains only whitespace.";
            res.summaryFeedback = res.insufficientEvidenceReason;
            res.feedbackPoints.push_back("No answer was submitted for evaluation.");

            for (const auto& kt : refSolution.getMandatoryKeyTerms()) {
                res.missingKeyTerms.push_back(kt);
            }
            for (const auto& sk : refSolution.getStructuralKeywords()) {
                res.missingStructuralKeywords.push_back(sk);
            }
            return res;
        }

        // 3. Tokenize Submitted Answer
        auto rawTokens = TextNormalizer::tokenize(submittedAnswer, false);

        // 4. Evaluate Mandatory Key Terms (Whole-Token Boundary Matching)
        const auto& reqTerms = refSolution.getMandatoryKeyTerms();
        for (const auto& term : reqTerms) {
            if (isKeywordPresent(term, rawTokens)) {
                res.matchedKeyTerms.push_back(term);
            } else {
                res.missingKeyTerms.push_back(term);
            }
        }

        if (!reqTerms.empty()) {
            res.keyTermScore = (static_cast<double>(res.matchedKeyTerms.size()) /
                                static_cast<double>(reqTerms.size())) * 100.0;
        }

        // 5. Evaluate Structural Keywords
        const auto& structKws = refSolution.getStructuralKeywords();
        for (const auto& kw : structKws) {
            if (isKeywordPresent(kw, rawTokens)) {
                res.matchedStructuralKeywords.push_back(kw);
            } else {
                res.missingStructuralKeywords.push_back(kw);
            }
        }

        if (!structKws.empty()) {
            res.structuralScore = (static_cast<double>(res.matchedStructuralKeywords.size()) /
                                   static_cast<double>(structKws.size())) * 100.0;
        }

        // 6. Evaluate Model Answer Rubric Recall
        if (!refSolution.getModelAnswer().empty()) {
            double coverage = computeRubricCoverage(refSolution.getModelAnswer(), submittedAnswer);
            res.rubricCoverageScore = std::round(coverage * 10000.0) / 100.0;
        }

        // 7. Evaluate Numerical / Formula Target
        if (res.hasNumericalOrFormulaReference) {
            res.numericalOrFormulaMatched = checkNumericalOrFormula(
                res.expectedNumericalOrFormula, submittedAnswer, res.detectedNumericalOrFormula,
                absTol, relTol);
            res.numericalFormulaScore = res.numericalOrFormulaMatched ? 100.0 : 0.0;
        }

        // 8. Dynamic Weight Redistribution Across Active Criteria
        double activeWeightSum = 0.0;
        if (!reqTerms.empty()) activeWeightSum += BASE_WEIGHT_KEY_TERMS;
        if (!structKws.empty()) activeWeightSum += BASE_WEIGHT_STRUCTURAL;
        if (!refSolution.getModelAnswer().empty()) activeWeightSum += BASE_WEIGHT_RUBRIC_TEXT;
        if (res.hasNumericalOrFormulaReference) activeWeightSum += BASE_WEIGHT_NUMERICAL;

        if (activeWeightSum > 0.0) {
            double composite = 0.0;
            if (!reqTerms.empty()) {
                composite += (BASE_WEIGHT_KEY_TERMS / activeWeightSum) * res.keyTermScore;
            }
            if (!structKws.empty()) {
                composite += (BASE_WEIGHT_STRUCTURAL / activeWeightSum) * res.structuralScore;
            }
            if (!refSolution.getModelAnswer().empty()) {
                composite += (BASE_WEIGHT_RUBRIC_TEXT / activeWeightSum) * res.rubricCoverageScore;
            }
            if (res.hasNumericalOrFormulaReference) {
                composite += (BASE_WEIGHT_NUMERICAL / activeWeightSum) * res.numericalFormulaScore;
            }

            res.overallScore = std::round(composite * 100.0) / 100.0;
            if (res.overallScore > 100.0) res.overallScore = 100.0;
        } else {
            res.isInsufficientEvidence = true;
            res.insufficientEvidenceReason = "Stored reference solution contains no defined evaluation criteria.";
            res.overallScore = 0.0;
        }

        // 9. Generate Constructive Feedback Points
        if (!reqTerms.empty()) {
            std::ostringstream oss;
            oss << "Key Term Coverage (" << res.matchedKeyTerms.size() << "/" << reqTerms.size() << "): ";
            if (!res.matchedKeyTerms.empty()) {
                oss << "Identified: [";
                for (size_t i = 0; i < res.matchedKeyTerms.size(); ++i) {
                    oss << "'" << res.matchedKeyTerms[i] << "'" << (i + 1 < res.matchedKeyTerms.size() ? ", " : "");
                }
                oss << "]. ";
            }
            if (!res.missingKeyTerms.empty()) {
                oss << "Missing: [";
                for (size_t i = 0; i < res.missingKeyTerms.size(); ++i) {
                    oss << "'" << res.missingKeyTerms[i] << "'" << (i + 1 < res.missingKeyTerms.size() ? ", " : "");
                }
                oss << "].";
            }
            res.feedbackPoints.push_back(oss.str());
        }

        if (!structKws.empty()) {
            std::ostringstream oss;
            oss << "Structural Elements (" << res.matchedStructuralKeywords.size() << "/" << structKws.size() << "): ";
            if (res.missingStructuralKeywords.empty()) {
                oss << "All required structural milestones detected.";
            } else {
                oss << "Missing elements: [";
                for (size_t i = 0; i < res.missingStructuralKeywords.size(); ++i) {
                    oss << "'" << res.missingStructuralKeywords[i] << "'"
                        << (i + 1 < res.missingStructuralKeywords.size() ? ", " : "");
                }
                oss << "].";
            }
            res.feedbackPoints.push_back(oss.str());
        }

        if (res.hasNumericalOrFormulaReference) {
            if (res.numericalOrFormulaMatched) {
                double dummy = 0.0;
                if (tryParseNumber(res.expectedNumericalOrFormula, dummy)) {
                    res.feedbackPoints.push_back(
                        "Numerical value matched reference (" + res.expectedNumericalOrFormula +
                        "). Note: Intermediate mathematical derivation steps were not verified.");
                } else {
                    res.feedbackPoints.push_back(
                        "Expected formula or complexity notation matched: '" +
                        res.expectedNumericalOrFormula + "' (verified via textual structure; not a proof of symbolic mathematical equivalence).");
                }
            } else {
                double dummy = 0.0;
                if (tryParseNumber(res.expectedNumericalOrFormula, dummy)) {
                    res.feedbackPoints.push_back(
                        "Expected numerical target '" + res.expectedNumericalOrFormula +
                        "' was not detected within allowed tolerance in the submitted answer.");
                } else {
                    res.feedbackPoints.push_back(
                        "Expected formula or complexity target '" + res.expectedNumericalOrFormula +
                        "' was not detected in the submitted answer.");
                }
            }
        }

        if (!refSolution.getModelAnswer().empty()) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1);
            oss << "Rubric Vocabulary Coverage: " << res.rubricCoverageScore << "% of reference content words addressed.";
            res.feedbackPoints.push_back(oss.str());
        }

        // Summary Feedback
        std::ostringstream sumOss;
        sumOss << std::fixed << std::setprecision(2);
        sumOss << "Answer Verification Score: " << res.overallScore << "/100.0. ";
        if (res.overallScore >= 80.0) {
            sumOss << "Strong answer: Demonstrates solid coverage of required concepts and terminology.";
        } else if (res.overallScore >= 50.0) {
            sumOss << "Satisfactory answer: Core elements detected, but several key terms or structural milestones were omitted.";
        } else {
            sumOss << "Needs revision: Critical mandatory concepts or expected results are missing from the submission.";
        }
        res.summaryFeedback = sumOss.str();

        return res;
    }
};

} // namespace studysync

#endif // STUDYSYNC_SOLUTION_VERIFIER_HPP
