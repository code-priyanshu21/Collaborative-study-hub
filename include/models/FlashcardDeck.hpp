#ifndef STUDYSYNC_FLASHCARD_DECK_HPP
#define STUDYSYNC_FLASHCARD_DECK_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <utility>

#include "ds/CircularQueue.hpp"
#include "models/Flashcard.hpp"

namespace studysync {

/**
 * @brief User feedback rating for a flashcard review presentation.
 */
enum class Rating {
    AGAIN = 1, // Complete failure of recall (rotates in queue, resets interval)
    HARD = 2,  // Recalled with significant difficulty (rotates in queue, moderate interval growth)
    GOOD = 3,  // Successful recall (completes card for session, standard SM-2 growth)
    EASY = 4   // Effortless, instant recall (completes card for session, bonus interval growth)
};

/**
 * @brief Manages a collection of flashcards and orchestrates active revision sessions.
 * 
 * Implements a simplified SM-2-inspired spaced-repetition algorithm using CircularQueue:
 * - Cards due for review are loaded into a CircularQueue<std::string> (card IDs).
 * - Mastered cards (GOOD/EASY) are dequeued and completed for the session.
 * - Struggling cards (AGAIN/HARD) are rotated to the rear of the CircularQueue in O(1).
 * - An attempt limit (MAX_SESSION_ATTEMPTS = 3) prevents infinite session loops.
 */
class FlashcardDeck {
public:
    static constexpr int MAX_SESSION_ATTEMPTS = 3;

private:
    std::vector<Flashcard> cards;
    CircularQueue<std::string> sessionQueue;
    std::unordered_map<std::string, int> sessionAttempts;
    bool sessionActive;

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

public:
    FlashcardDeck() : sessionActive(false) {}

    // ------------------------------------------------------------------------
    // Card Collection Management
    // ------------------------------------------------------------------------

    /**
     * @brief Adds a new flashcard to the deck.
     * @return true if added, false if a card with the same ID already exists.
     */
    bool addCard(const Flashcard& card) {
        if (hasCard(card.getId())) {
            return false;
        }
        cards.push_back(card);
        return true;
    }

    /**
     * @brief Checks if a card with the given ID exists.
     */
    bool hasCard(const std::string& id) const {
        std::string cleanId = trim(id);
        return std::any_of(cards.begin(), cards.end(),
            [&cleanId](const Flashcard& c) { return c.getId() == cleanId; });
    }

    /**
     * @brief Retrieves a pointer to a card by ID (mutable).
     * @return Pointer to Flashcard, or nullptr if not found.
     */
    Flashcard* getCard(const std::string& id) {
        std::string cleanId = trim(id);
        auto it = std::find_if(cards.begin(), cards.end(),
            [&cleanId](const Flashcard& c) { return c.getId() == cleanId; });
        return it != cards.end() ? &(*it) : nullptr;
    }

    /**
     * @brief Retrieves a pointer to a card by ID (const).
     * @return Const pointer to Flashcard, or nullptr if not found.
     */
    const Flashcard* getCard(const std::string& id) const {
        std::string cleanId = trim(id);
        auto it = std::find_if(cards.begin(), cards.end(),
            [&cleanId](const Flashcard& c) { return c.getId() == cleanId; });
        return it != cards.end() ? &(*it) : nullptr;
    }

    /**
     * @brief Removes a card by ID from the deck.
     * @return true if removed, false if ID not found.
     */
    bool removeCard(const std::string& id) {
        std::string cleanId = trim(id);
        auto it = std::find_if(cards.begin(), cards.end(),
            [&cleanId](const Flashcard& c) { return c.getId() == cleanId; });

        if (it != cards.end()) {
            cards.erase(it);
            // If the removed card is currently in active session, end session or clean up
            if (sessionActive) {
                // Rebuild session queue without the removed card ID
                CircularQueue<std::string> newQueue(sessionQueue.capacity());
                while (!sessionQueue.isEmpty()) {
                    std::string cid = sessionQueue.dequeue();
                    if (cid != cleanId) {
                        newQueue.enqueue(cid);
                    }
                }
                sessionQueue = std::move(newQueue);
                sessionAttempts.erase(cleanId);
                if (sessionQueue.isEmpty()) {
                    sessionActive = false;
                }
            }
            return true;
        }
        return false;
    }

