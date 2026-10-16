#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include "models/Flashcard.hpp"

using namespace studysync;

void test_construction_and_defaults() {
    std::cout << "[RUN] Test 1: Construction and default values..." << std::endl;
    Flashcard card("CARD-001", "What is an AVL Tree?", "A self-balancing binary search tree.");

    assert(card.getId() == "CARD-001");
    assert(card.getFront() == "What is an AVL Tree?");
    assert(card.getBack() == "A self-balancing binary search tree.");
    assert(card.getSubject().empty());
    assert(card.getTopic().empty());
    assert(card.getTags().empty());
    assert(!card.isFormulaCard());
    assert(card.getDifficulty() == 3); // Default normal difficulty
    assert(card.getInterval() == 0);   // Default new card interval
    assert(card.getEaseFactor() == 2.5); // Default SM-2 ease factor
    assert(card.getReviewAttempts() == 0);
    assert(card.getSuccessfulRepetitions() == 0);
    assert(card.getNextReviewTimestamp().empty());

    // Explicit custom construction
    std::vector<std::string> tags = {"#math", "#calculus"};
    Flashcard formulaCard("CARD-002", "d/dx [sin(x)]", "cos(x)", "Mathematics",
                          "Calculus", tags, true, 4, 3, 2.6, 5, 4, "2026-10-20");

    assert(formulaCard.getId() == "CARD-002");
    assert(formulaCard.getSubject() == "Mathematics");
    assert(formulaCard.getTopic() == "Calculus");
    assert(formulaCard.isFormulaCard());
    assert(formulaCard.getDifficulty() == 4);
    assert(formulaCard.getInterval() == 3);
    assert(formulaCard.getEaseFactor() == 2.6);
    assert(formulaCard.getReviewAttempts() == 5);
    assert(formulaCard.getSuccessfulRepetitions() == 4);
    assert(formulaCard.getNextReviewTimestamp() == "2026-10-20");

    std::cout << "  -> Passed: Default and custom construction values verified." << std::endl;
}

void test_invalid_field_rejection() {
    std::cout << "[RUN] Test 2: Rejection of invalid empty required fields and negative intervals..." << std::endl;

    // 1. Empty ID
    bool caughtEmptyId = false;
    try {
        Flashcard bad("", "Front", "Back");
    } catch (const std::invalid_argument&) {
        caughtEmptyId = true;
    }
    assert(caughtEmptyId);

    // 2. Whitespace ID
    bool caughtWhitespaceId = false;
    try {
        Flashcard bad("   \t  ", "Front", "Back");
    } catch (const std::invalid_argument&) {
        caughtWhitespaceId = true;
    }
    assert(caughtWhitespaceId);

    // 3. Empty Front
    bool caughtEmptyFront = false;
    try {
        Flashcard bad("CARD-001", "", "Back");
    } catch (const std::invalid_argument&) {
        caughtEmptyFront = true;
    }
    assert(caughtEmptyFront);

    // 4. Empty Back
    bool caughtEmptyBack = false;
    try {
        Flashcard bad("CARD-001", "Front", "   ");
    } catch (const std::invalid_argument&) {
        caughtEmptyBack = true;
    }
    assert(caughtEmptyBack);

    // 5. Negative interval in constructor
    bool caughtNegInterval = false;
    try {
        Flashcard bad("CARD-001", "Front", "Back", "Sub", "Top", {}, false, 3, -5);
    } catch (const std::invalid_argument&) {
        caughtNegInterval = true;
    }
    assert(caughtNegInterval);

    // 6. Setter validations
    Flashcard validCard("CARD-001", "Front", "Back");
    bool caughtSetEmptyFront = false;
    try {
        validCard.setFront("");
    } catch (const std::invalid_argument&) {
        caughtSetEmptyFront = true;
    }
    assert(caughtSetEmptyFront);

    bool caughtSetNegInterval = false;
    try {
        validCard.setInterval(-1);
    } catch (const std::invalid_argument&) {
        caughtSetNegInterval = true;
    }
    assert(caughtSetNegInterval);

    std::cout << "  -> Passed: Input validation prevents empty fields and negative intervals." << std::endl;
}

void test_difficulty_and_ease_factor_clamping() {
    std::cout << "[RUN] Test 3: Difficulty and ease-factor validation and clamping..." << std::endl;

    // Difficulty clamped below 1 -> 1
    Flashcard lowDiff("CARD-1", "F", "B", "", "", {}, false, -10);
    assert(lowDiff.getDifficulty() == 1);

    // Difficulty clamped above 5 -> 5
    Flashcard highDiff("CARD-2", "F", "B", "", "", {}, false, 99);
    assert(highDiff.getDifficulty() == 5);

    // Setter difficulty clamping
    lowDiff.setDifficulty(0);
    assert(lowDiff.getDifficulty() == 1);
    lowDiff.setDifficulty(10);
    assert(lowDiff.getDifficulty() == 5);

    // Ease factor clamped to minimum 1.3
    Flashcard lowEf("CARD-3", "F", "B", "", "", {}, false, 3, 0, 0.5);
    assert(lowEf.getEaseFactor() == 1.3);

    // Setter ease factor clamping
    lowEf.setEaseFactor(1.1);
    assert(lowEf.getEaseFactor() == 1.3);
    lowEf.setEaseFactor(2.8);
    assert(lowEf.getEaseFactor() == 2.8);

    std::cout << "  -> Passed: Difficulty clamped to [1, 5] and easeFactor clamped to >= 1.3." << std::endl;
}

