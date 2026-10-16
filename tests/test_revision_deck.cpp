#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>
#include <stdexcept>
#include "models/Flashcard.hpp"
#include "models/FlashcardDeck.hpp"

using namespace studysync;

void test_add_retrieve_remove_cards() {
    std::cout << "[RUN] Test 1: Adding, retrieving, and removing cards..." << std::endl;
    FlashcardDeck deck;
    assert(deck.cardCount() == 0);

    Flashcard c1("C1", "Question 1", "Answer 1", "Data Structures");
    Flashcard c2("C2", "Question 2", "Answer 2", "Algorithms");

    assert(deck.addCard(c1));
    assert(deck.addCard(c2));
    assert(deck.cardCount() == 2);
    assert(deck.hasCard("C1"));
    assert(deck.hasCard("C2"));
    assert(!deck.hasCard("C3"));

    // Retrieval
    Flashcard* retrieved = deck.getCard("C1");
    assert(retrieved != nullptr);
    assert(retrieved->getId() == "C1");
    assert(retrieved->getSubject() == "Data Structures");

    // Missing card retrieval
    assert(deck.getCard("MISSING") == nullptr);

    // Removal
    assert(deck.removeCard("C1"));
    assert(!deck.hasCard("C1"));
    assert(deck.cardCount() == 1);
    assert(!deck.removeCard("C1")); // Second removal fails safely

    std::cout << "  -> Passed: Basic card addition, retrieval, and removal verified." << std::endl;
}

void test_duplicate_id_handling() {
    std::cout << "[RUN] Test 2: Duplicate ID prevention..." << std::endl;
    FlashcardDeck deck;

    Flashcard c1("DUP-1", "Front 1", "Back 1");
    Flashcard c2("DUP-1", "Different Front", "Different Back"); // Same ID

    assert(deck.addCard(c1));
    assert(!deck.addCard(c2)); // Rejected
    assert(deck.cardCount() == 1);
    assert(deck.getCard("DUP-1")->getFront() == "Front 1");

    std::cout << "  -> Passed: Duplicate card IDs strictly rejected." << std::endl;
}

void test_subject_filtering_and_max_cards() {
    std::cout << "[RUN] Test 3: Subject filtering and maxCards limit..." << std::endl;
    FlashcardDeck deck;

    deck.addCard(Flashcard("DS-1", "F1", "B1", "Data Structures"));
    deck.addCard(Flashcard("DS-2", "F2", "B2", "Data Structures"));
    deck.addCard(Flashcard("DS-3", "F3", "B3", "Data Structures"));
    deck.addCard(Flashcard("AL-1", "F4", "B4", "Algorithms"));

    // Filter by "Data Structures" with maxCards = 2
    size_t enrolled = deck.startReviewSession("Data Structures", 2);
    assert(enrolled == 2);
    assert(deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 2);

    // Filter by subject with no matches
    deck.startReviewSession("Operating Systems", 10);
    assert(!deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 0);

    // maxCards == 0
    deck.startReviewSession("Data Structures", 0);
    assert(!deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 0);

    // All subjects (empty subject string) with maxCards = 10
    enrolled = deck.startReviewSession("", 10);
    assert(enrolled == 4);
    assert(deck.getRemainingSessionCount() == 4);

    std::cout << "  -> Passed: Subject filtering and maxCards constraints enforced." << std::endl;
}

void test_session_init_and_current_card() {
    std::cout << "[RUN] Test 4: Session initialization and current-card access..." << std::endl;
    FlashcardDeck deck;

    // Highest priority card (due immediately, difficulty 5)
    deck.addCard(Flashcard("URGENT", "High Priority Question", "Answer", "DSA", "Trees", {}, false, 5, 0, 2.5, 0, 0, "2026-10-10"));
    deck.addCard(Flashcard("LATER", "Low Priority Question", "Answer", "DSA", "Trees", {}, false, 1, 10, 2.5, 0, 0, "2026-10-30"));

    deck.startReviewSession("DSA", 5);
    assert(deck.isSessionActive());

    // Current card should be "URGENT" due to operator< sorting
    Flashcard* current = deck.getCurrentCard();
    assert(current != nullptr);
    assert(current->getId() == "URGENT");

    std::cout << "  -> Passed: Session initialized with prioritized card at front." << std::endl;
}

