#ifndef STUDYSYNC_FLASHCARD_HPP
#define STUDYSYNC_FLASHCARD_HPP

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <utility>

namespace studysync {

/**
 * @brief Represents an individual study flashcard or formula card.
 * 
 * Encapsulates card identity, front prompt, back answer/formula,
 * academic subject/topic categorization, tags, and spaced-repetition metrics.
 * 
 * Overloads operators for:
 * - Review priority ordering (<, >) for std::sort
 * - Card identity comparison (==, !=)
 * - Review attempts progression (++card, card++)
 * - Formatted console and debug stream output (<<)
 */
class Flashcard {
private:
    std::string id;
    std::string front;
    std::string back;
    std::string subject;
    std::string topic;
    std::vector<std::string> tags;
    bool isFormula;
    int difficulty;                // Clamped to range [1, 5] (1 = Easiest, 5 = Hardest)
    int interval;                  // Spaced repetition interval in days (>= 0)
    double easeFactor;             // SM-2 multiplier (clamped to minimum 1.3)
    size_t reviewAttempts;         // Total presentation count across all sessions
    size_t successfulRepetitions;  // Consecutive successful recall count (GOOD/EASY)
    std::string nextReviewTimestamp; // Scheduled due date (e.g., "YYYY-MM-DD")

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static int clampDifficulty(int diff) {
        if (diff < 1) return 1;
        if (diff > 5) return 5;
        return diff;
    }

    static double clampEaseFactor(double ef) {
        return ef < 1.3 ? 1.3 : ef;
    }

public:
    /**
     * @brief Constructs a Flashcard with validation and default parameters.
     * @throws std::invalid_argument if id, front, or back is empty, or if interval is negative.
     */
    Flashcard(std::string cardId,
              std::string cardFront,
              std::string cardBack,
              std::string cardSubject = "",
              std::string cardTopic = "",
              std::vector<std::string> cardTags = {},
              bool formula = false,
              int cardDifficulty = 3,
              int cardInterval = 0,
              double cardEaseFactor = 2.5,
              size_t attempts = 0,
              size_t repetitions = 0,
              std::string nextReview = "")
        : id(trim(cardId)),
          front(trim(cardFront)),
          back(trim(cardBack)),
          subject(trim(cardSubject)),
          topic(trim(cardTopic)),
          tags(std::move(cardTags)),
          isFormula(formula),
          difficulty(clampDifficulty(cardDifficulty)),
          interval(cardInterval),
          easeFactor(clampEaseFactor(cardEaseFactor)),
          reviewAttempts(attempts),
          successfulRepetitions(repetitions),
          nextReviewTimestamp(trim(nextReview)) {

        if (id.empty()) {
            throw std::invalid_argument("Flashcard ID cannot be empty.");
        }
        if (front.empty()) {
            throw std::invalid_argument("Flashcard front prompt cannot be empty.");
        }
        if (back.empty()) {
            throw std::invalid_argument("Flashcard back answer cannot be empty.");
        }
        if (interval < 0) {
            throw std::invalid_argument("Flashcard interval cannot be negative.");
        }

        // Clean tags and remove duplicates
        std::vector<std::string> cleanTags;
        for (const auto& tag : tags) {
            std::string ct = trim(tag);
            if (!ct.empty() && std::find(cleanTags.begin(), cleanTags.end(), ct) == cleanTags.end()) {
                cleanTags.push_back(std::move(ct));
            }
        }
        tags = std::move(cleanTags);
    }

    ~Flashcard() = default;

    // Rule of 5: member-wise deep copy and move semantics
    Flashcard(const Flashcard&) = default;
    Flashcard& operator=(const Flashcard&) = default;
    Flashcard(Flashcard&&) noexcept = default;
    Flashcard& operator=(Flashcard&&) noexcept = default;

    // ------------------------------------------------------------------------
    // Operator Overloading
    // ------------------------------------------------------------------------

    /**
     * @brief Strict weak ordering for review priority (for std::sort).
     * 
     * Higher revision priority comes first in ascending sort order:
     * 1. Earlier nextReviewTimestamp comes first (empty timestamp = due immediately, comes first).
     * 2. Tie-breaker 1: Higher difficulty comes first (5 before 1).
     * 3. Tie-breaker 2: Smaller interval comes first (due sooner).
     * 4. Tie-breaker 3 (Identity): Card ID in ascending lexical order.
     */
    bool operator<(const Flashcard& other) const {
        // Priority 1: Due date / timestamp
        if (nextReviewTimestamp != other.nextReviewTimestamp) {
            // An empty timestamp indicates a new or immediately due card (comes first)
            if (nextReviewTimestamp.empty()) return true;
            if (other.nextReviewTimestamp.empty()) return false;
            return nextReviewTimestamp < other.nextReviewTimestamp;
        }

        // Priority 2: Higher difficulty takes precedence
        if (difficulty != other.difficulty) {
            return difficulty > other.difficulty;
        }

        // Priority 3: Shorter interval takes precedence
        if (interval != other.interval) {
            return interval < other.interval;
        }

        // Priority 4: Lexicographical tie-breaker on ID (ensures strict weak ordering)
        return id < other.id;
    }