void test_identity_equality_operators() {
    std::cout << "[RUN] Test 4: Equality based on card identity (ID) rather than content..." << std::endl;

    Flashcard card1("CARD-100", "Question A", "Answer A");
    Flashcard card2("CARD-100", "Different Question", "Different Answer"); // Same ID
    Flashcard card3("CARD-200", "Question A", "Answer A");                 // Same content, different ID

    // Same ID means identical card
    assert(card1 == card2);
    assert(!(card1 != card2));

    // Different ID means different cards even if content matches
    assert(card1 != card3);
    assert(!(card1 == card3));

    std::cout << "  -> Passed: operator== and operator!= accurately reflect unique card identity." << std::endl;
}

void test_operator_less_tie_breakers() {
    std::cout << "[RUN] Test 5: operator< ordering and hierarchical tie-breakers..." << std::endl;

    // 1. Earlier timestamp takes priority (empty = new/due immediately, comes first)
    Flashcard dueNow("C1", "F", "B", "", "", {}, false, 3, 0, 2.5, 0, 0, "");
    Flashcard dueEarly("C2", "F", "B", "", "", {}, false, 3, 0, 2.5, 0, 0, "2026-10-15");
    Flashcard dueLater("C3", "F", "B", "", "", {}, false, 3, 0, 2.5, 0, 0, "2026-10-20");

    assert(dueNow < dueEarly);
    assert(dueEarly < dueLater);
    assert(dueLater > dueEarly);
    assert(!(dueEarly < dueNow));

    // 2. Equal timestamps -> higher difficulty takes priority (5 before 1)
    Flashcard hardCard("C4", "F", "B", "", "", {}, false, 5, 2, 2.5, 0, 0, "2026-10-15");
    Flashcard easyCard("C5", "F", "B", "", "", {}, false, 2, 2, 2.5, 0, 0, "2026-10-15");
    assert(hardCard < easyCard);
    assert(easyCard > hardCard);

    // 3. Equal timestamps and difficulty -> smaller interval takes priority (due sooner)
    Flashcard shortInt("C6", "F", "B", "", "", {}, false, 4, 1, 2.5, 0, 0, "2026-10-15");
    Flashcard longInt("C7", "F", "B", "", "", {}, false, 4, 10, 2.5, 0, 0, "2026-10-15");
    assert(shortInt < longInt);

    // 4. Equal timestamps, difficulty, and interval -> tie-breaker on ID (lexicographical)
    Flashcard idA("CARD-A", "F", "B", "", "", {}, false, 4, 5, 2.5, 0, 0, "2026-10-15");
    Flashcard idB("CARD-B", "F", "B", "", "", {}, false, 4, 5, 2.5, 0, 0, "2026-10-15");
    assert(idA < idB);
    assert(idB > idA);
    assert(!(idA < idA)); // Irreflexive

    std::cout << "  -> Passed: operator< strictly implements prioritized review ordering." << std::endl;
}

void test_std_sort_review_priority() {
    std::cout << "[RUN] Test 6: std::sort producing the intended revision priority sequence..." << std::endl;

    Flashcard c1("C1", "F", "B", "", "", {}, false, 2, 10, 2.5, 0, 0, "2026-10-25"); // Due late, easy
    Flashcard c2("C2", "F", "B", "", "", {}, false, 5, 1, 2.5, 0, 0, "2026-10-15");  // Due soon, hard (Priority 2)
    Flashcard c3("C3", "F", "B", "", "", {}, false, 1, 1, 2.5, 0, 0, "2026-10-15");  // Due soon, easy (Priority 3)
    Flashcard c4("C4", "F", "B", "", "", {}, false, 3, 0, 2.5, 0, 0, "");            // Due now/new (Priority 1)

    std::vector<Flashcard> deck = {c1, c2, c3, c4};
    std::sort(deck.begin(), deck.end());

    // Expected order:
    // 1st: C4 (Due immediately, empty timestamp)
    // 2nd: C2 (Due 2026-10-15, difficulty 5)
    // 3rd: C3 (Due 2026-10-15, difficulty 1)
    // 4th: C1 (Due 2026-10-25)
    assert(deck[0].getId() == "C4");
    assert(deck[1].getId() == "C2");
    assert(deck[2].getId() == "C3");
    assert(deck[3].getId() == "C1");

    std::cout << "  -> Passed: std::sort successfully ordered deck by urgent review priority." << std::endl;
}