void test_new_card_ratings_and_intervals() {
    std::cout << "[RUN] Test 5 & 7: New-card interval behavior across all 4 ratings..." << std::endl;

    // 1. New card rated AGAIN -> interval = 0
    {
        FlashcardDeck deck;
        deck.addCard(Flashcard("N1", "F", "B"));
        deck.startReviewSession("", 1);
        deck.submitReview(Rating::AGAIN);
        Flashcard* c = deck.getCard("N1");
        assert(c->getInterval() == 0);
        assert(c->getSuccessfulRepetitions() == 0);
        assert(c->getReviewAttempts() == 1);
        assert(std::abs(c->getEaseFactor() - 2.30) < 0.001); // 2.5 - 0.20
    }

    // 2. New card rated HARD -> interval = 1
    {
        FlashcardDeck deck;
        deck.addCard(Flashcard("N2", "F", "B"));
        deck.startReviewSession("", 1);
        deck.submitReview(Rating::HARD);
        Flashcard* c = deck.getCard("N2");
        assert(c->getInterval() == 1);
        assert(c->getSuccessfulRepetitions() == 0); // HARD does not count as successful repetition
        assert(std::abs(c->getEaseFactor() - 2.36) < 0.001); // 2.5 - 0.14
    }

    // 3. New card rated GOOD -> interval = 1, repetitions = 1
    {
        FlashcardDeck deck;
        deck.addCard(Flashcard("N3", "F", "B"));
        deck.startReviewSession("", 1);
        deck.submitReview(Rating::GOOD);
        Flashcard* c = deck.getCard("N3");
        assert(c->getInterval() == 1);
        assert(c->getSuccessfulRepetitions() == 1);
        assert(std::abs(c->getEaseFactor() - 2.50) < 0.001); // Unchanged
    }

    // 4. New card rated EASY -> interval = 2, repetitions = 1
    {
        FlashcardDeck deck;
        deck.addCard(Flashcard("N4", "F", "B"));
        deck.startReviewSession("", 1);
        deck.submitReview(Rating::EASY);
        Flashcard* c = deck.getCard("N4");
        assert(c->getInterval() == 2);
        assert(c->getSuccessfulRepetitions() == 1);
        assert(std::abs(c->getEaseFactor() - 2.60) < 0.001); // 2.5 + 0.10
    }

    std::cout << "  -> Passed: New-card interval and ease-factor calculation verified." << std::endl;
}