    /**
     * @brief Returns a copy of all flashcards in the deck.
     */
    const std::vector<Flashcard>& getAllCards() const {
        return cards;
    }

    /**
     * @brief Returns total number of cards in the deck.
     */
    size_t cardCount() const noexcept {
        return cards.size();
    }

    /**
     * @brief Clears all cards and resets any active session.
     */
    void clear() {
        cards.clear();
        sessionQueue.clear();
        sessionAttempts.clear();
        sessionActive = false;
    }

    // ------------------------------------------------------------------------
    // Review Session Lifecycle
    // ------------------------------------------------------------------------

    /**
     * @brief Initiates a new revision session using the CircularQueue scheduler.
     * 
     * @param subject Optional subject filter (empty string selects all subjects).
     * @param maxCards Maximum number of cards to include (0 selects no cards).
     * @return Number of cards enrolled in the active session.
     */
    size_t startReviewSession(const std::string& subject = "", size_t maxCards = 0) {
        sessionQueue.clear();
        sessionAttempts.clear();
        sessionActive = false;

        if (maxCards == 0 || cards.empty()) {
            return 0;
        }

        std::string cleanSubject = trim(subject);

        // Filter eligible candidate cards
        std::vector<Flashcard*> candidates;
        for (auto& card : cards) {
            if (cleanSubject.empty() || card.getSubject() == cleanSubject) {
                candidates.push_back(&card);
            }
        }

        if (candidates.empty()) {
            return 0;
        }

        // Sort candidates by priority using Flashcard::operator< (highest review priority first)
        std::sort(candidates.begin(), candidates.end(),
            [](const Flashcard* a, const Flashcard* b) { return *a < *b; });

        // Limit to maxCards
        size_t countToEnroll = std::min(maxCards, candidates.size());

        // Enqueue into CircularQueue
        for (size_t i = 0; i < countToEnroll; ++i) {
            sessionQueue.enqueue(candidates[i]->getId());
            sessionAttempts[candidates[i]->getId()] = 0;
        }

        sessionActive = !sessionQueue.isEmpty();
        return sessionQueue.size();
    }

    /**
     * @brief Checks if a review session is currently active with cards remaining.
     */
    bool isSessionActive() const noexcept {
        return sessionActive && !sessionQueue.isEmpty();
    }

    /**
     * @brief Returns the number of cards remaining in the active session queue.
     */
    size_t getRemainingSessionCount() const noexcept {
        return (sessionActive && !sessionQueue.isEmpty()) ? sessionQueue.size() : 0;
    }

    /**
     * @brief Retrieves the card currently presented at the front of the session queue.
     * @throws std::runtime_error if no session is active or queue is empty.
     */
    Flashcard* getCurrentCard() {
        if (!isSessionActive()) {
            throw std::runtime_error("No active review session or queue is empty.");
        }
        std::string currentId = sessionQueue.front();
        Flashcard* card = getCard(currentId);
        if (!card) {
            throw std::runtime_error("Current session card not found in deck: " + currentId);
        }
        return card;
    }

    /**
     * @brief Const overload of getCurrentCard().
     */
    const Flashcard* getCurrentCard() const {
        if (!isSessionActive()) {
            throw std::runtime_error("No active review session or queue is empty.");
        }
        std::string currentId = sessionQueue.front();
        const Flashcard* card = getCard(currentId);
        if (!card) {
            throw std::runtime_error("Current session card not found in deck: " + currentId);
        }
        return card;
    }