void test_pre_and_post_increment() {
    std::cout << "[RUN] Test 7: Pre-increment (++card) and post-increment (card++)..." << std::endl;

    Flashcard card("CARD-INC", "Front", "Back");
    assert(card.getReviewAttempts() == 0);

    // Pre-increment: returns reference to updated card
    Flashcard& ref = ++card;
    assert(ref.getReviewAttempts() == 1);
    assert(card.getReviewAttempts() == 1);

    // Post-increment: returns copy of card prior to increment
    Flashcard oldCopy = card++;
    assert(oldCopy.getReviewAttempts() == 1);
    assert(card.getReviewAttempts() == 2);

    std::cout << "  -> Passed: ++card and card++ adhere to standard C++ operator semantics." << std::endl;
}

void test_attempts_vs_repetitions_separation() {
    std::cout << "[RUN] Test 8: Separation of review attempts and successful repetitions..." << std::endl;

    Flashcard card("CARD-REP", "Question", "Answer");
    assert(card.getReviewAttempts() == 0);
    assert(card.getSuccessfulRepetitions() == 0);

    // Failed attempt (AGAIN)
    card.recordAttempt(false);
    assert(card.getReviewAttempts() == 1);
    assert(card.getSuccessfulRepetitions() == 0); // No successful rep

    // Another failed attempt
    card.recordAttempt(false);
    assert(card.getReviewAttempts() == 2);
    assert(card.getSuccessfulRepetitions() == 0);

    // Successful attempt (GOOD)
    card.recordAttempt(true);
    assert(card.getReviewAttempts() == 3);
    assert(card.getSuccessfulRepetitions() == 1);

    // Another successful attempt (EASY)
    card.recordAttempt(true);
    assert(card.getReviewAttempts() == 4);
    assert(card.getSuccessfulRepetitions() == 2);

    std::cout << "  -> Passed: Review presentation attempts tracked independently from successful recalls." << std::endl;
}

void test_stream_output() {
    std::cout << "[RUN] Test 9: Formatted stream output (operator<<)..." << std::endl;

    Flashcard card("CARD-STREAM", "Euler's Formula", "e^(i*pi) + 1 = 0", "Mathematics",
                  "Complex Analysis", {"#formula", "#euler"}, true, 4, 7, 2.6);

    std::ostringstream oss;
    oss << card;
    std::string output = oss.str();

    // Verify key formatted fields appear in stream output
    assert(output.find("CARD-STREAM") != std::string::npos);
    assert(output.find("Mathematics") != std::string::npos);
    assert(output.find("Diff: 4") != std::string::npos);
    assert(output.find("Int: 7d") != std::string::npos);
    assert(output.find("Formula") != std::string::npos);
    assert(output.find("Euler's Formula") != std::string::npos);

    std::cout << "  -> Passed: operator<< generates clean, formatted card descriptions." << std::endl;
}

void test_copy_and_move_semantics() {
    std::cout << "[RUN] Test 10: Copying and moving Flashcard objects (Rule of 5)..." << std::endl;

    Flashcard original("CARD-ORIG", "Original Question", "Original Answer", "Physics",
                      "Mechanics", {"#physics"}, false, 4, 5, 2.5, 3, 2, "2026-10-18");

    // 1. Copy Construction
    Flashcard copy = original;
    assert(copy == original);
    assert(copy.getFront() == original.getFront());

    // Mutating copy leaves original unaffected
    copy.setFront("Modified Front (Copy Only)");
    assert(copy.getFront() == "Modified Front (Copy Only)");
    assert(original.getFront() == "Original Question");

    // 2. Copy Assignment
    Flashcard assigned("CARD-TMP", "Temp", "Temp");
    assigned = original;
    assert(assigned == original);
    assert(assigned.getSubject() == "Physics");

    // 3. Move Construction
    Flashcard moved(std::move(copy));
    assert(moved.getId() == "CARD-ORIG");
    assert(moved.getFront() == "Modified Front (Copy Only)");

    // 4. Move Assignment
    Flashcard target("CARD-TARGET", "T", "T");
    target = std::move(moved);
    assert(target.getId() == "CARD-ORIG");
    assert(target.getFront() == "Modified Front (Copy Only)");

    std::cout << "  -> Passed: Value semantics, deep copy isolation, and move semantics verified." << std::endl;
}

int main() {
    std::cout << "===================================================" << std::endl;
    std::cout << " StudySync - Task 2.2 Flashcard Model Test Suite   " << std::endl;
    std::cout << "===================================================" << std::endl;

    test_construction_and_defaults();
    test_invalid_field_rejection();
    test_difficulty_and_ease_factor_clamping();
    test_identity_equality_operators();
    test_operator_less_tie_breakers();
    test_std_sort_review_priority();
    test_pre_and_post_increment();
    test_attempts_vs_repetitions_separation();
    test_stream_output();
    test_copy_and_move_semantics();

    std::cout << "===================================================" << std::endl;
    std::cout << " ALL FLASHCARD TESTS PASSED! (10/10 Test Suites)   " << std::endl;
    std::cout << "===================================================" << std::endl;

    return 0;
}