    /**
     * @brief Inverse priority comparison.
     */
    bool operator>(const Flashcard& other) const {
        return other < *this;
    }

    /**
     * @brief Card identity comparison (evaluated strictly on card ID).
     */
    bool operator==(const Flashcard& other) const {
        return id == other.id;
    }

    /**
     * @brief Card identity inequality.
     */
    bool operator!=(const Flashcard& other) const {
        return id != other.id;
    }

    /**
     * @brief Pre-increment: ++card increments reviewAttempts and returns *this.
     */
    Flashcard& operator++() {
        reviewAttempts++;
        return *this;
    }

    /**
     * @brief Post-increment: card++ increments reviewAttempts and returns previous copy.
     */
    Flashcard operator++(int) {
        Flashcard temp(*this);
        reviewAttempts++;
        return temp;
    }

    /**
     * @brief Formatted stream output operator.
     */
    friend std::ostream& operator<<(std::ostream& os, const Flashcard& card) {
        os << "[Flashcard " << card.id
           << " | Sub: " << (card.subject.empty() ? "General" : card.subject)
           << " | Diff: " << card.difficulty
           << " | Int: " << card.interval << "d"
           << " | EF: " << card.easeFactor
           << " | Att: " << card.reviewAttempts
           << " | Rep: " << card.successfulRepetitions
           << (card.isFormula ? " | Formula" : "")
           << " | Front: \"" << card.front << "\"]";
        return os;
    }

    // ------------------------------------------------------------------------
    // Getters and Setters with Validation
    // ------------------------------------------------------------------------

    const std::string& getId() const { return id; }

    const std::string& getFront() const { return front; }
    void setFront(const std::string& newFront) {
        std::string cleaned = trim(newFront);
        if (cleaned.empty()) {
            throw std::invalid_argument("Flashcard front prompt cannot be empty.");
        }
        front = std::move(cleaned);
    }

    const std::string& getBack() const { return back; }
    void setBack(const std::string& newBack) {
        std::string cleaned = trim(newBack);
        if (cleaned.empty()) {
            throw std::invalid_argument("Flashcard back answer cannot be empty.");
        }
        back = std::move(cleaned);
    }

    const std::string& getSubject() const { return subject; }
    void setSubject(const std::string& newSubject) {
        subject = trim(newSubject);
    }

    const std::string& getTopic() const { return topic; }
    void setTopic(const std::string& newTopic) {
        topic = trim(newTopic);
    }

    const std::vector<std::string>& getTags() const { return tags; }
    void addTag(const std::string& tag) {
        std::string cleaned = trim(tag);
        if (cleaned.empty()) return;
        if (!hasTag(cleaned)) {
            tags.push_back(std::move(cleaned));
        }
    }
    bool removeTag(const std::string& tag) {
        std::string cleaned = trim(tag);
        auto it = std::find(tags.begin(), tags.end(), cleaned);
        if (it != tags.end()) {
            tags.erase(it);
            return true;
        }
        return false;
    }
    bool hasTag(const std::string& tag) const {
        std::string cleaned = trim(tag);
        return std::find(tags.begin(), tags.end(), cleaned) != tags.end();
    }
    void setTags(const std::vector<std::string>& newTags) {
        tags.clear();
        for (const auto& tag : newTags) {
            addTag(tag);
        }
    }

    bool isFormulaCard() const { return isFormula; }
    void setIsFormula(bool formula) { isFormula = formula; }

    int getDifficulty() const { return difficulty; }
    void setDifficulty(int diff) {
        difficulty = clampDifficulty(diff);
    }

    int getInterval() const { return interval; }
    void setInterval(int days) {
        if (days < 0) {
            throw std::invalid_argument("Interval cannot be negative.");
        }
        interval = days;
    }

    double getEaseFactor() const { return easeFactor; }
    void setEaseFactor(double ef) {
        easeFactor = clampEaseFactor(ef);
    }

    size_t getReviewAttempts() const { return reviewAttempts; }
    void setReviewAttempts(size_t attempts) {
        reviewAttempts = attempts;
    }

    size_t getSuccessfulRepetitions() const { return successfulRepetitions; }
    void setSuccessfulRepetitions(size_t reps) {
        successfulRepetitions = reps;
    }

    const std::string& getNextReviewTimestamp() const { return nextReviewTimestamp; }
    void setNextReviewTimestamp(const std::string& timestamp) {
        nextReviewTimestamp = trim(timestamp);
    }

    // Progression Helpers
    void recordAttempt(bool successful) {
        reviewAttempts++;
        if (successful) {
            successfulRepetitions++;
        }
    }
};

} // namespace studysync

#endif // STUDYSYNC_FLASHCARD_HPP