    /**
     * @brief Submits user recall rating for the current card and updates scheduling metrics.
     * 
     * SCHEDULING ALGORITHM (Simplified SM-2-inspired):
     * 1. Ease Factor Update:
     *    EF' = max(1.3, EF + (0.1 - (4 - rating) * (0.08 + (4 - rating) * 0.02)))
     *    - EASY (4): +0.10
     *    - GOOD (3): 0.00 (unchanged)
     *    - HARD (2): -0.14
     *    - AGAIN (1): -0.20
     * 
     * 2. Interval Calculation (calculated using the updated ease factor EF'):
     *    - New Cards (interval == 0 or successfulRepetitions == 0):
     *        AGAIN: interval = 0
     *        HARD:  interval = 1
     *        GOOD:  interval = 1, repetitions = 1
     *        EASY:  interval = 2, repetitions = 1
     *    - Reviewed Cards (interval >= 1 and successfulRepetitions >= 1):
     *        AGAIN: interval = 1, repetitions = 0
     *        HARD:  interval = max(1, round(interval * 1.2))
     *        GOOD:  interval = round(interval * EF'), repetitions++
     *        EASY:  interval = round(interval * EF' * 1.3), repetitions++
     * 
     * 3. Queue Session Behavior:
     *    - GOOD / EASY: Card mastered -> dequeued from active session.
     *    - AGAIN / HARD: Card struggling -> rotated to rear of CircularQueue.
     *      If a card reaches MAX_SESSION_ATTEMPTS (3), it is dequeued to prevent
     *      an infinite session loop and scheduled for future review.
     * 
     * @throws std::runtime_error if called when no session is active.
     * @throws std::invalid_argument if rating is outside [1, 4].
     */
    void submitReview(Rating rating) {
        if (!isSessionActive()) {
            throw std::runtime_error("Cannot submit review: No active review session.");
        }

        int r = static_cast<int>(rating);
        if (r < 1 || r > 4) {
            throw std::invalid_argument("Invalid review rating. Must be between 1 and 4.");
        }

        std::string currentId = sessionQueue.front();
        Flashcard* card = getCard(currentId);
        if (!card) {
            sessionQueue.dequeue();
            if (sessionQueue.isEmpty()) sessionActive = false;
            throw std::runtime_error("Current session card missing from deck: " + currentId);
        }

        // Increment presentation attempts
        card->recordAttempt(false); // increments reviewAttempts
        sessionAttempts[currentId]++;

        // 1. Calculate updated Ease Factor (EASY: +0.10, GOOD: 0.00, HARD: -0.14, AGAIN: -0.20)
        double deltaEF = 0.0;
        switch (rating) {
            case Rating::EASY:  deltaEF = 0.10; break;
            case Rating::GOOD:  deltaEF = 0.00; break;
            case Rating::HARD:  deltaEF = -0.14; break;
            case Rating::AGAIN: deltaEF = -0.20; break;
        }
        double updatedEF = std::max(1.3, card->getEaseFactor() + deltaEF);
        card->setEaseFactor(updatedEF);

        // 2. Calculate updated Interval using updatedEF
        int currInterval = card->getInterval();
        size_t currReps = card->getSuccessfulRepetitions();

        if (currInterval == 0 || currReps == 0) {
            // New card rules
            switch (rating) {
                case Rating::AGAIN:
                    card->setInterval(0);
                    break;
                case Rating::HARD:
                    card->setInterval(1);
                    break;
                case Rating::GOOD:
                    card->setInterval(1);
                    card->setSuccessfulRepetitions(1);
                    break;
                case Rating::EASY:
                    card->setInterval(2);
                    card->setSuccessfulRepetitions(1);
                    break;
            }
        } else {
            // Previously reviewed card rules
            switch (rating) {
                case Rating::AGAIN:
                    card->setInterval(1);
                    card->setSuccessfulRepetitions(0);
                    break;
                case Rating::HARD: {
                    int nextInt = std::max(1, static_cast<int>(std::round(currInterval * 1.2)));
                    card->setInterval(nextInt);
                    break;
                }
                case Rating::GOOD: {
                    int nextInt = std::max(1, static_cast<int>(std::round(currInterval * updatedEF)));
                    card->setInterval(nextInt);
                    card->setSuccessfulRepetitions(currReps + 1);
                    break;
                }
                case Rating::EASY: {
                    int nextInt = std::max(1, static_cast<int>(std::round(currInterval * updatedEF * 1.3)));
                    card->setInterval(nextInt);
                    card->setSuccessfulRepetitions(currReps + 1);
                    break;
                }
            }
        }

        // 3. Queue Session Rotation / Completion Logic
        if (rating == Rating::GOOD || rating == Rating::EASY) {
            // Mastered for this session: dequeue
            sessionQueue.dequeue();
        } else {
            // Struggling (AGAIN or HARD)
            if (sessionAttempts[currentId] < MAX_SESSION_ATTEMPTS) {
                // Rotate to rear of circular queue for re-test
                sessionQueue.rotate();
            } else {
                // Capped at 3 presentations: dequeue to ensure session completion
                sessionQueue.dequeue();
            }
        }

        // If queue emptied, mark session inactive
        if (sessionQueue.isEmpty()) {
            sessionActive = false;
        }
    }
};

} // namespace studysync

#endif // STUDYSYNC_FLASHCARD_DECK_HPP