void test_reviewed_card_ratings_and_ease_factor() {
    std::cout << "[RUN] Test 6 & 8: Previously reviewed card intervals and ease-factor clamping..." << std::endl;

    // 1. GOOD rating: interval = round(10 * 2.5) = 25
    {
        FlashcardDeck deck;
        deck.addCard(Flashcard("R1", "F", "B", "", "", {}, false, 3, 10, 2.5, 3, 2, "2026-10-15"));
        deck.startReviewSession("", 1);
        deck.submitReview(Rating::GOOD);
        Flashcard* c = deck.getCard("R1");
        assert(c->getInterval() == 25);
        assert(c->getSuccessfulRepetitions() == 3);
        assert(c->getEaseFactor() == 2.5);
    }

    // 2. EASY rating: EF' = 2.5 + 0.10 = 2.6; interval = round(10 * 2.6 * 1.3) = round(33.8) = 34
    {
        FlashcardDeck deck;
        deck.addCard(Flashcard("R2", "F", "B", "", "", {}, false, 3, 10, 2.5, 3, 2, "2026-10-15"));
        deck.startReviewSession("", 1);
        deck.submitReview(Rating::EASY);
        Flashcard* c = deck.getCard("R2");
        assert(c->getInterval() == 34);
        assert(c->getSuccessfulRepetitions() == 3);
        assert(std::abs(c->getEaseFactor() - 2.6) < 0.001);
    }

    // 3. HARD rating: EF' = 2.5 - 0.14 = 2.36; interval = max(1, round(10 * 1.2)) = 12
    {
        FlashcardDeck deck;
        deck.addCard(Flashcard("R3", "F", "B", "", "", {}, false, 3, 10, 2.5, 3, 2, "2026-10-15"));
        deck.startReviewSession("", 1);
        deck.submitReview(Rating::HARD);
        Flashcard* c = deck.getCard("R3");
        assert(c->getInterval() == 12);
        assert(c->getSuccessfulRepetitions() == 2); // Unchanged
        assert(std::abs(c->getEaseFactor() - 2.36) < 0.001);
    }

    // 4. AGAIN rating: resets interval to 1 day and repetitions to 0; EF' = 2.5 - 0.20 = 2.30
    {
        FlashcardDeck deck;
        deck.addCard(Flashcard("R4", "F", "B", "", "", {}, false, 3, 10, 2.5, 3, 2, "2026-10-15"));
        deck.startReviewSession("", 1);
        deck.submitReview(Rating::AGAIN);
        Flashcard* c = deck.getCard("R4");
        assert(c->getInterval() == 1);
        assert(c->getSuccessfulRepetitions() == 0);
        assert(std::abs(c->getEaseFactor() - 2.30) < 0.001);
    }

    // 5. Minimum clamping test for Ease Factor (cannot drop below 1.3)
    {
        FlashcardDeck deck;
        deck.addCard(Flashcard("LOW-EF", "F", "B", "", "", {}, false, 3, 5, 1.4, 5, 2));
        deck.startReviewSession("", 1);
        deck.submitReview(Rating::AGAIN); // 1.4 - 0.20 would be 1.2 -> clamped to 1.3
        Flashcard* c = deck.getCard("LOW-EF");
        assert(c->getEaseFactor() == 1.3);
    }

    std::cout << "  -> Passed: Mathematical interval scaling and ease-factor clamping verified." << std::endl;
}

void test_rotation_and_removal_session_behavior() {
    std::cout << "[RUN] Test 9 & 10: Queue rotation after AGAIN/HARD and removal after GOOD/EASY..." << std::endl;
    FlashcardDeck deck;

    deck.addCard(Flashcard("CARD-A", "F-A", "B-A", "Math", "", {}, false, 3, 1, 2.5, 1, 1));
    deck.addCard(Flashcard("CARD-B", "F-B", "B-B", "Math", "", {}, false, 3, 2, 2.5, 1, 1));

    deck.startReviewSession("Math", 2);
    assert(deck.getRemainingSessionCount() == 2);
    assert(deck.getCurrentCard()->getId() == "CARD-A");

    // Rate CARD-A with HARD: must rotate to rear; CARD-B becomes front
    deck.submitReview(Rating::HARD);
    assert(deck.getRemainingSessionCount() == 2); // Still 2 cards in session
    assert(deck.getCurrentCard()->getId() == "CARD-B");

    // Rate CARD-B with GOOD: mastered -> dequeued from session! CARD-A becomes front
    deck.submitReview(Rating::GOOD);
    assert(deck.getRemainingSessionCount() == 1); // 1 card remaining
    assert(deck.getCurrentCard()->getId() == "CARD-A");

    // Rate CARD-A with EASY: mastered -> dequeued from session!
    deck.submitReview(Rating::EASY);
    assert(!deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 0);

    std::cout << "  -> Passed: Circular queue rotations and removals function correctly in active session." << std::endl;
}

