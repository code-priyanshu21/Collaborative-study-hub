#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <cmath>

#include "models/Flashcard.hpp"
#include "models/FlashcardDeck.hpp"
#include "ds/HashTable.hpp"
#include "storage/FlashcardStorage.hpp"

using namespace studysync;

// Helper to remove temporary test files
static void cleanFile(const std::string& filepath) {
    std::error_code ec;
    std::filesystem::remove(filepath, ec);
    std::filesystem::remove(filepath + ".tmp", ec);
    std::filesystem::remove(filepath + ".bak", ec);
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " StudySync - Task 2.4 FlashcardStorage & Formula Indexing " << std::endl;
    std::cout << "==========================================================" << std::endl;

    // Ensure data directory exists
    std::error_code dirEc;
    std::filesystem::create_directories("data", dirEc);

    // ========================================================================
    // Test 1: Saving and loading ordinary flashcards
    // ========================================================================
    {
        std::cout << "[RUN] Test 1: Saving and loading ordinary flashcards..." << std::endl;
        std::string testPath = "data/test_regular_cards.dat";
        cleanFile(testPath);

        std::vector<Flashcard> originalCards = {
            Flashcard("FC-01", "What is an ADT?", "An Abstract Data Type defines behavior without implementation.",
                      "Data Structures", "Basics", {"#dsa", "#concept"}, false, 2, 3, 2.6, 5, 4, "2026-10-20"),
            Flashcard("FC-02", "What is amortized analysis?", "Averaged running time over a sequence of operations.",
                      "Algorithms", "Complexity", {"#algorithms"}, false, 4, 1, 2.36, 3, 1, "2026-10-17")
        };

        FlashcardStorage::saveCards(testPath, originalCards);
        assert(std::filesystem::exists(testPath));

        std::vector<Flashcard> loadedCards = FlashcardStorage::loadCards(testPath, true);
        assert(loadedCards.size() == 2);

        // Verify FC-01
        assert(loadedCards[0].getId() == "FC-01");
        assert(loadedCards[0].getFront() == "What is an ADT?");
        assert(loadedCards[0].getBack() == "An Abstract Data Type defines behavior without implementation.");
        assert(loadedCards[0].getSubject() == "Data Structures");
        assert(loadedCards[0].getTopic() == "Basics");
        assert(loadedCards[0].getTags().size() == 2);
        assert(loadedCards[0].isFormulaCard() == false);
        assert(loadedCards[0].getDifficulty() == 2);
        assert(loadedCards[0].getInterval() == 3);
        assert(std::abs(loadedCards[0].getEaseFactor() - 2.6) < 0.001);
        assert(loadedCards[0].getReviewAttempts() == 5);
        assert(loadedCards[0].getSuccessfulRepetitions() == 4);
        assert(loadedCards[0].getNextReviewTimestamp() == "2026-10-20");

        // Verify FC-02
        assert(loadedCards[1].getId() == "FC-02");
        assert(loadedCards[1].getFront() == "What is amortized analysis?");
        assert(loadedCards[1].isFormulaCard() == false);
        assert(loadedCards[1].getDifficulty() == 4);
        assert(loadedCards[1].getInterval() == 1);
        assert(std::abs(loadedCards[1].getEaseFactor() - 2.36) < 0.001);

        cleanFile(testPath);
        std::cout << "  -> Passed: Regular flashcards saved and loaded with all metrics intact." << std::endl;
    }

    // ========================================================================
    // Test 2: Saving and loading formula cards
    // ========================================================================
    {
        std::cout << "[RUN] Test 2: Saving and loading formula cards..." << std::endl;
        std::string testPath = "data/test_formula_cards.dat";
        cleanFile(testPath);

        std::vector<Flashcard> formulaCards = {
            Flashcard("F-01", "Pythagorean Theorem", "a^2 + b^2 = c^2",
                      "Mathematics", "Geometry", {"#math", "#geometry"}, true, 1, 5, 2.7, 8, 7, "2026-11-01"),
            Flashcard("F-02", "Quadratic Formula", "x = (-b +- sqrt(b^2-4ac))/(2a)",
                      "Mathematics", "Algebra", {"#math", "#algebra"}, true, 3, 2, 2.5, 4, 3, "2026-10-25")
        };

        FlashcardStorage::saveCards(testPath, formulaCards);
        std::vector<Flashcard> loaded = FlashcardStorage::loadCards(testPath, true);

        assert(loaded.size() == 2);
        assert(loaded[0].isFormulaCard() == true);
        assert(loaded[1].isFormulaCard() == true);
        assert(loaded[0].getFront() == "Pythagorean Theorem");
        assert(loaded[0].getBack() == "a^2 + b^2 = c^2");
        assert(loaded[1].getFront() == "Quadratic Formula");
        assert(loaded[1].getBack() == "x = (-b +- sqrt(b^2-4ac))/(2a)");

        cleanFile(testPath);
        std::cout << "  -> Passed: Formula cards and isFormula flags preserved correctly." << std::endl;
    }

    // ========================================================================
    // Test 3: Multiple formula cards sharing keywords
    // ========================================================================
    {
        std::cout << "[RUN] Test 3: Multiple formula cards sharing keywords..." << std::endl;
        HashTable formulaIndex;

        Flashcard c1("F-CALC-1", "Derivative Power Rule", "d/dx [x^n] = n*x^(n-1)",
                     "Calculus", "Differentiation", {"#calculus", "#derivatives"}, true);
        Flashcard c2("F-CALC-2", "Derivative Product Rule", "d/dx [u*v] = u'*v + u*v'",
                     "Calculus", "Differentiation", {"#calculus", "#derivatives"}, true);
        Flashcard c3("F-CALC-3", "Integral Power Rule", "int x^n dx = (x^(n+1))/(n+1) + C",
                     "Calculus", "Integration", {"#calculus", "#integrals"}, true);

        FlashcardStorage::indexFormulaCard(c1, formulaIndex);
        FlashcardStorage::indexFormulaCard(c2, formulaIndex);
        FlashcardStorage::indexFormulaCard(c3, formulaIndex);

        // All 3 cards share the subject "Calculus"
        auto calcIds = FlashcardStorage::searchFormulaIds("calculus", formulaIndex);
        assert(calcIds.size() == 3);
        assert(std::find(calcIds.begin(), calcIds.end(), "F-CALC-1") != calcIds.end());
        assert(std::find(calcIds.begin(), calcIds.end(), "F-CALC-2") != calcIds.end());
        assert(std::find(calcIds.begin(), calcIds.end(), "F-CALC-3") != calcIds.end());

        // c1 and c2 share the keyword "derivative"
        auto derivIds = FlashcardStorage::searchFormulaIds("derivative", formulaIndex);
        assert(derivIds.size() == 2);
        assert(std::find(derivIds.begin(), derivIds.end(), "F-CALC-1") != derivIds.end());
        assert(std::find(derivIds.begin(), derivIds.end(), "F-CALC-2") != derivIds.end());

        // c1 and c3 share the keyword "power"
        auto powerIds = FlashcardStorage::searchFormulaIds("power", formulaIndex);
        assert(powerIds.size() == 2);
        assert(std::find(powerIds.begin(), powerIds.end(), "F-CALC-1") != powerIds.end());
        assert(std::find(powerIds.begin(), powerIds.end(), "F-CALC-3") != powerIds.end());

        std::cout << "  -> Passed: One-to-many keyword mapping functions without ID collisions or loss." << std::endl;
    }

    // ========================================================================
    // Test 4: Keyword, title, tag, subject, and topic searches
    // ========================================================================
    {
        std::cout << "[RUN] Test 4: Keyword, title, tag, subject, and topic searches..." << std::endl;
        HashTable index;

        std::vector<Flashcard> cards = {
            Flashcard("F-AVL", "AVL Tree Balance Factor", "BF(node) = height(left) - height(right)",
                      "Data Structures", "Self Balancing Trees", {"#avl", "#trees"}, true),
            Flashcard("F-BST", "Binary Search Tree Invariant", "left < root < right",
                      "Data Structures", "Trees", {"#bst", "#trees"}, true)
        };

        for (const auto& c : cards) {
            FlashcardStorage::indexFormulaCard(c, index);
        }

        // Title search
        auto titleMatches = FlashcardStorage::searchFormulas("AVL Tree Balance Factor", index, cards);
        assert(titleMatches.size() == 1);
        assert(titleMatches[0]->getId() == "F-AVL");

        // Keyword search
        auto kwMatches = FlashcardStorage::searchFormulas("balance", index, cards);
        assert(kwMatches.size() == 1);
        assert(kwMatches[0]->getId() == "F-AVL");

        // Tag search with '#'
        auto tagWithHash = FlashcardStorage::searchFormulas("#trees", index, cards);
        assert(tagWithHash.size() == 2);

        // Tag search without '#'
        auto tagWithoutHash = FlashcardStorage::searchFormulas("trees", index, cards);
        assert(tagWithoutHash.size() == 2);

        // Subject search
        auto subjMatches = FlashcardStorage::searchFormulas("Data Structures", index, cards);
        assert(subjMatches.size() == 2);

        // Topic search
        auto topicMatches = FlashcardStorage::searchFormulas("Self Balancing Trees", index, cards);
        assert(topicMatches.size() == 1);
        assert(topicMatches[0]->getId() == "F-AVL");

        std::cout << "  -> Passed: Accurate retrieval across titles, keywords, tags, subjects, and topics." << std::endl;
    }

    // ========================================================================
    // Test 5: Case handling and empty search terms
    // ========================================================================
    {
        std::cout << "[RUN] Test 5: Case handling and empty search terms..." << std::endl;
        HashTable index;
        Flashcard card("F-BINOM", "Binomial Coefficient", "n! / (k! * (n-k)!)",
                      "Combinatorics", "Discrete Math", {"#binomial", "#math"}, true);
        FlashcardStorage::indexFormulaCard(card, index);

        // Case insensitivity
        assert(FlashcardStorage::searchFormulaIds("BINOMIAL", index).size() == 1);
        assert(FlashcardStorage::searchFormulaIds("binomial", index).size() == 1);
        assert(FlashcardStorage::searchFormulaIds("BiNoMiAl", index).size() == 1);
        assert(FlashcardStorage::searchFormulaIds("  DiScReTe MaTh  ", index).size() == 1);

        // Empty and whitespace queries
        assert(FlashcardStorage::searchFormulaIds("", index).empty());
        assert(FlashcardStorage::searchFormulaIds("   ", index).empty());
        assert(FlashcardStorage::searchFormulaIds("\t\n", index).empty());

        // Non-existent search term
        assert(FlashcardStorage::searchFormulaIds("NonExistentTermXYZ", index).empty());

        std::cout << "  -> Passed: Case-insensitive lookups and safe empty query handling verified." << std::endl;
    }

    // ========================================================================
    // Test 6: Complex LaTeX and KaTeX round-trip preservation
    // ========================================================================
    {
        std::cout << "[RUN] Test 6: Complex LaTeX and KaTeX round-trip preservation..." << std::endl;
        std::string testPath = "data/test_latex_cards.dat";
        cleanFile(testPath);

        std::string latexFront = "Gaussian Integral: \\int_{-\\infty}^{\\infty} e^{-x^2} dx";
        std::string latexBack = "\\sqrt{\\pi}. Evaluation: \\left(\\int_{-\\infty}^{\\infty} e^{-x^2} dx\\right)^2 = \\int_0^{2\\pi} d\\theta \\int_0^{\\infty} r e^{-r^2} dr = \\pi";

        std::string katexFront = "Matrix Determinant: \\det \\begin{pmatrix} a & b \\\\ c & d \\end{pmatrix}";
        std::string katexBack = "ad - bc. Inverse: \\frac{1}{ad-bc} \\begin{pmatrix} d & -b \\\\ -c & a \\end{pmatrix}";

        std::vector<Flashcard> latexCards = {
            Flashcard("LATEX-1", latexFront, latexBack, "Calculus", "Multivariable", {"#integral", "#gaussian"}, true),
            Flashcard("LATEX-2", katexFront, katexBack, "Linear Algebra", "Matrices", {"#matrices", "#determinant"}, true)
        };

        FlashcardStorage::saveCards(testPath, latexCards);
        std::vector<Flashcard> loaded = FlashcardStorage::loadCards(testPath, true);

        assert(loaded.size() == 2);
        assert(loaded[0].getFront() == latexFront);
        assert(loaded[0].getBack() == latexBack);
        assert(loaded[1].getFront() == katexFront);
        assert(loaded[1].getBack() == katexBack);

        cleanFile(testPath);
        std::cout << "  -> Passed: Complex LaTeX & KaTeX expressions restored verbatim without distortion." << std::endl;
    }

    // ========================================================================
    // Test 7: Backslashes, quotes, delimiters, and multiline fields
    // ========================================================================
    {
        std::cout << "[RUN] Test 7: Backslashes, quotes, delimiters, and multiline fields..." << std::endl;
        std::string testPath = "data/test_special_chars.dat";
        cleanFile(testPath);

        std::string multiFront = "Question on paths & quotes:\n"
                                 "Line 1: Windows path: C:\\Program Files\\StudySync\\bin\\\n"
                                 "Line 2: Quote: \"To be, or not to be\"\n"
                                 "Line 3: Delimiters: [NOTE], [VERSION], [FLASHCARD], commas: a,b,c";

        std::string multiBack = "Answer:\n"
                                "\tIndented item 1 with tab\n"
                                "\tIndented item 2: 'single' and \"double\" quotes\n"
                                "End of explanation.";

        Flashcard specialCard("SPEC-01", multiFront, multiBack, "System Programming",
                              "File I/O", {"#io", "#escapes", "#special"}, false);

        FlashcardStorage::saveCards(testPath, {specialCard});
        std::vector<Flashcard> loaded = FlashcardStorage::loadCards(testPath, true);

        assert(loaded.size() == 1);
        assert(loaded[0].getFront() == multiFront);
        assert(loaded[0].getBack() == multiBack);
        assert(loaded[0].getTags().size() == 3);

        cleanFile(testPath);
        std::cout << "  -> Passed: Multiline strings, quotes, tabs, and backslashes round-tripped losslessly." << std::endl;
    }

    // ========================================================================
    // Test 8: Missing and empty files
    // ========================================================================
    {
        std::cout << "[RUN] Test 8: Missing and empty files..." << std::endl;
        std::string nonExistent = "data/non_existent_deck.dat";
        cleanFile(nonExistent);

        // When errorIfMissing is false, returns empty vector
        auto cards = FlashcardStorage::loadCards(nonExistent, false);
        assert(cards.empty());

        // When errorIfMissing is true, throws FileAccessStorageException
        bool threwMissing = false;
        try {
            FlashcardStorage::loadCards(nonExistent, true);
        } catch (const FileAccessStorageException&) {
            threwMissing = true;
        }
        assert(threwMissing);

        // Empty file (0 bytes) should throw CorruptedRecordStorageException
        std::string emptyFile = "data/empty_deck.dat";
        cleanFile(emptyFile);
        std::ofstream out(emptyFile);
        out.close();

        bool threwEmpty = false;
        try {
            FlashcardStorage::loadCards(emptyFile, false);
        } catch (const CorruptedRecordStorageException&) {
            threwEmpty = true;
        }
        assert(threwEmpty);

        cleanFile(emptyFile);
        std::cout << "  -> Passed: Missing and empty file policies strictly enforced." << std::endl;
    }

    // ========================================================================
    // Test 9: Corrupted headers and incomplete records
    // ========================================================================
    {
        std::cout << "[RUN] Test 9: Corrupted headers and incomplete records..." << std::endl;
        std::string corruptPath = "data/corrupt_header.dat";
        cleanFile(corruptPath);

        // 1. Corrupt Header
        {
            std::ofstream out(corruptPath);
            out << "INVALID_FLASHCARDS_HEADER\n[END_STORAGE]\n";
            out.close();

            bool threw = false;
            try {
                FlashcardStorage::loadCards(corruptPath);
            } catch (const CorruptedRecordStorageException&) {
                threw = true;
            }
            assert(threw);
        }

        // 2. Missing Footer
        {
            std::ofstream out(corruptPath);
            out << FlashcardStorage::FORMAT_HEADER_V1 << "\n";
            out << "[FLASHCARD]\nID: C1\nFRONT: Q\nBACK: A\n[END_FLASHCARD]\n";
            // No [END_STORAGE]
            out.close();

            bool threw = false;
            try {
                FlashcardStorage::loadCards(corruptPath);
            } catch (const CorruptedRecordStorageException&) {
                threw = true;
            }
            assert(threw);
        }

        // 3. Incomplete Flashcard (missing [END_FLASHCARD])
        {
            std::ofstream out(corruptPath);
            out << FlashcardStorage::FORMAT_HEADER_V1 << "\n";
            out << "[FLASHCARD]\nID: C1\nFRONT: Q\nBACK: A\n";
            out << FlashcardStorage::FORMAT_FOOTER << "\n";
            out.close();

            bool threw = false;
            try {
                FlashcardStorage::loadCards(corruptPath);
            } catch (const CorruptedRecordStorageException&) {
                threw = true;
            }
            assert(threw);
        }

        cleanFile(corruptPath);
        std::cout << "  -> Passed: Malformed headers and incomplete block records detected and rejected." << std::endl;
    }

    // ========================================================================
    // Test 10: Duplicate card IDs in persisted data
    // ========================================================================
    {
        std::cout << "[RUN] Test 10: Duplicate card IDs..." << std::endl;
        std::string dupPath = "data/duplicate_cards.dat";
        cleanFile(dupPath);

        std::ofstream out(dupPath);
        out << FlashcardStorage::FORMAT_HEADER_V1 << "\n";
        out << "[FLASHCARD]\nID: SAME_ID\nFRONT: Q1\nBACK: A1\n[END_FLASHCARD]\n";
        out << "[FLASHCARD]\nID: SAME_ID\nFRONT: Q2\nBACK: A2\n[END_FLASHCARD]\n";
        out << FlashcardStorage::FORMAT_FOOTER << "\n";
        out.close();

        bool threwDup = false;
        try {
            FlashcardStorage::loadCards(dupPath);
        } catch (const CorruptedRecordStorageException&) {
            threwDup = true;
        }
        assert(threwDup);

        cleanFile(dupPath);
        std::cout << "  -> Passed: Duplicate card IDs rejected with CorruptedRecordStorageException." << std::endl;
    }

    // ========================================================================
    // Test 11: Failure during saving and preservation of existing data
    // ========================================================================
    {
        std::cout << "[RUN] Test 11: Failure during saving and preservation of existing data..." << std::endl;
        std::string targetPath = "data/test_preserve_cards.dat";
        cleanFile(targetPath);

        std::vector<Flashcard> initialCards = {
            Flashcard("P1", "Original Prompt 1", "Original Answer 1", "Subject 1", "Topic 1", {}, true),
            Flashcard("P2", "Original Prompt 2", "Original Answer 2", "Subject 2", "Topic 2", {}, true)
        };
        FlashcardStorage::saveCards(targetPath, initialCards);
        assert(std::filesystem::exists(targetPath));

        // Attempting to save to an invalid target directory should throw and leave targetPath unaffected
        std::string invalidPath = "Z:/non_existent_folder_xyz/cards.dat";
        bool threwInvalid = false;
        try {
            FlashcardStorage::saveCards(invalidPath, initialCards);
        } catch (const FileAccessStorageException&) {
            threwInvalid = true;
        }
        assert(threwInvalid);

        // Verify targetPath is still fully intact and unchanged
        auto verifiedCards = FlashcardStorage::loadCards(targetPath, true);
        assert(verifiedCards.size() == 2);
        assert(verifiedCards[0].getId() == "P1");
        assert(verifiedCards[1].getId() == "P2");

        cleanFile(targetPath);
        std::cout << "  -> Passed: Target data remains intact and protected upon save failure." << std::endl;
    }

    // ========================================================================
    // Test 12: Rebuilding indexes after loading
    // ========================================================================
    {
        std::cout << "[RUN] Test 12: Rebuilding indexes after loading..." << std::endl;
        std::string testPath = "data/test_rebuild_index.dat";
        cleanFile(testPath);

        std::vector<Flashcard> cards = {
            Flashcard("F-SORT", "QuickSort Partitioning", "Hoare or Lomuto partition scheme",
                      "Algorithms", "Sorting", {"#sorting", "#dsa"}, true),
            Flashcard("F-SEARCH", "Binary Search Condition", "Array must be sorted in monotonic order",
                      "Algorithms", "Searching", {"#searching", "#dsa"}, true),
            Flashcard("NON-FORMULA", "What is an algorithm?", "A finite sequence of well-defined instructions",
                      "Computer Science", "Theory", {"#theory"}, false)
        };

        FlashcardStorage::saveCards(testPath, cards);

        HashTable autoIndex;
        auto loaded = FlashcardStorage::loadCards(testPath, true, &autoIndex);
        assert(loaded.size() == 3);

        // autoIndex should index the 2 formula cards, skipping the non-formula card
        assert(autoIndex.contains("sorting", "F-SORT") || autoIndex.contains("#sorting", "F-SORT"));
        assert(autoIndex.contains("searching", "F-SEARCH") || autoIndex.contains("#searching", "F-SEARCH"));
        assert(!autoIndex.containsKey("theory")); // non-formula card was not indexed in formula index

        // Search through index directly after loading
        auto sortMatches = FlashcardStorage::searchFormulas("partition", autoIndex, loaded);
        assert(sortMatches.size() == 1);
        assert(sortMatches[0]->getId() == "F-SORT");

        cleanFile(testPath);
        std::cout << "  -> Passed: Formula index reconstructed accurately and dynamically upon load." << std::endl;
    }

    // ========================================================================
    // Test 13: Removing or updating indexed cards without stale entries
    // ========================================================================
    {
        std::cout << "[RUN] Test 13: Removing or updating indexed cards without stale entries..." << std::endl;
        HashTable index;

        Flashcard card("F-MUTABLE", "Euler Characteristic: V - E + F = 2", "Polyhedral formula",
                      "Topology", "Polyhedra", {"#topology", "#euler"}, true);
        FlashcardStorage::indexFormulaCard(card, index);

        // Verify initial index state
        assert(FlashcardStorage::searchFormulaIds("euler", index).size() == 1);
        assert(FlashcardStorage::searchFormulaIds("topology", index).size() == 1);

        // 1. Update card: changes front, subject, and tags
        Flashcard updatedCard("F-MUTABLE", "Gauss Divergence Theorem", "Flux through closed surface equals volume integral of divergence",
                             "Vector Calculus", "Electromagnetism", {"#gauss", "#divergence"}, true);
        FlashcardStorage::updateCardInIndex(updatedCard, index);

        // Stale entries from previous version MUST be gone
        assert(FlashcardStorage::searchFormulaIds("euler", index).empty());
        assert(FlashcardStorage::searchFormulaIds("topology", index).empty());
        assert(FlashcardStorage::searchFormulaIds("polyhedra", index).empty());

        // New entries must be active
        assert(FlashcardStorage::searchFormulaIds("gauss", index).size() == 1);
        assert(FlashcardStorage::searchFormulaIds("divergence", index).size() == 1);
        assert(FlashcardStorage::searchFormulaIds("vector calculus", index).size() == 1);

        // 2. Remove card entirely
        bool removed = FlashcardStorage::removeCardFromIndex("F-MUTABLE", index);
        assert(removed);

        // All entries for F-MUTABLE must be purged
        assert(FlashcardStorage::searchFormulaIds("gauss", index).empty());
        assert(FlashcardStorage::searchFormulaIds("divergence", index).empty());
        assert(FlashcardStorage::searchFormulaIds("vector calculus", index).empty());
        assert(index.empty()); // No leftover empty nodes or keys

        std::cout << "  -> Passed: Stale index entries completely purged upon mutation and deletion." << std::endl;
    }

    // ========================================================================
    // Test 14: FlashcardDeck integration with save and loadDeck
    // ========================================================================
    {
        std::cout << "[RUN] Test 14: FlashcardDeck save and loadDeck integration..." << std::endl;
        std::string deckPath = "data/test_deck_integration.dat";
        cleanFile(deckPath);

        FlashcardDeck originalDeck;
        originalDeck.addCard(Flashcard("D1", "What is an AVL Tree?", "Self-balancing binary search tree", "DSA", "Trees", {"#avl"}, true));
        originalDeck.addCard(Flashcard("D2", "What is Big-O?", "Upper bound asymptotic notation", "DSA", "Complexity", {"#bigo"}, false));

        FlashcardStorage::saveCards(deckPath, originalDeck);

        FlashcardDeck loadedDeck;
        HashTable deckIndex;
        FlashcardStorage::loadDeck(deckPath, loadedDeck, true, &deckIndex);

        assert(loadedDeck.cardCount() == 2);
        assert(loadedDeck.hasCard("D1"));
        assert(loadedDeck.hasCard("D2"));
        assert(deckIndex.containsKey("avl") || deckIndex.containsKey("#avl"));

        cleanFile(deckPath);
        std::cout << "  -> Passed: FlashcardDeck save and loadDeck work seamlessly together." << std::endl;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL FLASHCARD STORAGE & INDEXING TESTS PASSED! (14/14)   " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
