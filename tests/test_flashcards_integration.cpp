#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cmath>

#include "ds/CircularQueue.hpp"
#include "ds/HashTable.hpp"
#include "models/Flashcard.hpp"
#include "models/FlashcardDeck.hpp"
#include "storage/FlashcardStorage.hpp"

using namespace studysync;

// Helper to remove temporary integration test files safely
static void cleanIntegrationFiles(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    std::filesystem::remove(path + ".tmp", ec);
    std::filesystem::remove(path + ".bak", ec);
}

int main() {
    std::cout << "============================================================" << std::endl;
    std::cout << " StudySync - Module 2 Complete End-to-End Integration Suite " << std::endl;
    std::cout << "============================================================" << std::endl;

    std::string testPath = "data/flashcards_module2_integration.dat";
    cleanIntegrationFiles(testPath);

    // ========================================================================
    // Step 1: Create a FlashcardDeck
    // ========================================================================
    std::cout << "[STEP 1] Creating FlashcardDeck..." << std::endl;
    FlashcardDeck deck;
    assert(deck.cardCount() == 0);
    assert(!deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 0);

    // ========================================================================
    // Step 2: Add several ordinary flashcards and formula cards
    // ========================================================================
    std::cout << "[STEP 2] Adding ordinary flashcards and formula cards..." << std::endl;

    // Ordinary cards (concepts & definitions)
    Flashcard c1("FC-DSA-01", "What is an AVL Tree?",
                  "A self-balancing binary search tree where balance factors are -1, 0, or 1.",
                  "Data Structures", "Trees", {"#dsa", "#tree", "#avl"}, false, 3, 0, 2.5);

    Flashcard c2("FC-DSA-02", "Explain Amortized Complexity",
                  "Average time per operation over a worst-case sequence of operations.",
                  "Data Structures", "Complexity", {"#dsa", "#asymptotics"}, false, 4, 1, 2.36);

    // Formula cards (mathematical and algorithmic formulas)
    Flashcard f1("FORM-MATH-01", "Euler's Identity: e^(i*pi) + 1 = 0",
                  "e^{i\\pi} + 1 = 0 connects e, i, pi, 1, and 0.",
                  "Mathematics", "Complex Analysis", {"#math", "#euler", "#analysis"}, true, 5, 0, 2.5);

    Flashcard f2("FORM-MATH-02", "Quadratic Formula: Roots of ax^2 + bx + c = 0",
                  "x = \\frac{-b \\pm \\sqrt{b^2 - 4ac}}{2a}",
                  "Mathematics", "Algebra", {"#math", "#algebra", "#roots"}, true, 2, 2, 2.6);

    Flashcard f3("FORM-CALC-01", "Fundamental Theorem of Calculus",
                  "\\int_a^b f(x) dx = F(b) - F(a) where F'(x) = f(x)",
                  "Mathematics", "Calculus", {"#math", "#calculus", "#integrals"}, true, 3, 0, 2.5);

    Flashcard f4("FORM-DSA-01", "Binary Tree Max Nodes Formula",
                  "N_{max} = 2^{h+1} - 1 for a binary tree of height h",
                  "Data Structures", "Trees", {"#dsa", "#tree", "#formula"}, true, 2, 0, 2.5);

    assert(deck.addCard(c1));
    assert(deck.addCard(c2));
    assert(deck.addCard(f1));
    assert(deck.addCard(f2));
    assert(deck.addCard(f3));
    assert(deck.addCard(f4));
    assert(deck.cardCount() == 6);

    // ========================================================================
    // Step 3: Verify duplicate card IDs are strictly rejected
    // ========================================================================
    std::cout << "[STEP 3] Verifying duplicate card IDs are rejected..." << std::endl;
    Flashcard dupCard("FC-DSA-01", "Duplicate AVL Question", "Duplicate Answer", "DSA", "Trees");
    assert(!deck.addCard(dupCard)); // Must return false
    assert(deck.cardCount() == 6);  // Count must remain unchanged

    // ========================================================================
    // Step 4 & 5: Index formulas in HashTable and search across dimensions
    // ========================================================================
    std::cout << "[STEP 4 & 5] Indexing formula cards and verifying searches..." << std::endl;
    HashTable formulaIndex;
    FlashcardStorage::rebuildFormulaIndex(deck, formulaIndex);

    // Verify non-formula cards were not indexed in formula index
    assert(!formulaIndex.containsKey("amortized"));
    assert(!formulaIndex.containsKey("#asymptotics"));

    // Title search
    auto eulerMatches = FlashcardStorage::searchFormulas("Euler's Identity", formulaIndex, deck);
    assert(eulerMatches.size() == 1);
    assert(eulerMatches[0]->getId() == "FORM-MATH-01");

    // Keyword search
    auto quadraticMatches = FlashcardStorage::searchFormulas("quadratic", formulaIndex, deck);
    assert(quadraticMatches.size() == 1);
    assert(quadraticMatches[0]->getId() == "FORM-MATH-02");

    // Tag search with and without '#'
    auto mathWithHash = FlashcardStorage::searchFormulas("#math", formulaIndex, deck);
    assert(mathWithHash.size() == 3); // FORM-MATH-01, FORM-MATH-02, FORM-CALC-01
    auto mathWithoutHash = FlashcardStorage::searchFormulas("math", formulaIndex, deck);
    assert(mathWithoutHash.size() == 3);

    // Subject search (all 3 mathematics formula cards)
    auto mathSubjectMatches = FlashcardStorage::searchFormulas("Mathematics", formulaIndex, deck);
    assert(mathSubjectMatches.size() == 3);

    // Topic search
    auto calculusMatches = FlashcardStorage::searchFormulas("Calculus", formulaIndex, deck);
    assert(calculusMatches.size() == 1);
    assert(calculusMatches[0]->getId() == "FORM-CALC-01");

    // Cross-module search (Formula in Data Structures)
    auto treeFormulaMatches = FlashcardStorage::searchFormulas("binary", formulaIndex, deck);
    assert(treeFormulaMatches.size() == 1);
    assert(treeFormulaMatches[0]->getId() == "FORM-DSA-01");

    // Verify no duplicates returned in any search results
    std::vector<std::string> seenIds;
    for (const auto* c : mathSubjectMatches) {
        assert(std::find(seenIds.begin(), seenIds.end(), c->getId()) == seenIds.end());
        seenIds.push_back(c->getId());
    }

    // ========================================================================
    // Step 6 & 7: Start a review session filtered by subject & limited by maxCards
    // ========================================================================
    std::cout << "[STEP 6 & 7] Starting prioritized review session..." << std::endl;
    // Deck has 3 Mathematics formula cards:
    // FORM-MATH-01: diff 5, int 0
    // FORM-MATH-02: diff 2, int 2
    // FORM-CALC-01: diff 3, int 1
    //
    // Review priority via operator<:
    // 1. Due date / empty timestamp (all empty)
    // 2. Higher difficulty first (FORM-MATH-01 [diff 5] > FORM-CALC-01 [diff 3] > FORM-MATH-02 [diff 2])
    // 3. Shorter interval first
    //
    // Start session filtered by "Mathematics", maxCards = 2
    size_t sessionCount = deck.startReviewSession("Mathematics", 2);
    assert(sessionCount == 2);
    assert(deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 2);

    // Priority Check: Highest difficulty (FORM-MATH-01) must be at the head
    Flashcard* current = deck.getCurrentCard();
    assert(current != nullptr);
    assert(current->getId() == "FORM-MATH-01");
    assert(current->getDifficulty() == 5);

    // ========================================================================
    // Step 8: Review a card using AGAIN and verify queue rotation
    // ========================================================================
    std::cout << "[STEP 8] Reviewing card with AGAIN (queue rotation)..." << std::endl;
    std::string againCardId = current->getId(); // "FORM-MATH-01"

    deck.submitReview(Rating::AGAIN);
    assert(deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 2); // Rotated, not removed

    // Check scheduling metrics for FORM-MATH-01
    Flashcard* cardAfterAgain = deck.getCard(againCardId);
    assert(cardAfterAgain->getInterval() == 0); // New card rated AGAIN remains 0
    assert(cardAfterAgain->getReviewAttempts() == 1);
    assert(cardAfterAgain->getSuccessfulRepetitions() == 0);
    assert(std::abs(cardAfterAgain->getEaseFactor() - 2.30) < 0.001); // 2.50 - 0.20

    // After rotation, head of queue should now be the second card (FORM-CALC-01)
    current = deck.getCurrentCard();
    assert(current != nullptr);
    assert(current->getId() == "FORM-CALC-01");
    assert(current->getDifficulty() == 3);

    // ========================================================================
    // Step 9: Review a card using HARD and verify rotation & interval updates
    // ========================================================================
    std::cout << "[STEP 9] Reviewing card with HARD (rotation and interval scaling)..." << std::endl;
    std::string hardCardId = current->getId(); // "FORM-CALC-01"

    deck.submitReview(Rating::HARD);
    assert(deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 2);

    Flashcard* cardAfterHard = deck.getCard(hardCardId);
    assert(cardAfterHard->getReviewAttempts() == 1);
    assert(cardAfterHard->getSuccessfulRepetitions() == 0); // HARD does not increment repetitions
    assert(std::abs(cardAfterHard->getEaseFactor() - 2.36) < 0.001); // 2.50 - 0.14
    assert(cardAfterHard->getInterval() == 1); // New card rated HARD gets interval 1

    // After both cards rotated, head of queue should cycle back to FORM-MATH-01
    current = deck.getCurrentCard();
    assert(current != nullptr);
    assert(current->getId() == againCardId);

    // ========================================================================
    // Step 10: Review cards using GOOD and EASY and complete the session
    // ========================================================================
    std::cout << "[STEP 10] Reviewing cards with GOOD and EASY (session completion)..." << std::endl;

    // Review FORM-MATH-01 with GOOD -> mastered for session, dequeued!
    deck.submitReview(Rating::GOOD);
    assert(deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 1); // Only 1 card remains

    Flashcard* cardAfterGood = deck.getCard(againCardId);
    assert(cardAfterGood->getReviewAttempts() == 2);        // 1 AGAIN + 1 GOOD
    assert(cardAfterGood->getSuccessfulRepetitions() == 1); // Only GOOD counted
    assert(cardAfterGood->getInterval() == 1);              // First successful repetition gives interval 1

    // Remaining card at head must be FORM-CALC-01
    current = deck.getCurrentCard();
    assert(current != nullptr);
    assert(current->getId() == hardCardId);

    // Review FORM-CALC-01 with EASY -> mastered with bonus growth, session finishes!
    deck.submitReview(Rating::EASY);
    assert(!deck.isSessionActive());
    assert(deck.getRemainingSessionCount() == 0);

    Flashcard* cardAfterEasy = deck.getCard(hardCardId);
    assert(cardAfterEasy->getReviewAttempts() == 2);        // 1 HARD + 1 EASY
    assert(cardAfterEasy->getSuccessfulRepetitions() == 1); // Only EASY counted
    assert(std::abs(cardAfterEasy->getEaseFactor() - 2.46) < 0.001); // 2.36 + 0.10
    assert(cardAfterEasy->getInterval() == 2); // New card rated EASY gets interval 2

    // ========================================================================
    // Step 11: Verify reviewAttempts and successfulRepetitions remain distinct
    // ========================================================================
    std::cout << "[STEP 11] Verifying independent attempt and repetition counters..." << std::endl;
    assert(cardAfterGood->getReviewAttempts() == 2);
    assert(cardAfterGood->getSuccessfulRepetitions() == 1);
    assert(cardAfterGood->getReviewAttempts() > cardAfterGood->getSuccessfulRepetitions());

    assert(cardAfterEasy->getReviewAttempts() == 2);
    assert(cardAfterEasy->getSuccessfulRepetitions() == 1);
    assert(cardAfterEasy->getReviewAttempts() > cardAfterEasy->getSuccessfulRepetitions());

    // ========================================================================
    // Step 12: Verify that the 3-attempt limit prevents infinite review loops
    // ========================================================================
    std::cout << "[STEP 12] Verifying three-attempt session cap..." << std::endl;
    FlashcardDeck testLoopDeck;
    testLoopDeck.addCard(Flashcard("LOOP-01", "Hard Question", "Hard Answer", "Physics", "Mechanics"));
    testLoopDeck.startReviewSession("Physics", 1);
    assert(testLoopDeck.isSessionActive());

    // Attempt 1: AGAIN (rotates)
    testLoopDeck.submitReview(Rating::AGAIN);
    assert(testLoopDeck.isSessionActive());

    // Attempt 2: AGAIN (rotates)
    testLoopDeck.submitReview(Rating::AGAIN);
    assert(testLoopDeck.isSessionActive());

    // Attempt 3: AGAIN -> 3-attempt limit reached, card removed, session ends cleanly!
    testLoopDeck.submitReview(Rating::AGAIN);
    assert(!testLoopDeck.isSessionActive());
    assert(testLoopDeck.getRemainingSessionCount() == 0);

    Flashcard* loopedCard = testLoopDeck.getCard("LOOP-01");
    assert(loopedCard->getReviewAttempts() == 3);
    assert(loopedCard->getSuccessfulRepetitions() == 0);

    // ========================================================================
    // Step 13: Save the deck to disk using FlashcardStorage
    // ========================================================================
    std::cout << "[STEP 13] Saving deck to disk using FlashcardStorage..." << std::endl;
    FlashcardStorage::saveCards(testPath, deck);
    assert(std::filesystem::exists(testPath));
    assert(!std::filesystem::exists(testPath + ".tmp"));
    assert(!std::filesystem::exists(testPath + ".bak"));

    // ========================================================================
    // Step 14 & 15: Reload deck and verify all fields restored accurately
    // ========================================================================
    std::cout << "[STEP 14 & 15] Reloading deck and validating data integrity..." << std::endl;
    FlashcardDeck reloadedDeck;
    HashTable reloadedIndex;
    FlashcardStorage::loadDeck(testPath, reloadedDeck, true, &reloadedIndex);

    assert(reloadedDeck.cardCount() == deck.cardCount());

    // Check every card in the deck matches original state verbatim
    for (const auto& originalCard : deck.getAllCards()) {
        const Flashcard* reloaded = reloadedDeck.getCard(originalCard.getId());
        assert(reloaded != nullptr);
        assert(reloaded->getId() == originalCard.getId());
        assert(reloaded->getFront() == originalCard.getFront());
        assert(reloaded->getBack() == originalCard.getBack());
        assert(reloaded->getSubject() == originalCard.getSubject());
        assert(reloaded->getTopic() == originalCard.getTopic());
        assert(reloaded->getTags() == originalCard.getTags());
        assert(reloaded->isFormulaCard() == originalCard.isFormulaCard());
        assert(reloaded->getDifficulty() == originalCard.getDifficulty());
        assert(reloaded->getInterval() == originalCard.getInterval());
        assert(std::abs(reloaded->getEaseFactor() - originalCard.getEaseFactor()) < 0.001);
        assert(reloaded->getReviewAttempts() == originalCard.getReviewAttempts());
        assert(reloaded->getSuccessfulRepetitions() == originalCard.getSuccessfulRepetitions());
        assert(reloaded->getNextReviewTimestamp() == originalCard.getNextReviewTimestamp());
    }

    // ========================================================================
    // Step 16 & 17: Rebuild formula index and verify searches on reloaded data
    // ========================================================================
    std::cout << "[STEP 16 & 17] Verifying formula index searches on reloaded data..." << std::endl;
    // Explicit index rebuild test
    FlashcardStorage::rebuildFormulaIndex(reloadedDeck, reloadedIndex);

    auto reloadedEuler = FlashcardStorage::searchFormulas("Euler", reloadedIndex, reloadedDeck);
    assert(reloadedEuler.size() == 1);
    assert(reloadedEuler[0]->getId() == "FORM-MATH-01");
    assert(reloadedEuler[0]->getBack() == "e^{i\\pi} + 1 = 0 connects e, i, pi, 1, and 0.");

    auto reloadedCalculus = FlashcardStorage::searchFormulas("calculus", reloadedIndex, reloadedDeck);
    assert(reloadedCalculus.size() == 1);
    assert(reloadedCalculus[0]->getId() == "FORM-CALC-01");

    auto reloadedMath = FlashcardStorage::searchFormulas("#math", reloadedIndex, reloadedDeck);
    assert(reloadedMath.size() == 3);

    // ========================================================================
    // Step 18: Test error handling for invalid or missing data
    // ========================================================================
    std::cout << "[STEP 18] Testing exception handling and boundary safeguards..." << std::endl;

    // 1. Inactive session exception
    bool threwInactive = false;
    try {
        reloadedDeck.getCurrentCard();
    } catch (const std::runtime_error&) {
        threwInactive = true;
    }
    assert(threwInactive);

    threwInactive = false;
    try {
        reloadedDeck.submitReview(Rating::GOOD);
    } catch (const std::runtime_error&) {
        threwInactive = true;
    }
    assert(threwInactive);

    // 2. Missing file error handling
    bool threwMissing = false;
    try {
        FlashcardStorage::loadCards("data/non_existent_file_xyz.dat", true);
    } catch (const FileAccessStorageException&) {
        threwMissing = true;
    }
    assert(threwMissing);

    // 3. Corrupted header handling
    std::string corruptPath = "data/temp_corrupt.dat";
    {
        std::ofstream out(corruptPath);
        out << "MALFORMED_HEADER\n[END_STORAGE]\n";
    }
    bool threwCorrupted = false;
    try {
        FlashcardStorage::loadCards(corruptPath, false);
    } catch (const CorruptedRecordStorageException&) {
        threwCorrupted = true;
    }
    assert(threwCorrupted);
    cleanIntegrationFiles(corruptPath);

    // ========================================================================
    // Step 19: Clean up temporary test files
    // ========================================================================
    std::cout << "[STEP 19] Cleaning up temporary test files..." << std::endl;
    cleanIntegrationFiles(testPath);
    assert(!std::filesystem::exists(testPath));

    std::cout << "============================================================" << std::endl;
    std::cout << " MODULE 2 INTEGRATION TESTS COMPLETED SUCCESSFULLY! (19/19) " << std::endl;
    std::cout << "============================================================" << std::endl;

    return 0;
}