void test_three_attempt_session_limit() {
    std::cout << "[RUN] Test 11 & 12: Three-attempt session cap prevents infinite loops..." << std::endl;
    FlashcardDeck deck;

    deck.addCard(Flashcard("STUBBORN", "Hard Question", "Answer", "Physics"));
    deck.startReviewSession("Physics", 1);
    assert(deck.isSessionActive());

    // Presentation 1: AGAIN (attempt 1) -> rotates
    deck.submitReview(Rating::AGAIN);
    assert(deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 1);
    assert(deck.getCard("STUBBORN")->getReviewAttempts() == 1);

    // Presentation 2: AGAIN (attempt 2) -> rotates
    deck.submitReview(Rating::AGAIN);
    assert(deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 1);
    assert(deck.getCard("STUBBORN")->getReviewAttempts() == 2);

    // Presentation 3: AGAIN (attempt 3) -> reaches MAX_SESSION_ATTEMPTS (3) -> dequeued!
    deck.submitReview(Rating::AGAIN);
    assert(!deck.isSessionActive()); // Session completes!
    assert(deck.getRemainingSessionCount() == 0);
    assert(deck.getCard("STUBBORN")->getReviewAttempts() == 3);

    std::cout << "  -> Passed: Three-attempt limit strictly enforced; infinite review loops prevented." << std::endl;
}

void test_invalid_calls_and_edge_cases() {
    std::cout << "[RUN] Test 13 & 14: Invalid calls, empty sessions, and missing card safety..." << std::endl;
    FlashcardDeck deck;

    // 1. getCurrentCard on inactive session throws
    bool caughtNoCurrent = false;
    try {
        deck.getCurrentCard();
    } catch (const std::runtime_error&) {
        caughtNoCurrent = true;
    }
    assert(caughtNoCurrent);

    // 2. submitReview on inactive session throws
    bool caughtNoReview = false;
    try {
        deck.submitReview(Rating::GOOD);
    } catch (const std::runtime_error&) {
        caughtNoReview = true;
    }
    assert(caughtNoReview);

    // 3. Removing a card while active in session
    deck.addCard(Flashcard("C1", "F", "B"));
    deck.addCard(Flashcard("C2", "F", "B"));
    deck.startReviewSession("", 2);
    assert(deck.getRemainingSessionCount() == 2);

    // Remove active card C1
    assert(deck.removeCard("C1"));
    assert(deck.getRemainingSessionCount() == 1);
    assert(deck.getCurrentCard()->getId() == "C2");

    // Remove C2
    assert(deck.removeCard("C2"));
    assert(!deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 0);

    std::cout << "  -> Passed: Inactive calls and in-session card deletions handled safely." << std::endl;
}

void test_starting_new_session_after_completion() {
    std::cout << "[RUN] Test 15: Starting a new session after previous session completes..." << std::endl;
    FlashcardDeck deck;

    deck.addCard(Flashcard("CYCLE-1", "Front", "Back"));

    // Session 1
    deck.startReviewSession("", 1);
    assert(deck.isSessionActive());
    deck.submitReview(Rating::GOOD);
    assert(!deck.isSessionActive());

    // Session 2 on same deck
    size_t enrolled = deck.startReviewSession("", 1);
    assert(enrolled == 1);
    assert(deck.isSessionActive());
    assert(deck.getCurrentCard()->getId() == "CYCLE-1");
    deck.submitReview(Rating::EASY);
    assert(!deck.isSessionActive());

    std::cout << "  -> Passed: Successive revision sessions initialize and complete cleanly." << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " StudySync - Task 2.3 FlashcardDeck & Scheduler Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    test_add_retrieve_remove_cards();
    test_duplicate_id_handling();
    test_subject_filtering_and_max_cards();
    test_session_init_and_current_card();
    test_new_card_ratings_and_intervals();
    test_reviewed_card_ratings_and_ease_factor();
    test_rotation_and_removal_session_behavior();
    test_three_attempt_session_limit();
    test_invalid_calls_and_edge_cases();
    test_starting_new_session_after_completion();

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL REVISION DECK TESTS PASSED! (10/10 Test Suites)      " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
